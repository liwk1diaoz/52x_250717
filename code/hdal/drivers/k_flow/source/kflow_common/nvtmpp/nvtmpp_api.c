/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2017/02/013
*/

#ifdef __KERNEL__
#include <linux/string.h>   // for memset
#else
#include <string.h>         // for memset, strncmp
#endif
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include "kflow_common/nvtmpp.h"
#include "nvtmpp_init.h"
#include "nvtmpp_int.h"
#include "nvtmpp_heap.h"
#include "nvtmpp_blk.h"
#include "nvtmpp_pool.h"
#include "nvtmpp_debug_cmd.h"
#include "nvtmpp_id.h"
#include "nvtmpp_log.h"
#include "nvtmpp_module.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_platform.h"


#if defined (DEBUG) && defined(__FREERTOS)
unsigned int nvtmpp_debug_level = THIS_DBGLVL;
#endif

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define NVTMPP_INITED_TAG       MAKEFOURCC('N', 'M', 'P', 'P') ///< a key value
#define NVTMPP_FREE_VA_MAX_NUM  512
#define NVTMPP_DYNAMIC_VA_DEBUG 0
#define NVTMPP_VA_MAP_MAX_NUM   50


typedef struct {
	NVTMPP_VB_POOL_S    *vb_pools;                                      ///< vb pools
	NVTMPP_VB_POOL_S    *vb_comm_pool_link_start[NVTMPP_DDR_MAX_NUM];   ///< vb common pools link list start pointer
	UINT32              showmsg_level;
	UINT32              fixed_pool_count;                               ///< fixed pools count
	UINT32              comm_pool_count;                                ///< vb common pools count
	BOOL                memory_corrupt_check_en;
	UINT32              fastboot_pool_count;
} NVTMPP_INFO_S;


typedef struct {
	BOOL                     start;
	UINT32                   sleep_ms;
	THREAD_HANDLE            thread;
	UINT32                   wait_free_va_list[NVTMPP_FREE_VA_MAX_NUM];
	UINT32                   in_idx;
	UINT32                   out_idx;
} NVTMPP_CHK_UNMAP_CTRL;


static NVTMPP_SYS_CONF_S g_sys_conf;                                ///< sys configure value
static UINT32            g_sys_init_tag;                            ///< sys init tag
static UINT32            g_vb_init_tag;                             ///< vb init tag
static UINT32            g_vb_conf_init_tag;                        ///< vb config init tag
static NVTMPP_VB_CONF_S  g_vb_conf;                                 ///< vb configure value
static NVTMPP_INFO_S     g_nvtmpp_info;
#if NVTMPP_FASTBOOT_SUPPORT
static BOOL              g_fastboot_checked;
#endif
/* codec rotate will read the address before 64 * line offset,
   so we need to reserve this area */
#define                  H26X_MAX_LINEOFFSET       4096
#define                  H26X_ROT_READ_BLINE       64
#define                  DDR2_RESERVED_START       0x40000000
//#define                  DDR2_RESERVED_END         0x40040000

#if NVTMPP_DYNAMIC_VA_DEBUG
typedef struct {
	UINT32                   va;
	UINT32                   pa;
	UINT32                   size;
} NVTMPP_VA_MAP_INFO;

NVTMPP_VA_MAP_INFO           va_map_list[NVTMPP_VA_MAP_MAX_NUM];

static void nvtmpp_vb_map_list_init_p(void);
static void nvtmpp_vb_map_list_add_p(NVTMPP_VA_MAP_INFO *map_info);
static void nvtmpp_vb_map_list_del_p(NVTMPP_VA_MAP_INFO *map_info);
static void nvtmpp_vb_map_list_dump_p(void);
#endif
static NVTMPP_CHK_UNMAP_CTRL  nvtmpp_check_unmap_ctrl;
static void *nvtmpp_vb_init_blk_map_va_p(NVTMPP_VB_BLK blk);


#if NVTMPP_DYNAMIC_VA_DEBUG
static void nvtmpp_vb_map_list_init_p(void)
{
	memset(&va_map_list[0], 0x00, sizeof(va_map_list));
}
static void nvtmpp_vb_map_list_add_p(NVTMPP_VA_MAP_INFO *map_info)
{
	UINT32 i;
	NVTMPP_VA_MAP_INFO *p_info;

	p_info = &va_map_list[0];
	for (i = 0;i < NVTMPP_VA_MAP_MAX_NUM; i++) {
		if (p_info->va == 0) {
			*p_info = *map_info;
			return;
		}
		p_info ++;
	}
	DBG_ERR("list full\r\n");
	nvtmpp_vb_map_list_dump_p();
}
static void nvtmpp_vb_map_list_del_p(NVTMPP_VA_MAP_INFO *map_info)
{
	UINT32 i;
	NVTMPP_VA_MAP_INFO *p_info;

	p_info = &va_map_list[0];
	for (i = 0;i < NVTMPP_VA_MAP_MAX_NUM; i++) {
		if (p_info->va == map_info->va && p_info->size == map_info->size) {
			memset(p_info, 0x00, sizeof(NVTMPP_VA_MAP_INFO));
			return;
		}
		p_info ++;
	}
	DBG_ERR("Not found va 0x%x, pa 0x%x, size 0x%x\r\n", map_info->va, map_info->pa, map_info->size);
	nvtmpp_vb_map_list_dump_p();
}
static void nvtmpp_vb_map_list_dump_p(void)
{
	UINT32 i;
	NVTMPP_VA_MAP_INFO *p_info;

	p_info = &va_map_list[0];
	for (i = 0; i < NVTMPP_VA_MAP_MAX_NUM; i++) {
		DBG_DUMP("list %d va 0x%x, pa 0x%x, size 0x%x\r\n",
				i, p_info->va, p_info->pa, p_info->size);
		p_info ++;
	}
	panic("Can not find pa\r\n");
}

static void nvtmpp_vb_map_list_dump(void)
{
	unsigned long    flags = 0;

	loc_cpu(flags);
	nvtmpp_vb_map_list_dump_p();
	unl_cpu(flags);
}
#endif


static void nvtmpp_lock_sem_p(void)
{
	SEM_WAIT(NVTMPP_SEM_ID);
}

static void nvtmpp_unlock_sem_p(void)
{
	SEM_SIGNAL(NVTMPP_SEM_ID);
}

static INT32 nvtmpp_vb_check_duplicate_size_p(const NVT_VB_CPOOL_S common_pool[NVTMPP_VB_MAX_COMM_POOLS])
{
	const NVT_VB_CPOOL_S *p_pool_i, *p_pool_j;
	UINT32               pool_i, pool_j;

	for (pool_i = 0; pool_i < NVTMPP_VB_MAX_COMM_POOLS; pool_i++) {
		p_pool_i = &common_pool[pool_i];
		if (0 == p_pool_i->blk_size || 0 == p_pool_i->blk_cnt) {
			continue;
		}
		for (pool_j = pool_i+1; pool_j < NVTMPP_VB_MAX_COMM_POOLS; pool_j++) {
			p_pool_j = &common_pool[pool_j];
			if (0 == p_pool_j->blk_size || 0 == p_pool_j->blk_cnt) {
				continue;
			}
			if (p_pool_i->type != POOL_TYPE_COMMON && p_pool_i->type == p_pool_j->type && p_pool_i->blk_size == p_pool_j->blk_size
				&& p_pool_i->ddr == p_pool_j->ddr){
				DBG_ERR("Same pooltype(0x%x) and blk size(0x%x) setted", p_pool_i->type, p_pool_i->blk_size);
				return -1;
			}
		}
	}
	return 0;
}


static INT32 nvtmpp_vb_init_common_pools_p(NVT_VB_CPOOL_S common_pool[NVTMPP_VB_MAX_COMM_POOLS])
{
	NVT_VB_CPOOL_S      *p_cfgpool;
	UINT32               pool_id = 0;
	NVTMPP_VB_POOL_S     *p_pool;
	UINT32               vcap_fixed_map_poolcnt;

	if (nvtmpp_vb_check_duplicate_size_p(common_pool) < 0) {
		return -1;
	}
	vcap_fixed_map_poolcnt = nvtmpp_get_vcap_fixed_map_poolcnt();
	nvtmpp_lock_sem_p();
	p_pool = &g_nvtmpp_info.vb_pools[g_nvtmpp_info.fixed_pool_count];
	for (pool_id = 0; pool_id < NVTMPP_VB_MAX_COMM_POOLS; pool_id++, p_pool++) {
		p_cfgpool = &common_pool[pool_id];
		// overwrite vcap fixed mapping pool pool_type to NVTMPP_VB_VCAP_OUT_POOL
		if (pool_id < vcap_fixed_map_poolcnt) {
			p_cfgpool->type = NVTMPP_VB_VCAP_OUT_POOL;
		}
		if (p_cfgpool->blk_size && p_cfgpool->blk_cnt) {

			if (nvtmpp_vb_pool_init(p_pool, NULL, p_cfgpool->blk_size, p_cfgpool->blk_cnt, p_cfgpool->ddr, 1, p_cfgpool->type) < 0) {
				DBG_ERR("Not enough buffer\r\n");
				goto init_comm_pools_err;
			}
			if (g_nvtmpp_info.vb_comm_pool_link_start[p_cfgpool->ddr] == NULL && p_cfgpool->type == POOL_TYPE_COMMON) {
				g_nvtmpp_info.vb_comm_pool_link_start[p_cfgpool->ddr] = p_pool;
			} else if (p_cfgpool->type == POOL_TYPE_COMMON){
				nvtmpp_vb_pool_insert_node_to_common(&g_nvtmpp_info.vb_comm_pool_link_start[p_cfgpool->ddr], p_pool);
			}
		} else {
			break;
		}
	}
	g_nvtmpp_info.comm_pool_count = pool_id;
	nvtmpp_unlock_sem_p();
	return 0;
init_comm_pools_err:
	DBG_ERR("init pool_id %d fail\r\n", pool_id);
	nvtmpp_unlock_sem_p();
	return -1;
}

#if NVTMPP_FASTBOOT_SUPPORT
static INT32  nvtmpp_sys_pa2ddrid(UINT32 phys_addr)
{
	UINT32 i;

	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		if (g_sys_conf.ddr_mem[i].phys_addr > 0 && phys_addr >= g_sys_conf.ddr_mem[i].phys_addr &&
			phys_addr < g_sys_conf.ddr_mem[i].phys_addr+g_sys_conf.ddr_mem[i].size) {
			return i;
		}
	}
	return -1;
}


NVTMPP_VB_POOL_S *nvtmpp_vb_get_common_pool(UINT32 id, BOOL is_pool_type_common)
{
	NVTMPP_VB_POOL_S *p_pool;
	int              pool_id;

	pool_id = g_nvtmpp_info.fixed_pool_count;
	if (pool_id + id > g_sys_conf.max_pools_cnt) {
		return NULL;
	}
	p_pool = &g_nvtmpp_info.vb_pools[pool_id+id];
	if (p_pool->init_tag != NVTMPP_INITED_TAG || p_pool->is_common == 0) {
		return NULL;
	}
	if (is_pool_type_common && p_pool->pool_type != POOL_TYPE_COMMON) {
		return NULL;
	}
	if (!is_pool_type_common && p_pool->pool_type == POOL_TYPE_COMMON) {
		return NULL;
	}
	return p_pool;
}

