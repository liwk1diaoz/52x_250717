#if defined __KERNEL__
#include <linux/string.h>   // for memset
#else
#include <string.h>         // for memset, strncmp
#include <stdio.h>          // sscanf
#endif
#include <kwrap/task.h>
#include <kwrap/file.h>
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>

#include "nvtmpp_init.h"
#include "nvtmpp_int.h"
#include "nvtmpp_blk.h"
#include "nvtmpp_pool.h"
#include "nvtmpp_debug_cmd.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_heap.h"
#include "nvtmpp_platform.h"

typedef struct _NVTMPP_CHK_MEM_CTRL {
	UINT32                   init;
	BOOL                     start;
	UINT32                   sleep_ms;
	THREAD_HANDLE            thread;
} NVTMPP_CHK_MEM_CTRL;


#define  NVTMPP_TEST_CMD    DISABLE

#if NVTMPP_TEST_CMD
static NVTMPP_VB_POOL       g_pool = NVTMPP_VB_INVALID_POOL;
static NVTMPP_VB_BLK        g_blk = NVTMPP_VB_INVALID_BLK;
static NVTMPP_MODULE        g_module = MAKE_NVTMPP_MODULE('u', 's', 'e', 'r', 0, 0, 0, 0);
#endif

static NVTMPP_CHK_MEM_CTRL  nvtmpp_check_mem_ctrl;

#if NVTMPP_FASTBOOT_SUPPORT
extern NVTMPP_VB_POOL_S *nvtmpp_vb_get_common_pool(UINT32 id, BOOL is_pool_type_common);
extern NVTMPP_VB_POOL_S *nvtmpp_vb_get_pool_by_name(char *name);
extern NVTMPP_VB_POOL_S *nvtmpp_vb_get_first_private_pool(NVTMPP_DDR ddr_id);
extern UINT32            nvtmpp_vb_arrange_fastboot_next_blk(UINT32 blk_addr, UINT32 blk_size);
#endif


static THREAD_DECLARE(nvtmpp_check_mem_func, arglist)
{
	NVTMPP_CHK_MEM_CTRL    *p_ctrl;

	p_ctrl = (NVTMPP_CHK_MEM_CTRL    *)arglist;
	while (!THREAD_SHOULD_STOP) {
		if (!p_ctrl->start) {
			break;
		}
		vos_task_delay_ms(p_ctrl->sleep_ms);
		nvtmpp_heap_check_mem_corrupt();
	}
	THREAD_RETURN(0);
}

static BOOL cmd_nvtmpp_dump_info(unsigned char argc, char **argv)
{
	nvtmpp_dump_status(vk_printk);
	return TRUE;
}


static BOOL cmd_nvtmpp_dump_max(unsigned char argc, char **argv)
{
	UINT32 pool_id = 0, max_cnt = 0;

	//sscanf_s(str_cmd, "%d %d", &pool_id, &max_cnt);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&pool_id);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&max_cnt);
	}
	if (nvtmpp_vb_set_dump_max_cnt(pool_id, max_cnt) == TRUE) {
		DBG_DUMP("set pool_id %d, max_cnt %d success\r\n", (int)pool_id, (int)max_cnt);
	} else {
		DBG_ERR("set pool_id %d, max_cnt %d fail\r\n", (int)pool_id, (int)max_cnt);
	}
	return TRUE;
}

static BOOL cmd_nvtmpp_showmsg(unsigned char argc, char **argv)
{
	UINT32 showmsg_level = 1;

	//sscanf_s(str_cmd, "%d", &showmsg_level);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&showmsg_level);
	}
	nvtmpp_vb_set_showmsg_level(showmsg_level);
	return TRUE;
}