static int nvtmpp_fastboot_lock_blk_cb(UINT32 blk_addr)
{
	NVTMPP_MODULE      module = MAKE_NVTMPP_MODULE('f', 'b', 'o', 'o', 't', 0, 0, 0);

	return nvtmpp_vb_lock_block(module, (NVTMPP_VB_BLK)(blk_addr - nvtmpp_vb_get_blk_header_size()));
}

static int nvtmpp_fastboot_unlock_blk_cb(UINT32 blk_addr)
{
	NVTMPP_MODULE      module = MAKE_NVTMPP_MODULE('f', 'b', 'o', 'o', 't', 0, 0, 0);

	return nvtmpp_vb_unlock_block(module, (NVTMPP_VB_BLK)(blk_addr - nvtmpp_vb_get_blk_header_size()));
}

static int nvtmpp_vb_init_fastboot_blks_p(NVTMPP_FASTBOOT_MEM_S *p_mem)
{
	NVTMPP_MODULE      module = MAKE_NVTMPP_MODULE('f', 'b', 'o', 'o', 't', 0, 0, 0);
	INT32              module_id, i;
	NVTMPP_VB_POOL_S  *p_pool;
	#if 1
	NVTMPP_VB_BLK      comn_blk[FBOOT_COMNBLK_MAX_CNT];
	UINT32             pool_id, blk_cnt;
	#else
	INT32              path_id;
	NVTMPP_VB_BLK      vcap_blk[FBOOT_VCAP_COMNBLK_MAX_CNT];
	NVTMPP_VB_BLK      vprc_blk[FBOOT_VPRC_COMNBLK_MAX_CNT];
	#endif
	NVTMPP_ER          ret;

	nvtmpp_vb_add_module(module);
	module_id = nvtmpp_vb_module_to_index(module);
	if (module_id < 0) {
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	#if 1
	pool_id = 0;
	blk_cnt = 0;
	for (i = 0; i < FBOOT_COMNBLK_MAX_CNT && i < p_mem->comn_blk_cnt; i++) {
		if (blk_cnt == 0) {
			p_pool = nvtmpp_vb_get_common_pool(pool_id, TRUE);
			if (NULL == p_pool) {
				goto fastboot_comn_blk_err;
			}
			pool_id ++;
			blk_cnt = p_pool->blk_cnt;
		}
		comn_blk[i] = nvtmpp_vb_get_free_block_from_pool(p_pool, p_mem->comn_blk[i].size);
		if (comn_blk[i] != (NVTMPP_VB_BLK)(p_mem->comn_blk[i].addr - nvtmpp_vb_get_blk_header_size())) {
			DBG_ERR("normal blk = 0x%x, fastboot blk = 0x%x", (int)comn_blk[i], (int)(p_mem->comn_blk[i].addr - nvtmpp_vb_get_blk_header_size()));
			goto fastboot_comn_blk_err;
		}
		// reference count initialize to be 1
		ret = nvtmpp_vb_add_blk_ref(module_id, comn_blk[i]);
		if (ret != NVTMPP_ER_OK) {
			goto fastboot_comn_blk_err;
		}
		blk_cnt --;
	}
	for (i = 0; i < FBOOT_COMNBLK_MAX_CNT && i < p_mem->comn_blk_cnt; i++) {
		if (p_mem->comn_blk[i].ref_cnt == 0) {
			nvtmpp_vb_unlock_block(module, comn_blk[i]);
		} else if (p_mem->comn_blk[i].ref_cnt == 2) {
			nvtmpp_vb_lock_block(module, comn_blk[i]);
		}
	}
	#else
	// vcap blk;
	p_pool = nvtmpp_vb_get_common_pool(0, TRUE);
	if (NULL == p_pool) {
		goto fastboot_vcap_blk_err;
	}
	for (i = 0; i < FBOOT_VCAP_COMNBLK_MAX_CNT && i < p_mem->vcap_blk_cnt; i++) {
		vcap_blk[i] = nvtmpp_vb_get_free_block_from_pool(p_pool, p_mem->vcap_blk[i].size);
		if (vcap_blk[i] != (NVTMPP_VB_BLK)(p_mem->vcap_blk[i].addr - nvtmpp_vb_get_blk_header_size())) {
			goto fastboot_vcap_blk_err;
		}
		// reference count initialize to be 1
		ret = nvtmpp_vb_add_blk_ref(module_id, vcap_blk[i]);
		if (ret != NVTMPP_ER_OK) {
			goto fastboot_vcap_blk_err;
		}
	}
	for (i = 0; i < FBOOT_VCAP_COMNBLK_MAX_CNT && i < p_mem->vcap_blk_cnt; i++) {
		if (p_mem->vcap_blk[i].ref_cnt == 0) {
			nvtmpp_vb_unlock_block(module, vcap_blk[i]);
		} else if (p_mem->vcap_blk[i].ref_cnt == 2) {
			nvtmpp_vb_lock_block(module, vcap_blk[i]);
		}
	}
	// vprc blk;
	for (path_id = 0; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
		if (!p_mem->vprc_blk_cnt[path_id])
			break;
		p_pool = nvtmpp_vb_get_common_pool(path_id+1, TRUE);
		if (NULL == p_pool) {
			goto fastboot_vprc_blk_err;
		}
		for (i = 0; i < FBOOT_VPRC_COMNBLK_MAX_CNT && i < p_mem->vprc_blk_cnt[path_id]; i++) {
			vprc_blk[i] = nvtmpp_vb_get_free_block_from_pool(p_pool, p_mem->vprc_blk[path_id][i].size);
			if (vprc_blk[i] != (NVTMPP_VB_BLK)(p_mem->vprc_blk[path_id][i].addr - nvtmpp_vb_get_blk_header_size())) {
				goto fastboot_vprc_blk_err;
			}
			// reference count initialize to be 1
			ret = nvtmpp_vb_add_blk_ref(module_id, vprc_blk[i]);
			if (ret != NVTMPP_ER_OK) {
				goto fastboot_vprc_blk_err;
			}
		}
		for (i = 0; i < FBOOT_VPRC_COMNBLK_MAX_CNT && i < p_mem->vprc_blk_cnt[path_id]; i++) {
			if (p_mem->vprc_blk[path_id][i].ref_cnt == 0) {
				nvtmpp_vb_unlock_block(module, vprc_blk[i]);
			} else if (p_mem->vprc_blk[path_id][i].ref_cnt == 2) {
				nvtmpp_vb_lock_block(module, vprc_blk[i]);
			}
		}
	}
	#endif
	nvtmpp_reg_fastboot_lock_cb(nvtmpp_fastboot_lock_blk_cb);
	nvtmpp_reg_fastboot_unlock_cb(nvtmpp_fastboot_unlock_blk_cb);
	return NVTMPP_ER_OK;

#if 1
fastboot_comn_blk_err:
	DBG_ERR("comn blk mismatch\r\n");
	return NVTMPP_ER_BLK_UNEXIST;
#else
fastboot_vcap_blk_err:
	DBG_ERR("vcap blk mismatch\r\n");
	return NVTMPP_ER_BLK_UNEXIST;
fastboot_vprc_blk_err:
	DBG_ERR("vprc blk mismatch\r\n");
	return NVTMPP_ER_BLK_UNEXIST;
#endif
}


static INT32 nvtmpp_vb_init_fastboot_pools_p(void)
{
	NVTMPP_VB_POOL_S     *p_pool;
	NVTMPP_VB_POOL        pool;
	NVTMPP_FASTBOOT_MEM_S *p_fastboot_mem = NULL;
	unsigned long         flags;
	NVTMPP_FBOOT_POOL_DTS_INFO_S *p_fboot_pool;
	UINT32                i;
	INT32                 ddr_id;

	// only need to check once
	if (g_fastboot_checked == TRUE) {
		return 0;
	} else {
		g_fastboot_checked = TRUE;
	}
	p_fastboot_mem = nvtmpp_get_fastboot_mem();
	if (p_fastboot_mem == NULL) {
		return 0;
	}
	flags = nvtmpp_fastboot_spin_lock();
	if (nvtmpp_vb_init_fastboot_blks_p(p_fastboot_mem) < 0) {
		nvtmpp_fastboot_spin_unlock(flags);
		goto init_fastboot_pools_err;
	}
	nvtmpp_fastboot_spin_unlock(flags);
	p_fboot_pool = nvtmpp_get_fastboot_pvpool_dts_info();
	for (i = 0; i < FBOOT_POOL_CNT; i++) {

		if (p_fastboot_mem->pv_pools[i].size && p_fastboot_mem->pv_pools[i].pa) {
			ddr_id = nvtmpp_sys_pa2ddrid(p_fastboot_mem->pv_pools[i].pa);
			if (ddr_id < 0) {
				DBG_ERR("Invalid addr 0x%x\r\n", p_fastboot_mem->pv_pools[i].pa);
				goto init_fastboot_pools_err;
			}
			pool = nvtmpp_vb_create_pool((CHAR *)p_fboot_pool->pool_name, p_fastboot_mem->pv_pools[i].size, 1, ddr_id);
			if (NVTMPP_VB_INVALID_POOL == pool) {
				goto init_fastboot_pools_err;
			}
			p_pool = (NVTMPP_VB_POOL_S *)pool;
			if (p_pool->first_blk->buf_addr != p_fastboot_mem->pv_pools[i].pa) {
				DBG_ERR("%s mismatch, blk=0x%x, dts= 0x%x\r\n", p_fboot_pool->pool_name, p_pool->first_blk->buf_addr, p_fastboot_mem->pv_pools[i].pa);
				goto init_fastboot_pools_err;
			}
			g_nvtmpp_info.fastboot_pool_count++;
		}
		p_fboot_pool++;
	}
	return 0;
init_fastboot_pools_err:
	return -1;
}
#endif


static INT32 nvtmpp_vb_force_reset_all_pools_p(void)
{
	UINT32 i;
	NVTMPP_VB_POOL_S *p_pool;

	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		NVTMPP_MEMINFO_S  ddr_mem;
		UINT32            ddr2_resv_end;

		ddr2_resv_end = DDR2_RESERVED_START + g_sys_conf.max_yuv_lineoffset * H26X_ROT_READ_BLINE;
		DBG_IND("max_yuv_lineoffset = %d, ddr2_resv_end = 0x%x\r\n", g_sys_conf.max_yuv_lineoffset, ddr2_resv_end);
		ddr_mem = g_sys_conf.ddr_mem[i];
		if (ddr_mem.phys_addr >= DDR2_RESERVED_START && ddr_mem.phys_addr < ddr2_resv_end) {
			UINT32 offset;

			offset = ddr2_resv_end - ddr_mem.phys_addr;
			ddr_mem.phys_addr += offset;
			if (ddr_mem.virt_addr) {
				ddr_mem.virt_addr += offset;
			}
			if (ddr_mem.size >= offset) {
				ddr_mem.size -= offset;
			} else {
				ddr_mem.size = 0;
			}
		}
		if (ddr_mem.size && nvtmpp_heap_init(i, &ddr_mem, g_sys_conf.max_pools_cnt) < 0) {
			nvtmpp_unlock_sem_p();
			return NVTMPP_ER_PARM;
		}
	}
	p_pool = g_nvtmpp_info.vb_pools;
	for (i = 0; i < g_sys_conf.max_pools_cnt; i++) {
		p_pool->init_tag = 0;
		p_pool++;
		//DBG_DUMP("p_pool = 0x%x\r\n", (int)p_pool);
	}
	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		g_nvtmpp_info.vb_comm_pool_link_start[i] = NULL;
	}
	g_nvtmpp_info.comm_pool_count = 0;
	return NVTMPP_ER_OK;
}
static INT32 nvtmpp_vb_free_old_pools_p(void)
{
	UINT32               pool_id;
	NVTMPP_VB_POOL_S     *p_pool;
	UINT32               i;
	INT32                ret;

	nvtmpp_lock_sem_p();
	p_pool = &g_nvtmpp_info.vb_pools[g_nvtmpp_info.fixed_pool_count];
	for (pool_id = 0; pool_id < g_sys_conf.max_pools_cnt - g_nvtmpp_info.fixed_pool_count; pool_id++, p_pool++) {
		if (p_pool->init_tag == NVTMPP_INITED_TAG) {
			ret = nvtmpp_vb_pool_exit(p_pool);
			if (ret < 0 && g_nvtmpp_info.fixed_pool_count == 0 && nvtmpp_is_dynamic_map() == 0) {
				goto force_reset_all_pools;
			}
		}
	}
	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		g_nvtmpp_info.vb_comm_pool_link_start[i] = NULL;
	}
	g_nvtmpp_info.comm_pool_count = 0;
	nvtmpp_unlock_sem_p();
	return 0;