static BOOL cmd_nvtmpp_check_mem_corrupt(unsigned char argc, char **argv)
{
	UINT32               is_start = TRUE;
	UINT32               sleep_ms = 2000;
	NVTMPP_CHK_MEM_CTRL  *p_ctrl;

	p_ctrl = &nvtmpp_check_mem_ctrl;

	// read parameter from command
	//sscanf_s(str_cmd, "%d %d", &is_start, &sleep_ms);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&is_start);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&sleep_ms);
	}
	// start check_mem_corrupt
	if (is_start) {
		if (TRUE == p_ctrl->start) {
			DBG_WRN("check_mem_corrupt already started\r\n");
			return FALSE;
		}
		p_ctrl->start = TRUE;
		p_ctrl->sleep_ms = sleep_ms;
		DBG_DUMP("start check_mem_corrupt\r\n");
		nvtmpp_vb_set_memory_corrupt_check_en(TRUE);
		THREAD_CREATE(p_ctrl->thread, nvtmpp_check_mem_func, p_ctrl, "chk_mem_thread");
		THREAD_RESUME(p_ctrl->thread);
	} else {
	// stop check_mem_corrupt
		if (TRUE != p_ctrl->start) {
			DBG_WRN("check_mem_corrupt not started\r\n");
			return FALSE;
		}
		p_ctrl->start = FALSE;
		nvtmpp_vb_set_memory_corrupt_check_en(FALSE);
		DBG_DUMP("stop check_mem_corrupt\r\n");
		//THREAD_DESTROY(p_ctrl->thread);
	}
	return TRUE;
}

#if defined(__LINUX)
static BOOL cmd_nvtmpp_dump_stack(unsigned char argc, char **argv)
{
	UINT32               pid = 0;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&pid);
	}
	nvtmpp_dump_stack_by_pid(pid);
	return TRUE;
}

static BOOL cmd_nvtmpp_mem_r(unsigned char argc, char **argv)
{
	UINT32  pa = 0 , length = 256;

	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&pa);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&length);
	}
	nvtmpp_dumpmem(pa, length);
	return TRUE;
}

static BOOL cmd_nvtmpp_mem_dump(unsigned char argc, char **argv)
{
	UINT32  length = 256;
	UINT32  pa = 0;
	#define FILENAME_MAX_LEN 64
	char    filename[FILENAME_MAX_LEN] = "/tmp/dump.bin";

	//argc = sscanf_s(str_cmd, "%x %x %s\n", &pa, &length, filename, sizeof(filename));
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&pa);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&length);
	}
	if (argc >= 3) {
		strncpy(filename, argv[2], FILENAME_MAX_LEN - 1);
		filename[FILENAME_MAX_LEN - 1] = 0; //fill string end
	}
	if ((argc == 1) || (argc == 2) || (argc == 3)) {
		nvtmpp_dumpmem2file(pa, length, filename);
	} else {
		DBG_DUMP("syntex: memdump [addr] (length) (filename)\r\n");
		return FALSE;
	}
	return TRUE;
}
#endif