force_reset_all_pools:
	DBG_DUMP("\r\n Free pool err !! force_reset_all_pools !!\r\n");
	nvtmpp_vb_force_reset_all_pools_p();
	nvtmpp_unlock_sem_p();
	return 0;
}
static void  nvtmpp_dump_pub_info_p(int (*dump)(const char *fmt, ...))
{
	char   module_string[(sizeof(NVTMPP_MODULE) + 1)*NVTMPP_MODULE_MAX_NUM + 1];
	UINT32 module_strlen_list[NVTMPP_MODULE_MAX_NUM];
	UINT32 i;

	dump("-------------------------VB PUB INFO ----------------------------------------------------------\r\n");
	dump("Max Count of pools: %d\r\n", g_sys_conf.max_pools_cnt);
	dump("Is dynamic map: %d\r\n", nvtmpp_is_dynamic_map());
	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		if (g_sys_conf.ddr_mem[i].size != 0) {
		dump("DDR%d: paddr = 0x%08X, vaddr = 0x%08X, size = 0x%08x, Free = 0x%08X, MaxFreeBlk = 0x%08X\r\n",
			(int)i+1,
			(int)g_sys_conf.ddr_mem[i].phys_addr,
			(int)g_sys_conf.ddr_mem[i].virt_addr,
			(int)g_sys_conf.ddr_mem[i].size,
			(int)nvtmpp_heap_get_free_size(i),
			(int)nvtmpp_heap_get_max_free_block_size(i)
			);
		}
	}

	nvtmpp_vb_get_all_modules_list_string(module_string, sizeof(module_string), module_strlen_list, FALSE);
	dump("Modules: %s\r\n", module_string);
}

static void  nvtmpp_dump_fixed_pools_p(int (*dump)(const char *fmt, ...))
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i;

	if (0 == g_nvtmpp_info.fixed_pool_count)
		return;
	dump("\r\n-------------------------FIXED POOL ---------------------------------------------------------\r\n");

	dump("PoolName              DDR     PhyAddr      VirAddr    BlkSize\r\n");

	p_pool = &g_nvtmpp_info.vb_pools[0];
	for (i = 0; i < (int)g_nvtmpp_info.fixed_pool_count; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			break;
		}
		if (p_pool->pool_type != POOL_TYPE_FIXED) {
			break;
		}
		dump("%-23s %d  0x%08X   0x%08X   0x%08X \r\n", p_pool->pool_name, p_pool->ddr + 1,
				(int)p_pool->pool_addr, (int)p_pool->pool_va, (int)p_pool->blk_size);
	}
}

static void  nvtmpp_dump_common_pools_p(int (*dump)(const char *fmt, ...))
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i, pool_id;
	UINT32             loop_count;
	char               module_string[(sizeof(NVTMPP_MODULE) + 1)*NVTMPP_MODULE_MAX_NUM + 1];
	UINT32             module_strlen_list[NVTMPP_MODULE_MAX_NUM] = {0};
	UINT32             module_count;
	NVTMPP_VB_BLK_S    *p_blk;



	dump("\r\n");
	dump("-------------------------COMMON POOL ----------------------------------------------------------\r\n");

	module_count = nvtmpp_vb_get_module_count();
	nvtmpp_vb_get_all_modules_list_string(module_string, sizeof(module_string), module_strlen_list, FALSE);
	pool_id = g_nvtmpp_info.fixed_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < NVTMPP_VB_MAX_COMM_POOLS; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG || p_pool->is_common == 0) {
			break;
		}
		dump("PoolId     PoolType   DDR     PhyAddr      VirAddr      BlkSize     BlkCnt  Free  MinFree\r\n");
		nvtmpp_log_open(dump);
		nvtmpp_log_save_str("%5d          0x%02X     %d  0x%08X   0x%08X   0x%08X       %2d    %2ld       %2ld\r\n",
							i, p_pool->pool_type , p_pool->ddr + 1, (int)p_pool->pool_addr,
							(int)p_pool->pool_va, (int)p_pool->blk_size, p_pool->blk_cnt,
							p_pool->blk_free_cnt, p_pool->blk_min_free_cnt);
		if (p_pool->first_used_blk == NULL) {
			nvtmpp_log_dump();
			nvtmpp_log_close();
			continue;
		}
		p_blk = p_pool->first_used_blk;
		nvtmpp_log_save_str("     BlkId      BlkHdl      BlkAddr     WantSize   %s\r\n", module_string);
		loop_count = 0;
		while (p_blk && nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)p_blk)) {
			if (p_blk->total_ref_cnt != 0) {
				nvtmpp_vb_dump_blk_ref(nvtmpp_log_save_str, (NVTMPP_VB_BLK)p_blk, module_count, module_strlen_list);
			}
			p_blk = p_blk->next;
			if (loop_count++ > NVTMPP_VB_MAX_BLK_EACH_POOL) {
				DBG_ERR("link list error\r\n");
			}
		}
		nvtmpp_log_save_str("\r\n");
		nvtmpp_log_dump();
		nvtmpp_log_close();
	}
}
static void  nvtmpp_dump_private_pools_p(int (*dump)(const char *fmt, ...), BOOL is_ignore_temp)
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i, pool_id;
	UINT32             loop_count, need_dump_count;
	char               module_string[(sizeof(NVTMPP_MODULE) + 1)*NVTMPP_MODULE_MAX_NUM + 1];
	UINT32             module_strlen_list[NVTMPP_MODULE_MAX_NUM] = {0};
	UINT32             module_count;
	NVTMPP_VB_BLK_S    *p_blk;


	dump("\r\n-------------------------PRIVATE POOL ---------------------------------------------------------\r\n");
	dump("PoolName              DDR     PhyAddr      VirAddr    BlkSize     BlkCnt  Free  MinFree\r\n");
	module_count = nvtmpp_vb_get_module_count();
	nvtmpp_vb_get_all_modules_list_string(module_string, sizeof(module_string), module_strlen_list, FALSE);
	pool_id = g_nvtmpp_info.fixed_pool_count + g_nvtmpp_info.comm_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < (int)g_sys_conf.max_pools_cnt - pool_id; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		if (!p_pool->is_common) {
			if (is_ignore_temp &&
				strncmp(p_pool->pool_name, NVTMPP_TEMP_POOL_NAME, strlen(NVTMPP_TEMP_POOL_NAME)) == 0) {
				continue;
			}
			/*
			dump("%-23s %d  0x%08X   0x%08X   0x%08X       %2ld    %2ld       %2ld\r\n",
				p_pool->pool_name, p_pool->ddr + 1, (int)nvtmpp_sys_va2pa(p_pool->pool_addr),
				(int)p_pool->pool_addr, (int)p_pool->blk_size, p_pool->blk_cnt, p_pool->blk_free_cnt,
				p_pool->blk_min_free_cnt);
			*/
			dump("%-23s %d  0x%08X   0x%08X   0x%08X       %2ld    %2ld       %2ld\r\n",
					p_pool->pool_name, p_pool->ddr + 1, (int)p_pool->pool_addr, (int)p_pool->pool_va,
					(int)p_pool->blk_size, p_pool->blk_cnt, p_pool->blk_free_cnt,
					p_pool->blk_min_free_cnt);
			if (p_pool->first_used_blk == NULL) {
				nvtmpp_log_dump();
				continue;
			}
			p_blk = p_pool->first_used_blk;
			need_dump_count = 0;
			while (p_blk && nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)p_blk)) {
				if (p_blk->total_ref_cnt != 0 && p_blk->max_ref_cnt > 1) {
					need_dump_count++;
				}
				p_blk = p_blk->next;
			}
			if (!need_dump_count) {
				continue;
			}
			nvtmpp_log_open(dump);
			nvtmpp_log_save_str("     BlkId      BlkHdl      BlkAddr     WantSize   %s\r\n", module_string);
			p_blk = p_pool->first_used_blk;
			loop_count = 0;
			while (p_blk && nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)p_blk)) {
				if (p_blk->total_ref_cnt != 0 && p_blk->max_ref_cnt > 1) {
					nvtmpp_vb_dump_blk_ref(nvtmpp_log_save_str, (NVTMPP_VB_BLK)p_blk, module_count, module_strlen_list);
				}
				p_blk = p_blk->next;
				if (loop_count++ > NVTMPP_VB_MAX_BLK_EACH_POOL) {
					DBG_ERR("link list error\r\n");
				}
			}
			nvtmpp_log_save_str("\r\n");
			nvtmpp_log_dump();
			nvtmpp_log_close();
		}
	}
}

NVTMPP_VB_POOL_S *nvtmpp_vb_get_pool_by_mem_range_p(UINT32 mem_start, UINT32 mem_end)
{
	NVTMPP_VB_POOL_S *p_pool;
	UINT32            i;

	//DBG_DUMP("mem_start = 0x%x, mem_end = 0x%x\r\n", mem_start, mem_end);
	for (i = 0; i < g_sys_conf.max_pools_cnt; i++) {
		p_pool = &g_nvtmpp_info.vb_pools[i];
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		//DBG_DUMP("p_pool->pool_addr = 0x%x, pool_end = 0x%x\r\n", p_pool->pool_addr, p_pool->pool_addr+p_pool->pool_size);
		if (p_pool->pool_addr <= mem_start && (p_pool->pool_addr+p_pool->pool_size) >= mem_end)
			return p_pool;
	}
	return NULL;
}

static void nvtmpp_vb_chk_dump_memory_not_freed_p(void)
{
	NVTMPP_VB_POOL_S *p_pool;
	int              i, pool_id;
	BOOL             has_comm_buffer_not_free = FALSE;
	BOOL             has_priv_buffer_not_free = FALSE;

	pool_id = g_nvtmpp_info.fixed_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < NVTMPP_VB_MAX_COMM_POOLS; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG || p_pool->is_common == 0) {
			break;
		}
		if (p_pool->blk_cnt != p_pool->blk_free_cnt) {
			has_comm_buffer_not_free = TRUE;
			break;
		}
	}
	pool_id = g_nvtmpp_info.fixed_pool_count + g_nvtmpp_info.comm_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < (int)g_sys_conf.max_pools_cnt - pool_id; i++, p_pool++) {
		if (!p_pool->is_common && p_pool->init_tag == NVTMPP_INITED_TAG) {
			if (strncmp(p_pool->pool_name, NVTMPP_TEMP_POOL_NAME, strlen(NVTMPP_TEMP_POOL_NAME))) {
				has_priv_buffer_not_free = TRUE;
				break;
			}
		}
	}
	if (has_comm_buffer_not_free) {
		vk_printk("\033[33m WRN: nvtmpp common buffer not all freed !!!!");
		nvtmpp_dump_common_pools_p(vk_printk);
		vk_printk("\033[0m");
	}
	if (has_priv_buffer_not_free) {
		vk_printk("\033[33m WRN: nvtmpp private buffer not all freed !!!!");
		nvtmpp_dump_private_pools_p(vk_printk, TRUE);
		vk_printk("\033[0m");
	}
}

#if NVTMPP_FASTBOOT_SUPPORT
NVTMPP_VB_POOL_S *nvtmpp_vb_chk_dup_with_fastboot_pool(char *name)
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i, pool_id;

	pool_id = g_nvtmpp_info.fixed_pool_count + g_nvtmpp_info.comm_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < g_nvtmpp_info.fastboot_pool_count; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		if (!p_pool->is_common) {
			if (strncmp(p_pool->pool_name, name, strlen(p_pool->pool_name))== 0) {
				return p_pool;
			}
		}
	}
	return NULL;
}
#endif

NVTMPP_ER      nvtmpp_sys_init(NVTMPP_SYS_CONF_S *p_sys_conf)
{
	UINT32 i;
	NVTMPP_VB_POOL_S *p_pool;


	#if defined __UITRON || defined __ECOS
	if (!NVTMPP_SEM_ID) {
		DBG_ERR("ID not install\r\n");
		return NVTMPP_ER_RESID_NOT_INSTALL;
	}
	#endif
	if (p_sys_conf == NULL) {
		return NVTMPP_ER_PARM;
	}
	if (g_sys_init_tag == NVTMPP_INITED_TAG) {
		return NVTMPP_ER_OK;
	}
	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		DBG_IND("ddr%d virt_addr=0x%08X, phys_addr=0x%08X, size=0x%08X\r\n",
			 (int)i+1,
		     (int)p_sys_conf->ddr_mem[i].virt_addr,
		     (int)p_sys_conf->ddr_mem[i].phys_addr,
		     (int)p_sys_conf->ddr_mem[i].size);

	}
	nvtmpp_lock_sem_p();
	g_sys_conf = *p_sys_conf;
	if (g_sys_conf.max_pools_cnt == 0) {
		g_sys_conf.max_pools_cnt = NVTMPP_VB_DEF_POOLS_CNT;
	}
	if (g_sys_conf.max_pools_cnt > NVTMPP_VB_MAX_POOLS) {
		DBG_ERR("max_pools_cnt %d exceeds limit %d \r\n", g_sys_conf.max_pools_cnt, NVTMPP_VB_MAX_POOLS);
		nvtmpp_unlock_sem_p();
		return NVTMPP_ER_PARM;
	}
	if (g_sys_conf.max_yuv_lineoffset == 0) {
		g_sys_conf.max_yuv_lineoffset = H26X_MAX_LINEOFFSET;
	}
	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		NVTMPP_MEMINFO_S  ddr_mem;
		UINT32            ddr2_resv_end;

		ddr2_resv_end = DDR2_RESERVED_START + g_sys_conf.max_yuv_lineoffset * H26X_ROT_READ_BLINE;
		DBG_IND("max_yuv_lineoffset = %d, ddr2_resv_end = 0x%x\r\n", g_sys_conf.max_yuv_lineoffset, ddr2_resv_end);
		ddr_mem = p_sys_conf->ddr_mem[i];
		if (ddr_mem.phys_addr >= DDR2_RESERVED_START && ddr_mem.phys_addr < ddr2_resv_end) {
			UINT32 offset;

			offset = ddr2_resv_end - ddr_mem.phys_addr;
			ddr_mem.phys_addr += offset;
			if (ddr_mem.virt_addr) {
				ddr_mem.virt_addr += offset;
			}
			if (ddr_mem.size >= offset) {
				ddr_mem.size -= offset;
			} else {
				ddr_mem.size = 0;
			}
		}
		if (ddr_mem.size && nvtmpp_heap_init(i, &ddr_mem, g_sys_conf.max_pools_cnt) < 0) {
			nvtmpp_unlock_sem_p();
			return NVTMPP_ER_PARM;
		}
	}
	memset(&g_nvtmpp_info, 0, sizeof(g_nvtmpp_info));
	g_nvtmpp_info.vb_pools = nvtmpp_vmalloc(g_sys_conf.max_pools_cnt * sizeof(NVTMPP_VB_POOL_S));
	DBG_IND("vmalloc size = 0x%x, addr = 0x%x\r\n", g_sys_conf.max_pools_cnt * sizeof(NVTMPP_VB_POOL_S),
			(int)g_nvtmpp_info.vb_pools);
	if (g_nvtmpp_info.vb_pools == NULL) {
		DBG_ERR("allocate pools buffer fail\r\n");
		nvtmpp_unlock_sem_p();
		return NVTMPP_ER_NOBUF;
	}
	// clear all pools init flag
	p_pool = g_nvtmpp_info.vb_pools;
	for (i = 0; i < g_sys_conf.max_pools_cnt; i++) {
		p_pool->init_tag = 0;
		p_pool++;
		//DBG_DUMP("p_pool = 0x%x\r\n", (int)p_pool);
	}
	//memset(g_nvtmpp_info.vb_pools, 0, g_sys_conf.max_pools_cnt * sizeof(NVTMPP_VB_POOL_S));
	nvtmpp_vb_pool_set_struct_range((UINT32)g_nvtmpp_info.vb_pools, g_sys_conf.max_pools_cnt * sizeof(NVTMPP_VB_POOL_S));
	g_sys_init_tag = NVTMPP_INITED_TAG;
	nvtmpp_unlock_sem_p();
	return NVTMPP_ER_OK;
}

void nvtmpp_sys_exit(void)
{
	if (g_sys_init_tag != NVTMPP_INITED_TAG) {
		return;
	}
	nvtmpp_lock_sem_p();
	if (g_nvtmpp_info.vb_pools) {
		nvtmpp_vfree(g_nvtmpp_info.vb_pools);
	}
	g_nvtmpp_info.vb_pools = NULL;
	g_sys_init_tag = 0;
	nvtmpp_unlock_sem_p();

}
#if defined(__FREERTOS)
/* return  > 0 for success, 0 for fail*/
UINT32  nvtmpp_sys_va2pa(UINT32 virt_addr)
{
	UINT32 i;

	for (i = 0; i < NVTMPP_DDR_MAX; i++) {
		if (g_sys_conf.ddr_mem[i].virt_addr > 0 && virt_addr >= g_sys_conf.ddr_mem[i].virt_addr &&
			virt_addr < g_sys_conf.ddr_mem[i].virt_addr+g_sys_conf.ddr_mem[i].size) {
			return virt_addr-g_sys_conf.ddr_mem[i].virt_addr+g_sys_conf.ddr_mem[i].phys_addr;
		}
	}
	DBG_ERR("Invalid virt_addr 0x%x\r\n", (int)virt_addr);
	return 0;
}

int  nvtmpp_is_dynamic_map(void)
{
	// freertos not support this function, just return 0
	return 0;
}

UINT32  nvtmpp_get_vcap_fixed_map_poolcnt(void)
{
	// freertos not support this function, just return 0
	return 0;
}

UINT32  nvtmpp_get_dynamic_map_blk_threshold(void)
{
	// freertos not support this function, just return 0
	return 0;
}
#endif

NVTMPP_VB_BLK  nvtmpp_sys_get_blk_by_pa(UINT32 phys_addr)
{
	NVTMPP_VB_POOL_S   *p_pool;
	NVTMPP_VB_BLK_S    *p_blk;
	unsigned long      flags = 0;


	p_pool = nvtmpp_vb_get_pool_by_mem_range_p(phys_addr, phys_addr + 1);
	//DBG_DUMP("p_pool 0x%x\r\n", (int)p_pool);
	if (!p_pool) {
		//DBG_DUMP("1 va 0\r\n");
		return NVTMPP_VB_INVALID_BLK;
	}
	loc_cpu(flags);
	p_blk = p_pool->first_used_blk;
	while (p_blk) {
		//DBG_DUMP("buf_addr 0x%x, blk_size = 0x%x\r\n", p_blk->buf_addr, p_blk->blk_size);
		if (phys_addr >= p_blk->buf_addr && phys_addr < (p_blk->buf_addr + p_blk->blk_size)) {
			unl_cpu(flags);
			return (NVTMPP_VB_BLK)p_blk;
		}
		p_blk = p_blk->next;
	}
	unl_cpu(flags);
	return NVTMPP_VB_INVALID_BLK;
}

/* return  > 0 for success, 0 for fail*/
UINT32  nvtmpp_sys_pa2va(UINT32 phys_addr)
{
	if (!nvtmpp_is_dynamic_map()) {
		UINT32 virt_addr = 0, i;

		for (i = 0; i < NVTMPP_DDR_MAX; i++) {
			if (g_sys_conf.ddr_mem[i].phys_addr > 0 && phys_addr >= g_sys_conf.ddr_mem[i].phys_addr &&
				phys_addr < g_sys_conf.ddr_mem[i].phys_addr+g_sys_conf.ddr_mem[i].size) {
				return phys_addr-g_sys_conf.ddr_mem[i].phys_addr+g_sys_conf.ddr_mem[i].virt_addr;
			}
		}
		return virt_addr;
		/*
		if (g_sys_conf.ddr_mem[NVTMPP_DDR_1].phys_addr > 0 && phys_addr >= g_sys_conf.ddr_mem[NVTMPP_DDR_1].phys_addr &&
			phys_addr < g_sys_conf.ddr_mem[NVTMPP_DDR_1].phys_addr+g_sys_conf.ddr_mem[NVTMPP_DDR_1].size) {
			return phys_addr-g_sys_conf.ddr_mem[NVTMPP_DDR_1].phys_addr+g_sys_conf.ddr_mem[NVTMPP_DDR_1].virt_addr;
		}
		if (g_sys_conf.ddr_mem[NVTMPP_DDR_2].phys_addr > 0 && phys_addr >= g_sys_conf.ddr_mem[NVTMPP_DDR_2].phys_addr &&
			phys_addr < g_sys_conf.ddr_mem[NVTMPP_DDR_2].phys_addr+g_sys_conf.ddr_mem[NVTMPP_DDR_2].size) {
			return phys_addr-g_sys_conf.ddr_mem[NVTMPP_DDR_2].phys_addr+g_sys_conf.ddr_mem[NVTMPP_DDR_2].virt_addr;
		}
		return virt_addr;
		*/
	} else {
		NVTMPP_VB_BLK_S *p_blk;
		NVTMPP_VB_BLK    blk;

		blk = nvtmpp_sys_get_blk_by_pa(phys_addr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			DBG_ERR("fail to get blk by pa 0x%08x\n", phys_addr);
			//nvtmpp_dump_stack();
			return 0;
		}
		p_blk = (NVTMPP_VB_BLK_S *)blk;
		//DBG_DUMP("va 0x%x\r\n", (int)p_blk->buf_va + (phys_addr - p_blk->buf_addr));
		return (p_blk->buf_va + (phys_addr - p_blk->buf_addr));
	}
}