#if NVTMPP_FASTBOOT_SUPPORT
static BOOL cmd_nvtmpp_fastboot_mem(unsigned char argc, char **argv)
{
	NVTMPP_VB_POOL_S *p_pool;
	NVTMPP_VB_POOL_S *p_first_pv_pool[NVTMPP_DDR_MAX_NUM];
	NVTMPP_FBOOT_POOL_DTS_INFO_S *p_fboot_pool;
	UINT32           blk_addr[NVTMPP_DDR_MAX_NUM], blk_cnt;
	int              fd, len, i , j, pool_id, /*path_id */ misc_cpool_id /*, first_misc_cpool_id = 0*/;
	char             tmp_buf[80]="/mnt/sd/nvt-na51055-fastboot-hdal-mem.dtsi";
	NVTMPP_DDR       ddr_id;

	if (argc >= 1) {
		sscanf(argv[0], "%s", tmp_buf);
	}
	fd = vos_file_open(tmp_buf, O_CREAT|O_TRUNC|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		printk("open %s failure\r\n", tmp_buf);
		return FALSE;
	}
	// dts begin
	len = snprintf(tmp_buf, sizeof(tmp_buf), "&fastboot {\r\n\thdal-mem {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	#if 1
	blk_cnt = 0;
	// common blocks
	for (pool_id = 0 ; pool_id < NVTMPP_VB_MAX_COMM_POOLS; pool_id++) {
		p_pool = nvtmpp_vb_get_common_pool(pool_id, TRUE);
		if (NULL == p_pool) {
			continue;
		}
		for (i = 0; i < p_pool->blk_cnt; i++) {
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tcomn_blk_%d = <0x%x 0x%x>;\r\n", blk_cnt,
				(int)p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i), (int)p_pool->blk_size);
			vos_file_write(fd, (void *)tmp_buf, len);
			blk_cnt++;
		}
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tcomn_blk_cnt = <%d>;\r\n", (int)blk_cnt);
	vos_file_write(fd, (void *)tmp_buf, len);
	// misc common pool type
	misc_cpool_id = 0;
	for (pool_id = 0 ; pool_id < NVTMPP_VB_MAX_COMM_POOLS; pool_id++) {
		p_pool = nvtmpp_vb_get_common_pool(pool_id, FALSE);
		if (NULL == p_pool) {
			continue;
		}
		len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_pool_type = <%d>;\r\n", misc_cpool_id, (int)p_pool->pool_type);
		vos_file_write(fd, (void *)tmp_buf, len);
		len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_blk_cnt = <%d>;\r\n", misc_cpool_id, (int)p_pool->blk_cnt);
		vos_file_write(fd, (void *)tmp_buf, len);
		for (i = 0;i < p_pool->blk_cnt; i++) {
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_blk_%d = <0x%x 0x%x>;\r\n", misc_cpool_id, i, (int)p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i), (int)p_pool->blk_size);
			vos_file_write(fd, (void *)tmp_buf, len);
		}
		misc_cpool_id ++;
		if (misc_cpool_id > FBOOT_MISC_CPOOL_MAX) {
			break;
		}
	}
	#else
	// vcap common blocks
	p_pool = nvtmpp_vb_get_common_pool(0, TRUE);
	if (NULL == p_pool) {
		goto dump_err;
	}
	if (p_pool->blk_cnt < FBOOT_VCAP_COMNBLK_MIN_CNT) {
		goto dump_err;
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvcap_blk_cnt = <%d>;\r\n", (int)p_pool->blk_cnt);
	vos_file_write(fd, (void *)tmp_buf, len);
	for (i = 0;i < p_pool->blk_cnt; i++) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvcap_blk_%d = <0x%x 0x%x>;\r\n", i, (int)nvtmpp_sys_va2pa(p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i)), (int)p_pool->blk_size);
		vos_file_write(fd, (void *)tmp_buf, len);
	}
	// vprc path1 common blocks
	p_pool = nvtmpp_vb_get_common_pool(1, TRUE);
	if (NULL == p_pool) {
		goto dump_err;
	}
	if (p_pool->blk_cnt < FBOOT_VPRC_COMNBLK_MIN_CNT) {
		goto dump_err;
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvprc_blk_cnt = <%d>;\r\n", (int)p_pool->blk_cnt);
	vos_file_write(fd, (void *)tmp_buf, len);
	for (i = 0;i < p_pool->blk_cnt; i++) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvprc_blk_%d = <0x%x 0x%x>;\r\n", i, (int)nvtmpp_sys_va2pa(p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i)), (int)p_pool->blk_size);
		vos_file_write(fd, (void *)tmp_buf, len);
	}
	// vproc path2, path3 ...
	for (path_id = 1 ; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
		p_pool = nvtmpp_vb_get_common_pool(path_id+1, TRUE);
		if (NULL == p_pool || p_pool->blk_cnt < FBOOT_VPRC_COMNBLK_MIN_CNT) {
			break;
		}
		len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvprc_p%d_blk_cnt = <%d>;\r\n", path_id+1, (int)p_pool->blk_cnt);
		vos_file_write(fd, (void *)tmp_buf, len);
		for (i = 0;i < p_pool->blk_cnt; i++) {
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvprc_p%d_blk_%d = <0x%x 0x%x>;\r\n", path_id+1, i, (int)nvtmpp_sys_va2pa(p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i)), (int)p_pool->blk_size);
			vos_file_write(fd, (void *)tmp_buf, len);
		}
	}
	for (misc_cpool_id = 1 ; misc_cpool_id < NVTMPP_VB_MAX_COMM_POOLS; misc_cpool_id++) {
		p_pool = nvtmpp_vb_get_common_pool(misc_cpool_id, FALSE);
		if (NULL != p_pool) {
			first_misc_cpool_id = misc_cpool_id;
			break;
		}
	}
	// misc common pool type
	if (first_misc_cpool_id != 0) {
		for (misc_cpool_id = 0 ; misc_cpool_id < FBOOT_VPRC_MAX_PATH; misc_cpool_id++) {
			p_pool = nvtmpp_vb_get_common_pool(first_misc_cpool_id+misc_cpool_id, FALSE);
			if (NULL == p_pool) {
				break;
			}
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_pool_type = <%d>;\r\n", misc_cpool_id, (int)p_pool->pool_type);
			vos_file_write(fd, (void *)tmp_buf, len);
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_blk_cnt = <%d>;\r\n", misc_cpool_id, (int)p_pool->blk_cnt);
			vos_file_write(fd, (void *)tmp_buf, len);
			for (i = 0;i < p_pool->blk_cnt; i++) {
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tmisc_cpool%d_blk_%d = <0x%x 0x%x>;\r\n", misc_cpool_id, i, (int)nvtmpp_sys_va2pa(p_pool->first_blk->buf_addr + (p_pool->blk_total_size * i)), (int)p_pool->blk_size);
				vos_file_write(fd, (void *)tmp_buf, len);
			}
		}
	}
	#endif
	// private pools
	p_fboot_pool = nvtmpp_get_fastboot_pvpool_dts_info();
	for (i = 0;i < FBOOT_POOL_CNT; i++, p_fboot_pool++) {
		if (0 == i) {
			for (j = 0; j < NVTMPP_DDR_MAX; j ++) {
				p_first_pv_pool[j] = nvtmpp_vb_get_first_private_pool(j);
				if (p_first_pv_pool[j]) {
					blk_addr[j] = p_first_pv_pool[j]->first_blk->buf_addr;
				} else {
					blk_addr[j] = 0;
				}
			}
		} else {
			if (NULL != p_pool) {
				blk_addr[ddr_id] = nvtmpp_vb_arrange_fastboot_next_blk(blk_addr[ddr_id], p_pool->blk_size);
			}
		}
		p_pool = nvtmpp_vb_get_pool_by_name((CHAR *)p_fboot_pool->pool_name);
		DBG_IND("pool_name = %s, p_pool = 0x%x\r\n", (CHAR *)p_fboot_pool->pool_name, (int)p_pool);
		if (NULL == p_pool && (!p_fboot_pool->optional)) {
			goto dump_err;
		}
		if (p_pool) {
			ddr_id = p_pool->ddr;
			DBG_IND("ddr_id = %d \r\n", ddr_id);
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t%s = <0x%x 0x%x>;\r\n", p_fboot_pool->dts_node, (int)blk_addr[ddr_id], (int)p_pool->blk_size);
			vos_file_write(fd, (void *)tmp_buf, len);
		}
	}
	// dts end
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n};");
	vos_file_write(fd, (void *)tmp_buf, len);
	vos_file_close(fd);
	return TRUE;