NVTMPP_VB_POOL nvtmpp_sys_create_fixed_pool(CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, NVTMPP_DDR ddr)
{
	NVTMPP_VB_POOL_S     *p_pool;
	NVTMPP_VB_POOL       pool;

	if (g_sys_init_tag != NVTMPP_INITED_TAG) {
		DBG_ERR("Please call nvtmpp_sys_init() firstly\r\n");
		return NVTMPP_ER_UNCONFIG;
	}
	if (g_vb_init_tag == NVTMPP_INITED_TAG) {
		DBG_ERR("State error\r\n");
		return NVTMPP_ER_STATE;
	}
	if (!pool_name) {
		return NVTMPP_VB_INVALID_POOL;
	}
	nvtmpp_lock_sem_p();
	p_pool = nvtmpp_vb_get_free_pool(g_nvtmpp_info.vb_pools, g_sys_conf.max_pools_cnt);
	if (p_pool == NULL) {
		goto create_fixed_pool_err;
	}
	if (nvtmpp_vb_pool_init(p_pool, pool_name, blk_size, blk_cnt, ddr, 0, POOL_TYPE_FIXED) < 0) {
		DBG_ERR("Not enough buffer\r\n");
		goto create_fixed_pool_err;
	}
	g_nvtmpp_info.fixed_pool_count++;
	nvtmpp_unlock_sem_p();
	pool = (int)p_pool;
	return pool;
create_fixed_pool_err:
	nvtmpp_dump_mem_range(vk_printk);
	nvtmpp_unlock_sem_p();
	return  NVTMPP_VB_INVALID_POOL;
}

NVTMPP_ER      nvtmpp_vb_set_conf(const NVTMPP_VB_CONF_S *p_vb_conf)
{
	if (g_sys_init_tag != NVTMPP_INITED_TAG) {
		DBG_ERR("Please call nvtmpp_sys_init() firstly\r\n");
		return NVTMPP_ER_UNCONFIG;
	}
	if (p_vb_conf == NULL) {
		return NVTMPP_ER_PARM;
	}
	#if 0
	if (p_vb_conf->max_pool_cnt > NVTMPP_VB_MAX_POOLS) {
		DBG_ERR("max_pool_cnt %d exceeds limit %d \r\n", p_vb_conf->max_pool_cnt, NVTMPP_VB_MAX_POOLS);
		return NVTMPP_ER_PARM;
	}
	#endif
	nvtmpp_lock_sem_p();
	g_vb_conf = *p_vb_conf;
	g_vb_conf_init_tag = NVTMPP_INITED_TAG;
	nvtmpp_unlock_sem_p();
	return NVTMPP_ER_OK;
}

NVTMPP_ER      nvtmpp_vb_get_conf(NVTMPP_VB_CONF_S *p_vb_conf)
{
	if (p_vb_conf == NULL) {
		return NVTMPP_ER_PARM;
	}
	*p_vb_conf = g_vb_conf;
	return NVTMPP_ER_OK;
}

int nvtmpp_vb_enqueue_va(UINT32 va)
{
	NVTMPP_CHK_UNMAP_CTRL  *p_ctrl = &nvtmpp_check_unmap_ctrl;
	UINT32                  in_next_idx;

	DBG_IND("in_idx =%d, va =0x%x\r\n", (int)p_ctrl->in_idx, va);
	in_next_idx = p_ctrl->in_idx + 1;
	if (in_next_idx >= NVTMPP_FREE_VA_MAX_NUM) {
		in_next_idx -= NVTMPP_FREE_VA_MAX_NUM;
	}
	if (in_next_idx == p_ctrl->out_idx) {
		DBG_ERR("wait_free_va_list is full\r\n");
		return -1;
	}
	p_ctrl->wait_free_va_list[p_ctrl->in_idx] = va;
	p_ctrl->in_idx = in_next_idx;
	return NVTMPP_ER_OK;
}

void nvtmpp_vb_dequeue_all_va(void)
{
	NVTMPP_CHK_UNMAP_CTRL  *p_ctrl = &nvtmpp_check_unmap_ctrl;
	UINT32 va;
	unsigned long      flags = 0;

	while (p_ctrl->out_idx != p_ctrl->in_idx) {
		loc_cpu(flags);
		va = p_ctrl->wait_free_va_list[p_ctrl->out_idx];
		DBG_IND("unmap va = 0x%x, out_idx = %d\r\n", (int)va, (int)p_ctrl->out_idx);
		p_ctrl->out_idx++;
		if (p_ctrl->out_idx >= NVTMPP_FREE_VA_MAX_NUM) {
			p_ctrl->out_idx -= NVTMPP_FREE_VA_MAX_NUM;
		}
		unl_cpu(flags);
		nvtmpp_iounmap((void *)va);
	}
}

static THREAD_DECLARE(nvtmpp_unmap_func, arglist)
{
	NVTMPP_CHK_UNMAP_CTRL    *p_ctrl;

	p_ctrl = (NVTMPP_CHK_UNMAP_CTRL    *)arglist;
	while (!THREAD_SHOULD_STOP) {
		if (!p_ctrl->start) {
			break;
		}
		nvtmpp_vb_dequeue_all_va();
		vos_task_delay_ms(p_ctrl->sleep_ms);
	}
	set_flg(NVTMPP_FLAG_ID, FLG_NVTMPP_TASK_EXIT);
	THREAD_RETURN(0);
}

NVTMPP_ER      nvtmpp_vb_init(void)
{
	#if NVTMPP_FASTBOOT_SUPPORT
	NVT_VB_CPOOL_S      *p_cfgpool = &g_vb_conf.common_pool[0];
	#endif
	if (g_vb_conf_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNCONFIG;
	}
	if (g_vb_init_tag == NVTMPP_INITED_TAG) {
		DBG_WRN("re-init => force exit and init again !!!!");
		nvtmpp_vb_exit();
		g_vb_conf_init_tag = NVTMPP_INITED_TAG;
	}
	nvtmpp_vb_free_old_pools_p();
	if (nvtmpp_vb_init_common_pools_p(g_vb_conf.common_pool) < 0) {
		return NVTMPP_ER_PARM;
	}
	nvtmpp_lock_sem_p();
	#if NVTMPP_DYNAMIC_VA_DEBUG
	fmem_reg_dump_va_cb(nvtmpp_vb_map_list_dump);
	nvtmpp_vb_map_list_init_p();
	#endif
	nvtmpp_vb_module_init();
	nvtmpp_vb_module_add_one(USER_MODULE);
	g_vb_init_tag = NVTMPP_INITED_TAG;
	if (nvtmpp_is_dynamic_map()) {
		NVTMPP_CHK_UNMAP_CTRL  *p_ctrl = &nvtmpp_check_unmap_ctrl;

		p_ctrl->start = TRUE;
		p_ctrl->sleep_ms = 30;
		THREAD_CREATE(p_ctrl->thread, nvtmpp_unmap_func, p_ctrl, "chk_unmap_thread");
		THREAD_SET_PRIORITY(p_ctrl->thread, 8);
		THREAD_RESUME(p_ctrl->thread);
	}
	nvtmpp_unlock_sem_p();
	nvtmpp_install_cmd();
	#if NVTMPP_FASTBOOT_SUPPORT
	// check if init with common pools
	if (p_cfgpool->blk_cnt && p_cfgpool->blk_size && nvtmpp_vb_init_fastboot_pools_p() < 0) {
		return NVTMPP_ER_PARM;
	}
	#endif


	return NVTMPP_ER_OK;
}



NVTMPP_ER      nvtmpp_vb_exit(void)
{
	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	nvtmpp_lock_sem_p();
	nvtmpp_vb_chk_dump_memory_not_freed_p();
	g_vb_init_tag = 0;
	g_vb_conf_init_tag = 0;
	nvtmpp_uninstall_cmd();
	if (nvtmpp_is_dynamic_map()) {
		NVTMPP_CHK_UNMAP_CTRL  *p_ctrl = &nvtmpp_check_unmap_ctrl;
		UINT32                 delay_count = 0;
		FLGPTN                 flag_ptn = 0;

		while (p_ctrl->out_idx != p_ctrl->in_idx) {
			vos_task_delay_ms(p_ctrl->sleep_ms);
			delay_count++;
			if (delay_count > 20) {
				DBG_ERR("timeout for unmap in_idx =%d, out_idx=%d\r\n", p_ctrl->in_idx, p_ctrl->out_idx);
				break;
			}
		}
		clr_flg(NVTMPP_FLAG_ID, FLG_NVTMPP_TASK_EXIT);
		p_ctrl->start = FALSE;
		// wait task exit
		wai_flg(&flag_ptn, NVTMPP_FLAG_ID, FLG_NVTMPP_TASK_EXIT, TWF_ORW);
		//vos_task_delay_ms(200);
	}
	nvtmpp_unlock_sem_p();
	return NVTMPP_ER_OK;
}

NVTMPP_ER      nvtmpp_vb_relayout(void)
{
	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	nvtmpp_vb_chk_dump_memory_not_freed_p();
	nvtmpp_vb_free_old_pools_p();
	if (nvtmpp_vb_init_common_pools_p(g_vb_conf.common_pool) < 0) {
		return NVTMPP_ER_PARM;
	}
	#if NVTMPP_FASTBOOT_SUPPORT
	if (nvtmpp_vb_init_fastboot_pools_p() < 0) {
		return NVTMPP_ER_PARM;
	}
	#endif
	return NVTMPP_ER_OK;
}