dump_err:
	DBG_ERR("dump dtsi fail\r\n");
	vos_file_close(fd);
	return FALSE;

}
#endif

#if NVTMPP_TEST_CMD

static BOOL cmd_nvtmpp_init(unsigned char argc, char **argv)
{
	NVTMPP_ER        ret;
	NVTMPP_VB_CONF_S st_conf;

	memset(&st_conf, 0, sizeof(NVTMPP_VB_CONF_S));
	st_conf.max_pool_cnt = 64;
    st_conf.common_pool[0].blk_size = 1920 * 1080 * 3 / 2 + 1024;
	st_conf.common_pool[0].blk_cnt = 2;
	st_conf.common_pool[0].ddr = NVTMPP_DDR_1;
	st_conf.common_pool[0].type = POOL_TYPE_COMMON;
	st_conf.common_pool[1].blk_size = 864 * 480 * 3 / 2  + 1024;
	st_conf.common_pool[1].blk_cnt = 2;
	st_conf.common_pool[1].ddr = NVTMPP_DDR_1;
	st_conf.common_pool[1].type = POOL_TYPE_COMMON;
	st_conf.common_pool[2].blk_size = 2000;
	st_conf.common_pool[2].blk_cnt = 20;
	st_conf.common_pool[2].ddr = NVTMPP_DDR_1;
	st_conf.common_pool[2].type = POOL_TYPE_COMMON;
	ret = nvtmpp_vb_set_conf(&st_conf);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("nvtmpp set vb err: %d\r\n", ret);
		return TRUE;
	}
	ret = nvtmpp_vb_init();
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("nvtmpp init vb err: %d\r\n", ret);
		return TRUE;
	}
	return TRUE;
}