NVTMPP_VB_POOL nvtmpp_vb_create_pool(CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, NVTMPP_DDR ddr)
{
	NVTMPP_VB_POOL_S     *p_pool;
	NVTMPP_VB_POOL       pool;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_VB_INVALID_POOL;
	}
	if (!pool_name) {
		return NVTMPP_VB_INVALID_POOL;
	}
	if ((ddr >= NVTMPP_DDR_MAX) && (ddr != NVTMPP_DDR_AUTO)) {
		DBG_ERR("Invalid ddr_id %d\r\n", ddr + 1);
		return NVTMPP_VB_INVALID_POOL;
	}
	#if NVTMPP_FASTBOOT_SUPPORT
	if (g_nvtmpp_info.fastboot_pool_count) {
		p_pool = nvtmpp_vb_chk_dup_with_fastboot_pool(pool_name);
		if (p_pool) {
			pool = (int)p_pool;
			if (blk_size != p_pool->blk_size) {
				DBG_ERR("%s blk_size = (0x%x) mismatch with fastboot pool blk_size (0x%x)\r\n", pool_name, blk_size, p_pool->blk_size);
				return  NVTMPP_VB_INVALID_POOL;
			}
			return pool;
		}
	}
	#endif
	nvtmpp_lock_sem_p();
	p_pool = nvtmpp_vb_get_free_pool(g_nvtmpp_info.vb_pools, g_sys_conf.max_pools_cnt);
	if (p_pool == NULL) {
		goto create_pool_err;
	}
	if (ddr == NVTMPP_DDR_AUTO) {
		UINT32  i;

		for (i = 0; i < NVTMPP_DDR_MAX; i++) {
			if (nvtmpp_vb_pool_init(p_pool, pool_name, blk_size, blk_cnt, i, 0, POOL_TYPE_PRIVATE) >= 0) {
				goto create_pool_success;
			}
		}
		DBG_ERR("Not enough buffer, ddr %d, want 0x%x\r\n", ddr, blk_cnt*blk_size);
		goto create_pool_err;
	} else {
		if (nvtmpp_vb_pool_init(p_pool, pool_name, blk_size, blk_cnt, ddr, 0, POOL_TYPE_PRIVATE) < 0) {
			DBG_ERR("Not enough buffer, ddr %d, want 0x%x, maxfree 0x%x\r\n", ddr, blk_cnt*blk_size, nvtmpp_heap_get_max_free_block_size(ddr));
			goto create_pool_err;
		}
	}
create_pool_success:
	nvtmpp_unlock_sem_p();
	pool = (int)p_pool;
	//DBG_ERR("pool 0x%x\r\n", pool);
	return pool;
create_pool_err:
	//nvtmpp_dump_mem_range(vk_printk);
	nvtmpp_unlock_sem_p();
	return  NVTMPP_VB_INVALID_POOL;
}

NVTMPP_ER      nvtmpp_vb_destroy_pool(NVTMPP_VB_POOL pool)
{
	UINT32             pool_addr;
	NVTMPP_VB_POOL_S   *p_pool;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	pool_addr = pool;
	if ((pool == NVTMPP_VB_INVALID_POOL) || (pool_addr < POOL_TYPE_PRIVATE)) {
		DBG_ERR("Invalid pool 0x%x\r\n", (int)pool);
		return NVTMPP_ER_POOL_UNEXIST;
	}

	p_pool = (NVTMPP_VB_POOL_S *)pool_addr;
	if (!nvtmpp_vb_pool_chk_valid(pool_addr)) {
		DBG_ERR("Invalid pool 0x%x\r\n", (int)pool);
		return NVTMPP_ER_POOL_UNEXIST;
	}
	nvtmpp_lock_sem_p();
	nvtmpp_vb_pool_exit(p_pool);
	nvtmpp_unlock_sem_p();
	return NVTMPP_ER_OK;
}

NVTMPP_ER      nvtmpp_vb_destroy_pool_2(NVTMPP_VB_POOL pool, CHAR *pool_name)
{
	UINT32             pool_addr;
	NVTMPP_VB_POOL_S   *p_pool;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	pool_addr = pool;
	if ((pool == NVTMPP_VB_INVALID_POOL) || (pool_addr < POOL_TYPE_PRIVATE)) {
		DBG_ERR("Invalid pool 0x%x\r\n", (int)pool);
		return NVTMPP_ER_POOL_UNEXIST;
	}

	p_pool = (NVTMPP_VB_POOL_S *)pool_addr;
	if (!nvtmpp_vb_pool_chk_valid(pool_addr)) {
		DBG_ERR("Invalid pool 0x%x\r\n", (int)pool);
		return NVTMPP_ER_POOL_UNEXIST;
	}
	if (pool_name && strncmp(p_pool->pool_name, pool_name, strlen(pool_name)) != 0) {
		return NVTMPP_ER_POOL_UNEXIST;
	}
	nvtmpp_lock_sem_p();
	nvtmpp_vb_pool_exit(p_pool);
	nvtmpp_unlock_sem_p();
	return NVTMPP_ER_OK;
}


// default reference count is 1¡Aneed to add one reference blk
NVTMPP_VB_BLK  nvtmpp_vb_get_block(NVTMPP_MODULE module, NVTMPP_VB_POOL pool, UINT32 blk_size, NVTMPP_DDR ddr)
{
	NVTMPP_VB_POOL_S   *p_pool = NULL;
	NVTMPP_VB_BLK      blk;
	NVTMPP_ER          ret;
	INT32              module_id;
	UINT32             pool_addr;
	unsigned long      flags = 0;

	//DBG_ERR("pool 0x%x\r\n", (int)pool);
	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_VB_INVALID_BLK;
	}
	if ((ddr >= NVTMPP_DDR_MAX) && (ddr != NVTMPP_DDR_AUTO)) {
		DBG_ERR("Invalid ddr_id %d\r\n", ddr + 1);
		return NVTMPP_VB_INVALID_BLK;
	}
	pool_addr = pool;
	// get blk from private pools
	if ((pool != NVTMPP_VB_INVALID_POOL) && (pool_addr > POOL_TYPE_PRIVATE)) {
		if (!nvtmpp_vb_pool_chk_valid(pool_addr)) {
			return NVTMPP_VB_INVALID_BLK;
		}
		module_id = nvtmpp_vb_module_to_index(module);
		if (module_id < 0) {
			return NVTMPP_ER_UNKNOWW_MODULE;
		}
		loc_cpu(flags);
		p_pool = (NVTMPP_VB_POOL_S *)pool_addr;
		blk = nvtmpp_vb_get_free_block_from_pool(p_pool, blk_size);
		// add one block reference
		ret = nvtmpp_vb_add_blk_ref(module_id, blk);
		if (ret != NVTMPP_ER_OK) {
			blk = NVTMPP_VB_INVALID_BLK;
			goto get_block_err;
		}
		unl_cpu(flags);
		if (nvtmpp_vb_init_blk_map_va_p(blk) == NULL) {
			DBG_ERR("nvtmpp_vb_blk_map_va fail\r\n");
			return NVTMPP_VB_INVALID_BLK;
		}
		return blk;
	} else { // get blk from common pools
		module_id = nvtmpp_vb_module_to_index(module);
		if (module_id < 0) {
			return NVTMPP_ER_UNKNOWW_MODULE;
		}
		loc_cpu(flags);
		if (pool == NVTMPP_VB_INVALID_POOL) {
			if (ddr == NVTMPP_DDR_AUTO) {
				UINT32  i, min_blk_size = 0xFFFFFFFF;
				NVTMPP_VB_POOL_S   *p_tmp_pool[NVTMPP_DDR_MAX];

				for (i = 0; i < NVTMPP_DDR_MAX; i++) {
					p_tmp_pool[i] = nvtmpp_vb_pool_search_match_size_from_common(g_nvtmpp_info.vb_comm_pool_link_start[i], blk_size);
					//DBG_DUMP("p_tmp_pool = 0x%x\r\n", (int)p_tmp_pool[i]);
					if (p_tmp_pool[i] && p_tmp_pool[i]->blk_size < min_blk_size) {
						min_blk_size = p_tmp_pool[i]->blk_size;
						p_pool = p_tmp_pool[i];
					}
				}
			} else {
				p_pool = nvtmpp_vb_pool_search_match_size_from_common(g_nvtmpp_info.vb_comm_pool_link_start[ddr], blk_size);
			}
		} else {
			p_pool = nvtmpp_vb_pool_search_common_by_pool_type(&g_nvtmpp_info.vb_pools[g_nvtmpp_info.fixed_pool_count], g_nvtmpp_info.comm_pool_count, pool, blk_size, ddr);
		}
		if (p_pool == NULL) {
			blk = NVTMPP_VB_INVALID_BLK;
			goto get_block_err;
		}
		blk = nvtmpp_vb_get_free_block_from_pool(p_pool, blk_size);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			goto get_block_err;
		}
		// add one block reference
		ret = nvtmpp_vb_add_blk_ref(module_id, blk);
		if (ret != NVTMPP_ER_OK) {
			blk = NVTMPP_VB_INVALID_BLK;
			goto get_block_err;
		}
		if (p_pool->dump_max_cnt && (p_pool->dump_max_cnt <= (p_pool->blk_cnt - p_pool->blk_free_cnt))) {
			DBG_ERR("p_pool = 0x%x, dump_max_cnt = %d, blk_cnt=%d, free_cnt=%d \r\n", (int)p_pool, p_pool->dump_max_cnt, p_pool->blk_cnt, p_pool->blk_free_cnt);
			nvtmpp_dump_status(vk_printk);
			nvtmpp_vb_pool_set_dump_max_cnt(p_pool, 0);
		}
		unl_cpu(flags);
		if (pool == NVTMPP_VB_INVALID_POOL && nvtmpp_vb_init_blk_map_va_p(blk) == NULL) {
			DBG_ERR("nvtmpp_vb_blk_map_va fail\r\n");
			return NVTMPP_VB_INVALID_BLK;
		}
	    return blk;
get_block_err:
		nvtmpp_vb_module_add_err_cnt(module_id, NVTMPP_MODULE_ER_GET_BLK_FAIL);
		if (g_nvtmpp_info.showmsg_level > 0) {
			CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

			nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
			DBG_ERR("module '%s' get a block fail, blk_size 0x%x, ddr %d\r\n", module_str, (int)blk_size, ddr+1);
			nvtmpp_dump_status(vk_printk);
		}
		unl_cpu(flags);
	    return blk;
	}
	return NVTMPP_VB_INVALID_BLK;
}

NVTMPP_ER      nvtmpp_vb_lock_block_p(NVTMPP_MODULE module, NVTMPP_VB_BLK blk)
{
	NVTMPP_ER          ret;
	INT32              module_id;
	NVTMPP_VB_BLK_S    *p_blk;

	p_blk = (NVTMPP_VB_BLK_S *)blk;
	module_id = nvtmpp_vb_module_to_index(module);
	if (module_id < 0) {
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	DBG_IND("module_id = %d\r\n", module_id);
	// the reference count should >=1 after get block, so here should not be zero
	if (p_blk->total_ref_cnt == 0) {
		ret = NVTMPP_ER_BLK_ALREADY_FREE;
		goto lock_block_err;
	}
	ret = nvtmpp_vb_add_blk_ref(module_id, blk);
	if (ret != NVTMPP_ER_OK) {
		goto lock_block_err;
	}
	return NVTMPP_ER_OK;

lock_block_err:
	nvtmpp_vb_module_add_err_cnt(module_id, NVTMPP_MODULE_ER_LOCK_BLK_FAIL);
	if (ret == NVTMPP_ER_BLK_ALREADY_FREE) {
		CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

		nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
		DBG_ERR("module '%s' want to lock a freed block 0x%x\r\n", module_str, (int)blk);
	}
	return ret;
}

NVTMPP_ER      nvtmpp_vb_lock_block(NVTMPP_MODULE module, NVTMPP_VB_BLK blk)
{
	NVTMPP_ER          ret;
	unsigned long      flags = 0;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

		nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
		DBG_ERR("module '%s' want to lock invalid blk 0x%x\r\n", module_str, (int)blk);
		return NVTMPP_ER_PARM;
	}
	loc_cpu(flags);
	ret = nvtmpp_vb_lock_block_p(module, blk);
	unl_cpu(flags);
	return ret;
}


NVTMPP_ER      nvtmpp_vb_unlock_block(NVTMPP_MODULE module, NVTMPP_VB_BLK blk)
{
	UINT32             pool;
	NVTMPP_VB_POOL_S   *p_pool;
	NVTMPP_ER          ret;
	INT32              module_id;
	unsigned long      flags = 0;
	NVTMPP_VB_BLK_S    *p_blk = (NVTMPP_VB_BLK_S *)blk;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

		nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
		DBG_ERR("module '%s' want to unlock invalid blk 0x%x\r\n", module_str, (int)blk);
		return NVTMPP_ER_PARM;
	}
	NVTMPP_BLK2POOL(pool, blk);
	p_pool = (NVTMPP_VB_POOL_S *)pool;
	DBG_IND("blk = 0x%x, p_pool = 0x%x, is_common =%d, pool_type = 0x%x\r\n",
		(int)blk, (int)p_pool, (int)p_pool->is_common, (int)p_pool->pool_type);
	#if 0
	if (!p_pool->is_common) {
		return NVTMPP_ER_OK;
	}
	#endif
	module_id = nvtmpp_vb_module_to_index(module);
	if (module_id < 0) {
		DBG_ERR("\r\n");
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	loc_cpu(flags);
	ret = nvtmpp_vb_minus_blk_ref(module_id, blk);
	if (ret != NVTMPP_ER_OK && ret != NVTMPP_BLK_REF_REACH_ZERO) {
		goto unlock_block_err;
	}
	if (p_pool->is_common && (p_pool->pool_type == POOL_TYPE_COMMON) &&
		(p_blk->total_ref_cnt == 1) && (p_blk->module_ref_cnt[USER_MODULE_IDX] == 1)) {
		if (nvtmpp_is_dynamic_map() &&
			p_pool->blk_size > nvtmpp_get_dynamic_map_blk_threshold() &&
			p_blk->buf_va) {
			#if NVTMPP_DYNAMIC_VA_DEBUG
			{
				NVTMPP_VA_MAP_INFO map_info;

				map_info.pa = p_blk->buf_addr;
				map_info.va = p_blk->buf_va;
				map_info.size = p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size();
				nvtmpp_vb_map_list_del_p(&map_info);
			}
			#endif
			nvtmpp_vb_enqueue_va(p_blk->buf_va);
			//DBG_DUMP("unmap va 0x%x, size =0x%x\r\n", p_blk->buf_va, p_pool->blk_size);
			p_blk->buf_va = 0;

		}
	}
	// reference count reaches zero
	if (/*p_pool->is_common &&*/ ret == NVTMPP_BLK_REF_REACH_ZERO) {
		if (p_pool->is_common && nvtmpp_is_dynamic_map() &&
			p_pool->blk_size > nvtmpp_get_dynamic_map_blk_threshold() &&
			p_blk->buf_va &&
			(p_pool->pool_type == POOL_TYPE_COMMON)) {
			#if NVTMPP_DYNAMIC_VA_DEBUG
			{
				NVTMPP_VA_MAP_INFO map_info;

				map_info.pa = p_blk->buf_addr;
				map_info.va = p_blk->buf_va;
				map_info.size = p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size();
				nvtmpp_vb_map_list_del_p(&map_info);
			}
			#endif
			nvtmpp_vb_enqueue_va(p_blk->buf_va);
			//DBG_DUMP("unmap 2 va 0x%x, size =0x%x\r\n", p_blk->buf_va, p_pool->blk_size);
			p_blk->buf_va = 0;
		}
		nvtmpp_vb_release_block_back_pool(p_pool, (NVTMPP_VB_BLK_S *)blk);
		p_pool->blk_free_cnt++;
	}
	unl_cpu(flags);
	return NVTMPP_ER_OK;

unlock_block_err:
	nvtmpp_vb_module_add_err_cnt(module_id, NVTMPP_MODULE_ER_UNLOCK_BLK_FAIL);
	unl_cpu(flags);
	if (NVTMPP_ER_BLK_UNLOCK_REF == ret) {
		CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

		nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
		DBG_ERR("module '%s' want to free a block 0x%x, but the block total refere count is 0\r\n", module_str, (int)blk);
	}
	else if (NVTMPP_ER_BLK_UNLOCK_MODULE_REF == ret) {
		CHAR   module_str[(sizeof(NVTMPP_MODULE) + 1)];

		nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
		DBG_ERR("module '%s' want to free a block 0x%x, but the block module's refere count is 0\r\n", module_str, (int)blk);
	}
	return ret;
}


UINT32      nvtmpp_vb_get_max_free_size(NVTMPP_DDR ddr)
{
	UINT32 blk_header_size;
	UINT32 max_heap_free_size;
	UINT32 pool_reserved_size;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return 0;
	}
	if (ddr >= NVTMPP_DDR_MAX) {
		return 0;
	}
	blk_header_size = nvtmpp_vb_get_blk_header_size();
	pool_reserved_size = nvtmpp_vb_get_pool_reserved_size();
	max_heap_free_size =  nvtmpp_heap_get_max_free_block_size(ddr);
	if (ALIGN_FLOOR(max_heap_free_size, NVTMPP_BLK_ALIGN) > (blk_header_size + pool_reserved_size)) {
		return (ALIGN_FLOOR(max_heap_free_size, NVTMPP_BLK_ALIGN) - blk_header_size - pool_reserved_size);
	} else {
		return 0;
	}
}

NVTMPP_ER      nvtmpp_vb_add_module(NVTMPP_MODULE module)
{
	NVTMPP_ER ret;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return NVTMPP_ER_UNINIT;
	}
	nvtmpp_lock_sem_p();
	ret = nvtmpp_vb_module_add_one(module);
	nvtmpp_unlock_sem_p();
	return ret;
}

void           nvtmpp_dump_status(int (*dump)(const char *fmt, ...))
{
	if (g_sys_init_tag != NVTMPP_INITED_TAG) {
		dump("nvtmpp not init\r\n");
		return;
	}
	nvtmpp_dump_pub_info_p(dump);
	nvtmpp_dump_fixed_pools_p(dump);
	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return;
	}
	nvtmpp_dump_common_pools_p(dump);
	nvtmpp_dump_private_pools_p(dump, FALSE);
}

#if 0
NVTMPP_VB_POOL nvtmpp_vb_block2pool(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_POOL_S *p_pool;
	UINT32            i;
	UINT32            blk_phy_addr = ((NVTMPP_VB_BLK_S *)blk)->buf_addr;

	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return NVTMPP_VB_INVALID_POOL;
	}
	for (i = 0; i < g_vb_conf.max_pool_cnt; i++) {
		p_pool = &g_nvtmpp_info.vb_pools[i];
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		if (blk_phy_addr >= p_pool->pool_addr && blk_phy_addr < p_pool->pool_addr + p_pool->pool_size)
			return (NVTMPP_VB_POOL)p_pool;
	}
	return NVTMPP_VB_INVALID_POOL;
}
#endif


void           nvtmpp_dump_mem_range(int (*dump)(const char *fmt, ...))
{
	NVTMPP_HEAP_LINK_S *p_free_block, *p_used_block, *p_block;
	UINT32              i, ddr;
	char                poolname[32] = {0};
	NVTMPP_MEMINFO_S   *p_ddr_mem;
	NVTMPP_VB_POOL_S   *p_pool;

	if (NVTMPP_INITED_TAG != g_sys_init_tag) {
		dump("nvtmpp not init\r\n");
		return;
	}
	for (ddr = NVTMPP_DDR_1; ddr < NVTMPP_DDR_MAX; ddr++) {
		p_ddr_mem = &g_sys_conf.ddr_mem[ddr];
		if (p_ddr_mem->size == 0)
			continue;
		#if defined __UITRON || defined __ECOS || defined __FREERTOS
		dump("\r\n-------------------------NVTMPP MEM LAYOUT ----------------------------------------------------\r\n");
		dump("nvtmpp ddr%d: Range[0x%08X~0x%08X] Size=0x%08X\r\n",
			ddr+1, (int)p_ddr_mem->phys_addr, (int)(p_ddr_mem->phys_addr + p_ddr_mem->size), (int)p_ddr_mem->size);
		#else
		dump("\r\n-------------------------MEM LAYOUT -----------------------------------------------------------\r\n");
		dump("DDR%d: Phy[0x%08X~0x%08X], Vir[0x%08X~0x%08X] Size=0x%08X\r\n",
			ddr+1, (int)p_ddr_mem->phys_addr, (int)p_ddr_mem->phys_addr + p_ddr_mem->size,
			(int)p_ddr_mem->virt_addr, (int)(p_ddr_mem->virt_addr + p_ddr_mem->size), (int)p_ddr_mem->size);
		#endif
		p_free_block = nvtmpp_heap_get_first_free_block(ddr);
		p_used_block = nvtmpp_heap_get_first_used_block(ddr);
		i = 0;
		while (p_free_block != NULL && p_used_block != NULL) {
			if (p_free_block && p_free_block->block_addr < p_used_block->block_addr) {
				p_block = p_free_block;
				p_free_block = p_free_block->next;
				strncpy(poolname, "[free]", sizeof(poolname)-1);
			} else if (p_used_block) {
				p_block = p_used_block;
				p_used_block = p_used_block->next;
				if (0 == p_block->block_size) {
					continue;
				}
				p_pool = nvtmpp_vb_get_pool_by_mem_range_p(ALIGN_CEIL((p_block->block_addr + nvtmpp_vb_get_blk_header_size()), NVTMPP_BLK_ALIGN),
				                                           ALIGN_CEIL((p_block->block_addr + nvtmpp_vb_get_blk_header_size()), NVTMPP_BLK_ALIGN) + nvtmpp_vb_get_blk_end_tag_size());
				if (p_pool == NULL) {
					break;
				}
				if (p_pool->is_common)
					strncpy(poolname, "common pool", sizeof(poolname)-1);
				else
					strncpy(poolname, p_pool->pool_name, sizeof(poolname)-1);
			}
			if (p_block && p_block->block_size != 0) {
				#if defined __UITRON || defined __ECOS || defined __FREERTOS
				dump("   Pool-%.2d: Range[0x%08X~0x%08X] Size=0x%08X, %s\r\n",
						 i++,
						 (int)p_block->block_addr,
						 (int)p_block->block_addr+p_block->block_size,
						 (int)p_block->block_size,
						 poolname
						 );
				#else
				if (!nvtmpp_is_dynamic_map()) {
					dump("   Pool-%.2d: Phy[0x%08X~0x%08X], Vir[0x%08X~0x%08X] Size=0x%08X, %s\r\n",
						 i++,
						 (int)p_block->block_addr,
						 (int)p_block->block_addr+p_block->block_size,
						 (int)p_block->block_va,
						 (int)p_block->block_va+p_block->block_size,
						 (int)p_block->block_size,
						 poolname
						 );
				} else {
					dump("   Pool-%.2d: Phy[0x%08X~0x%08X], Vir[0x%08X~0x%08X] Size=0x%08X, %s\r\n",
							 i++,
							 (int)p_block->block_addr,
							 (int)p_block->block_addr+p_block->block_size,
							 (int)p_block->block_va,
							 (int)0,
							 (int)p_block->block_size,
							 poolname
							 );
				}
				#endif
			}
		}
		//dump("\r\n-----------------------------------------------------------------------------------------------\r\n");
	}
}