static BOOL cmd_nvtmpp_exit(unsigned char argc, char **argv)
{
	nvtmpp_vb_exit();
	return TRUE;
}
static BOOL cmd_nvtmpp_create_pool(unsigned char argc, char **argv)
{
	NVTMPP_VB_POOL pool;
	NVTMPP_VB_BLK  blk;
	UINT32         blk_size = 320 * 240 * 2;
	UINT32         blk_cnt =  5, va, pa;
	NVTMPP_ER      ret;
	BOOL           is_fill = TRUE;
	CHAR           module_str[NVTMPP_VB_MAX_POOL_NAME+1] = "module1";

	//sscanf_s(str_cmd, "%x %d %d %s", &blk_size, &blk_cnt, &is_fill, &module_str, sizeof(module_str));
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&blk_size);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&blk_cnt);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%d", (int *)&is_fill);
	}
	if (argc >= 4) {
		strncpy(module_str, argv[3], sizeof(module_str)-1);
		module_str[sizeof(module_str)-1] = 0;
	}
	/* create a video buffer pool*/
	pool = nvtmpp_vb_create_pool(module_str, blk_size, blk_cnt, NVTMPP_DDR_1);
	if (NVTMPP_VB_INVALID_POOL == pool) {
		DBG_ERR("create private pool err\r\n");
		return TRUE;
	}
	g_pool = pool;
	DBG_IND("create pool 0x%x, blk_size=0x%x, blk_cnt=%d\r\n", (int)pool, (int)blk_size, blk_cnt);
	DBG_DUMP("nvtmpp create pool 0x%x\r\n", (int)pool);
	/* get a buffer block from pool*/
	blk = nvtmpp_vb_get_block(g_module, pool, blk_size, NVTMPP_DDR_1);
	DBG_IND("blk 0x%x\r\n", (int)blk);
	DBG_DUMP("nvtmpp get blk %x\r\n", (int)blk);
	if (NVTMPP_VB_INVALID_BLK == blk) {
		DBG_ERR("get vb block err\r\n");
		if (nvtmpp_vb_destroy_pool(pool) != NVTMPP_ER_OK) {
			DBG_ERR("free pool\r\n");
		}
		g_pool = NVTMPP_VB_INVALID_POOL;
		return TRUE;
	}
	va = nvtmpp_vb_blk2va(blk);
	if (va == 0) {
		DBG_ERR("blk2va fail, blk=0x%x\r\n", (int)blk);
		return TRUE;
	}
	if (is_fill) {
		memset((void *)va, 0x11, blk_size);
	} else {
		if (blk_size > 32) {
			memset((void *)(va+blk_size-32), 0x11, 32);
		} else {
			memset((void *)va, 0x11, blk_size);
		}
	}
	pa = nvtmpp_vb_blk2pa(blk);
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk=0x%x\r\n", (int)blk);
		return TRUE;
	}
	if (nvtmpp_vb_va2blk(va) != blk) {
		DBG_ERR("va2blk fail, va =0x%x\r\n", (int)blk);
		return TRUE;
	}
	ret = nvtmpp_vb_unlock_block(g_module, blk);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("unlock blk fail %d\r\n", ret);
		return TRUE;
	}
	return TRUE;
}

static BOOL cmd_nvtmpp_destroy_pool(unsigned char argc, char **argv)
{
	NVTMPP_VB_POOL pool = g_pool;
	NVTMPP_ER      ret;

	//sscanf_s(str_cmd, "%x", &pool);
	if (argc >= 1) {
		sscanf(argv[0], "%x", (int *)&pool);
	}
	/* create a video buffer pool*/
	ret = nvtmpp_vb_destroy_pool(pool);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("destory pool 0x%x\r\n", (int)pool);
	}
	DBG_DUMP("nvtmpp destory pool %x\r\n", (int)pool);
	return TRUE;
}


static BOOL cmd_nvtmpp_get_block(unsigned char argc, char **argv)
{
	NVTMPP_MODULE  module = g_module;
	NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
	UINT32         blk_size = 1920 * 1080;
	NVTMPP_DDR     ddr = NVTMPP_DDR_1;
	NVTMPP_VB_BLK  blk;

	//sscanf_s(str_cmd, "%d %d %x %d", &module, &pool, &blk_size, &ddr);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&module);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&pool);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%x", (int *)&blk_size);
	}
	if (argc >= 4) {
		sscanf(argv[3], "%d", (int *)&ddr);
	}
	DBG_DUMP("module 0x%x, pool =0x%x, blk_size=0x%x\r\n", (int)module, (int)pool, (int)blk_size);
	blk = nvtmpp_vb_get_block(module, pool, blk_size, ddr);
	if (NVTMPP_VB_INVALID_BLK == blk) {
		DBG_ERR("ge blk fail\r\n");
		return TRUE;
	}
	g_blk = blk;
	return TRUE;
}

static BOOL cmd_nvtmpp_lock_block(unsigned char argc, char **argv)
{
	NVTMPP_ER          ret;
	NVTMPP_MODULE      module = g_module;

	//sscanf_s(str_cmd, "%d", &module);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&module);
	}
	ret = nvtmpp_vb_lock_block(module, g_blk);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("lock blk fail %d\r\n", ret);
		return TRUE;
	}
	return TRUE;
}

static BOOL cmd_nvtmpp_unlock_block(unsigned char argc, char **argv)
{
	NVTMPP_ER          ret;
	NVTMPP_MODULE      module = g_module;

	//sscanf_s(str_cmd, "%d", &module);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&module);
	}
	ret = nvtmpp_vb_unlock_block(module, g_blk);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("unlock blk fail %d\r\n", ret);
		return TRUE;
	}
	return TRUE;
}

static BOOL cmd_nvtmpp_get_max_free_size(unsigned char argc, char **argv)
{
	UINT32         max_free_block_size, ddr = NVTMPP_DDR_1;

	//sscanf_s(str_cmd, "%d", &ddr);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&ddr);
	}
	max_free_block_size = nvtmpp_vb_get_max_free_size(ddr);
	DBG_IND("max_free_block_size = 0x%x\r\n", (int)max_free_block_size);
	DBG_DUMP("nvtmpp max blk %x\r\n", (int)max_free_block_size);
	return 0;
}


static BOOL cmd_nvtmpp_add_module(unsigned char argc, char **argv)
{
	CHAR           module_str[NVTMPP_VB_MAX_POOL_NAME+1] = "module1";
	NVTMPP_MODULE  module;

	if (argc >= 1) {
		strncpy(module_str, argv[0], sizeof(module_str)-1);
		module_str[sizeof(module_str)-1] = 0;
	}
	module = MAKE_NVTMPP_MODULE(module_str[0], module_str[1], module_str[2], module_str[3],
		                        module_str[4], module_str[5], module_str[6], module_str[7]);
	nvtmpp_vb_add_module(module);
	return 0;
}
#endif