void      nvtmpp_dump_err_status(int (*dump)(const char *fmt, ...))
{
	char   module_string[(sizeof(NVTMPP_MODULE) + 1)*NVTMPP_MODULE_MAX_NUM + 1];
	UINT32 module_strlen_list[NVTMPP_MODULE_MAX_NUM];
	UINT32 i, module_max_count;
	//CHAR   tmpbuf[10] = "%8ld ";

	if (NVTMPP_INITED_TAG != g_vb_init_tag) {
		dump("nvtmpp not init\r\n");
		return;
	}
	dump("\r\n-------------------------ERR STATUS -----------------------------------------------------------\r\n");
	nvtmpp_vb_get_all_modules_list_string(module_string, sizeof(module_string), module_strlen_list, TRUE);
	dump("Modules:      %s\r\n", module_string);
	dump("Get blk Fail: ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_GET_BLK_FAIL));
	}
	dump("\r\n");
	dump("Loc blk Fail: ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_LOCK_BLK_FAIL));
	}
	dump("\r\n");
	dump("Unl blk Fail: ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_UNLOCK_BLK_FAIL));
	}
	dump("\r\n");
	dump("Drop:         ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_DROP));
	}
	dump("\r\n");
	dump("Wrn:          ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_WRN));
	}
	dump("\r\n");
	dump("Err:          ");
	module_max_count = nvtmpp_vb_get_module_count();
	for (i = 0; i < module_max_count; i++) {
		dump("%8ld ", nvtmpp_vb_module_get_err_cnt(i, NVTMPP_MODULE_ER_MISC));
	}
	dump("\r\n");
}


BOOL nvtmpp_vb_set_dump_max_cnt(UINT32 pool_id, UINT32 dump_max_cnt)
{
	NVTMPP_VB_POOL_S *p_pool;

	if (pool_id >= NVTMPP_VB_MAX_COMM_POOLS)
		return FALSE;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	if (p_pool->init_tag != NVTMPP_INITED_TAG || p_pool->is_common == 0)
		return FALSE;
	nvtmpp_vb_pool_set_dump_max_cnt(p_pool, dump_max_cnt);
	return TRUE;
}

void   nvtmpp_vb_set_showmsg_level(UINT32 showmsg_level)
{
	g_nvtmpp_info.showmsg_level = showmsg_level;
	DBG_DUMP("set_showmsg_level = %d\r\n", (int)showmsg_level);
}

UINT32   nvtmpp_vb_get_showmsg_level(void)
{
	return g_nvtmpp_info.showmsg_level;
}

void   nvtmpp_vb_set_memory_corrupt_check_en(BOOL en)
{
	g_nvtmpp_info.memory_corrupt_check_en = en;
}

INT32  nvtmpp_vb_check_mem_corrupt(void)
{
	if (FALSE == g_nvtmpp_info.memory_corrupt_check_en) {
		return 0;
	}
	return nvtmpp_heap_check_mem_corrupt();
}


BOOL nvtmpp_vb_get_comm_pool_range(NVTMPP_DDR ddr, MEM_RANGE *p_range)
{
	NVTMPP_VB_POOL_S   *p_pool, *p_last_comm_pool = NULL;
	int                i, pool_id;
	UINT32              start_addr = 0;
	UINT32              end_addr = 0;

	pool_id = g_nvtmpp_info.fixed_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < NVTMPP_VB_MAX_COMM_POOLS; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG || p_pool->is_common == 0) {
			break;
		}
		if (start_addr == 0 && ddr == p_pool->ddr) {
			start_addr = p_pool->first_blk->buf_addr;
		}
		if (ddr == p_pool->ddr) {
			p_last_comm_pool = p_pool;
		}
	}
	if (p_last_comm_pool) {
		end_addr = p_last_comm_pool->pool_addr + p_last_comm_pool->pool_size;
	}
	//DBG_ERR("start_addr = 0x%x, end_addr = 0x%x\r\n", start_addr, end_addr);
	if (end_addr && start_addr) {
		p_range->addr = start_addr;
		p_range->size = end_addr - start_addr;
		//DBG_ERR("p_range->addr = 0x%x, p_range->size = 0x%x\r\n", p_range->addr, p_range->size);
		return TRUE;
	}
	return FALSE;
}
#if NVTMPP_FASTBOOT_SUPPORT
NVTMPP_VB_POOL_S *nvtmpp_vb_get_pool_by_name(char *name)
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i, pool_id;

	pool_id = g_nvtmpp_info.fixed_pool_count + g_nvtmpp_info.comm_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < g_sys_conf.max_pools_cnt - pool_id; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		if (!p_pool->is_common) {
			if (strncmp(p_pool->pool_name, name, strlen(p_pool->pool_name))== 0) {
				return p_pool;
			}
		}
	}
	return NULL;
}

NVTMPP_VB_POOL_S *nvtmpp_vb_get_first_private_pool(NVTMPP_DDR ddr)
{
	NVTMPP_VB_POOL_S *p_pool;
	int                i, pool_id;

	pool_id = g_nvtmpp_info.fixed_pool_count + g_nvtmpp_info.comm_pool_count;
	p_pool = &g_nvtmpp_info.vb_pools[pool_id];
	for (i = 0; i < g_sys_conf.max_pools_cnt - pool_id; i++, p_pool++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			continue;
		}
		if (!p_pool->is_common && p_pool->ddr == ddr) {
			return p_pool;
		}
	}
	return NULL;
}

UINT32 nvtmpp_vb_arrange_fastboot_next_blk(UINT32 blk_addr, UINT32 blk_size)
{
	UINT32 blk_total_size, heap_size;

	blk_total_size = ALIGN_CEIL(blk_size + nvtmpp_vb_get_blk_header_size() + nvtmpp_vb_get_blk_end_tag_size(), NVTMPP_BLK_ALIGN);
	heap_size = ALIGN_CEIL(blk_total_size + (HEAP_STRUCT_SIZE+HEAP_END_TAG_SIZE), NVTMPP_HEAP_ALIGN);
	return (blk_addr + heap_size);
}
#endif
UINT32 nvtmpp_vb_get_block_refcount(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S    *p_blk;

	if (g_vb_init_tag != NVTMPP_INITED_TAG) {
		return 0;
	}
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return 0;
	}
	p_blk = (NVTMPP_VB_BLK_S *)blk;
	return p_blk->total_ref_cnt;
}
NVTMPP_ER nvtmpp_vb_check_pool_valid(NVTMPP_VB_POOL pool)
{
	if (nvtmpp_vb_pool_chk_valid(pool) == TRUE) {
		return NVTMPP_ER_OK;
	} else {
		return NVTMPP_ER_POOL_UNEXIST;
	}
}
static void *nvtmpp_vb_init_blk_map_va_p(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk = (NVTMPP_VB_BLK_S *)blk;
	void            *va;

	if (!nvtmpp_is_dynamic_map()) {
		return (void *)p_blk->buf_va;
	}
	if (p_blk->buf_va) {
		return (void *)p_blk->buf_va;
	}
	va = nvtmpp_ioremap_cache(p_blk->buf_addr, p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size());
	p_blk->buf_va = (UINT32)va;
	#if NVTMPP_DYNAMIC_VA_DEBUG
	{
		NVTMPP_VA_MAP_INFO map_info;
		unsigned long    flags = 0;

		map_info.pa = p_blk->buf_addr;
		map_info.va = (UINT32)va;
		map_info.size = p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size();
		loc_cpu(flags);
		nvtmpp_vb_map_list_add_p(&map_info);
		unl_cpu(flags);
	}
	#endif
	DBG_IND("map pa = 0x%x, va = 0x%x, size 0x%x\r\n", (int)p_blk->buf_addr, (int)va, (int)p_blk->blk_size);
	return va;
}

static void *nvtmpp_vb_lock_blk_map_va_p(NVTMPP_VB_BLK blk, BOOL is_lock_blk)
{
	NVTMPP_VB_BLK_S *p_blk = (NVTMPP_VB_BLK_S *)blk;
	void            *va;
	unsigned long    flags = 0;

	if (!nvtmpp_is_dynamic_map()) {
		return (void *)p_blk->buf_va;
	}
	loc_cpu(flags);
	if (is_lock_blk) {
		nvtmpp_vb_lock_block_p(USER_MODULE, blk);
	}
	if (p_blk->buf_va) {
		va = (void *)p_blk->buf_va;
		unl_cpu(flags);
		return va;
	}
	unl_cpu(flags);
	va = nvtmpp_ioremap_cache(p_blk->buf_addr, p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size());
	loc_cpu(flags);
	// check if buf_va already map by others
	if (p_blk->buf_va) {
		nvtmpp_vb_enqueue_va((UINT32)va);
		//DBG_DUMP("unmap va3 0x%x, size =0x%x\r\n", p_blk->buf_va, p_blk->blk_size);
		unl_cpu(flags);
		return (void *)p_blk->buf_va;
	}
	p_blk->buf_va = (UINT32)va;
	#if NVTMPP_DYNAMIC_VA_DEBUG
	{
		NVTMPP_VA_MAP_INFO map_info;

		map_info.pa = p_blk->buf_addr;
		map_info.va = p_blk->buf_va;
		map_info.size = p_blk->blk_size + nvtmpp_vb_get_blk_end_tag_size();
		nvtmpp_vb_map_list_add_p(&map_info);
	}
	#endif
	unl_cpu(flags);
	//DBG_DUMP("map pa = 0x%x, va = 0x%x, size 0x%x\r\n", (int)p_blk->buf_addr, (int)va, (int)p_blk->blk_size);
	return va;
}

void *nvtmpp_vb_blk_map_va(NVTMPP_VB_BLK blk)
{
	return nvtmpp_vb_lock_blk_map_va_p(blk, TRUE);
}

void nvtmpp_vb_blk_unmap_va(NVTMPP_VB_BLK blk)
{
	if (!nvtmpp_is_dynamic_map()) {
		return;
	}
	nvtmpp_vb_unlock_block(USER_MODULE, blk);
}

NVTMPP_ER nvtmpp_vb_add_err_cnt(NVTMPP_MODULE module, NVTMPP_MODULE_ER err_type)
{
	INT32              module_id;

	module_id = nvtmpp_vb_module_to_index(module);
	if (module_id < 0) {
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	//DBG_ERR("module_id = %d, err_type = %d\r\n", module_id, err_type);
	nvtmpp_vb_module_add_err_cnt(module_id, err_type);
	return NVTMPP_ER_OK;
}