static SXCMD_BEGIN(nvtmpp_cmd_tbl, "nvtmpp")
SXCMD_ITEM("info",        cmd_nvtmpp_dump_info,         "dump nvtmpp info")
SXCMD_ITEM("dumpmax %",   cmd_nvtmpp_dump_max,          "dump when reach max buffer number, parm1 is pool_id, parm2 is max_cnt")
SXCMD_ITEM("showmsg %",   cmd_nvtmpp_showmsg,           "dump nvtmpp info when get block fail")
SXCMD_ITEM("chkmem_corrupt %",    cmd_nvtmpp_check_mem_corrupt,           "check if nvtmpp heap is corrupt")
#if defined(__LINUX)
SXCMD_ITEM("dumpstack %", cmd_nvtmpp_dump_stack,        "dumpstack with pid")
SXCMD_ITEM("memr %",      cmd_nvtmpp_mem_r,             "memory read")
SXCMD_ITEM("memdump %",   cmd_nvtmpp_mem_dump,          "memory dump to file")
#endif
#if NVTMPP_FASTBOOT_SUPPORT
SXCMD_ITEM("fastboot_mem %",   cmd_nvtmpp_fastboot_mem, "dump fastboot memory dtsi")
#endif
#if NVTMPP_TEST_CMD
SXCMD_ITEM("init %",      cmd_nvtmpp_init,              "init nvtmpp, no parameter")
SXCMD_ITEM("exit %",      cmd_nvtmpp_exit,              "exit nvtmpp, no parameter")
SXCMD_ITEM("crepool %",   cmd_nvtmpp_create_pool,       "create a private pool, parm1 is blk_size, parm2 is blk_cnt, parm3 is fill data to block, param4 is module string")
SXCMD_ITEM("despool %",   cmd_nvtmpp_destroy_pool,      "destroy a private pool, parm1 is pool handle")
SXCMD_ITEM("getblk %",    cmd_nvtmpp_get_block,         "get block, parm1 is 64bit module_id")
SXCMD_ITEM("locblk %",    cmd_nvtmpp_lock_block,        "lock block, parm1 is 64bit module_id")
SXCMD_ITEM("unlblk %",    cmd_nvtmpp_unlock_block,      "unlock block, parm1 is ddr_id")
SXCMD_ITEM("getmax %",    cmd_nvtmpp_get_max_free_size, "get max free size, parm1 is ddr_id")
SXCMD_ITEM("addmod %",    cmd_nvtmpp_add_module,        "add a new module name")
#endif
SXCMD_END()


void nvtmpp_install_cmd(void)
{
	#if defined __UITRON || defined __ECOS
	//SxCmd_AddTable(nvtmpp);
	#endif
}

void nvtmpp_uninstall_cmd(void)
{
	NVTMPP_CHK_MEM_CTRL  *p_ctrl;

	p_ctrl = &nvtmpp_check_mem_ctrl;
	if (TRUE == p_ctrl->start) {
		p_ctrl->start = FALSE;
		DBG_DUMP("stop check_mem_corrupt\r\n");
		THREAD_DESTROY(p_ctrl->thread);
	}
}

int nvtmpp_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(nvtmpp_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "nvtmpp");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", nvtmpp_cmd_tbl[loop].p_name, nvtmpp_cmd_tbl[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int nvtmpp_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(nvtmpp, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(nvtmpp_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	//DBG_DUMP("argc %d, argv[0] = %s, argv[1] = %s\r\n", argc, argv[0], argv[1]);
	if (strncmp(argv[1], "?", 2) == 0) {
		nvtmpp_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], nvtmpp_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = nvtmpp_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		nvtmpp_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}


