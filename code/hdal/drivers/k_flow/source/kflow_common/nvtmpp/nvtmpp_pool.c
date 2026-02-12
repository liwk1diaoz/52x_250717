/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_pool.c
    @ingroup

    @brief      nvtmpp video buffer pool handling

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2017/02/013
*/
#if defined __KERNEL__
#include <linux/string.h>   // for strncpy, strncmp
#else
#include <string.h>         // for strncpy, strncmp
#endif
#include "nvtmpp_heap.h"
#include "nvtmpp_blk.h"
#include "nvtmpp_pool.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_platform.h"


#define NVTMPP_INITED_TAG       MAKEFOURCC('N', 'M', 'P', 'P') ///< a key value

UINT32 pool_structs_start_addr, pool_structs_size;

// common pool link list sort by blksize
void nvtmpp_vb_pool_insert_node_to_common(NVTMPP_VB_POOL_S **start, NVTMPP_VB_POOL_S *node)                              \
{
	NVTMPP_VB_POOL_S *iterator = *start;
	UINT32           loop_count = 0;

	// insert to first
	if (node->blk_size < iterator->blk_size) {
		node->next = *start;
		*start = node;
		return;
	}
	while (iterator && iterator->init_tag == NVTMPP_INITED_TAG) {
		// add to last
		if (iterator->next == NULL && node->blk_size >= iterator->blk_size) {
			iterator->next = node;
			return;
		}
		if (node->blk_size >= iterator->blk_size && iterator->next && node->blk_size < iterator->next->blk_size) {
			break;
		}
		iterator = iterator->next;
		if (loop_count++ > NVTMPP_VB_MAX_COMM_POOLS) {
			DBG_ERR("link list error\r\n");
		}
	}
	if (iterator) {
		// insert to middle
		node->next = iterator->next;
		iterator->next = node;
	}
}

#if 0
static void nvtmpp_vb_pool_remove_node(NVTMPP_VB_POOL_S **start, NVTMPP_VB_POOL_S *node)
{
	NVTMPP_VB_POOL_S *iterator = *start;

	// if first node
	if (node == *start) {
		*start = iterator->next;
		return;
	}
	// not first node
	while (iterator) {
		if (iterator->next == node) {
			iterator->next = node->next;
			return;
		}
		iterator = iterator->next;
	}
}
#endif


//
NVTMPP_VB_POOL_S *nvtmpp_vb_pool_search_match_size_from_common(NVTMPP_VB_POOL_S *start, UINT32 blk_size)
{
	NVTMPP_VB_POOL_S *iterator = start;
	UINT32           loop_count = 0;

	DBG_IND("iterator = 0x%x\r\n", (int)iterator);
	while (iterator && iterator->init_tag == NVTMPP_INITED_TAG) {
		DBG_IND("blk_size 0x%x, blk_free_cnt = %d\r\n", (int)iterator->blk_size, iterator->blk_free_cnt);
		if (iterator->blk_size >= blk_size && iterator->blk_free_cnt > 0) {
			return iterator;
		}
		iterator = iterator->next;
		DBG_IND("iterator = 0x%x\r\n", (int)iterator);
		if (loop_count++ > NVTMPP_VB_MAX_COMM_POOLS) {
			DBG_ERR("link list error\r\n");
		}
	}
	return NULL;
}

NVTMPP_VB_POOL_S *nvtmpp_vb_pool_search_common_by_pool_type(NVTMPP_VB_POOL_S *p_pool, UINT32 pool_count, UINT32 pool_type, UINT32 blk_size, NVTMPP_DDR ddr)
{
	UINT32               i, min_match_blk_size = 0xFFFFFFFF;
	BOOL                 has_match = FALSE;
	NVTMPP_VB_POOL_S     *best_match_pool;


	//DBG_DUMP("pool_count = %d, pool_type = 0x%08X, blk_size =0x%08X, ddr = %d\r\n", (int)pool_count, pool_type , blk_size, (int)ddr+1);
	for (i = 0; i < pool_count; i++) {
		//DBG_DUMP("pool id %d, pool_type = 0x%08X, blk_size =0x%08X, ddr = %d\r\n", i, p_pool->pool_type , p_pool->blk_size, (int)ddr+1);
		if (p_pool->pool_type == pool_type && p_pool->ddr == ddr) {
			if (p_pool->blk_size == blk_size && p_pool->blk_free_cnt > 0) {
				return p_pool;
			} else if (p_pool->blk_size > blk_size && p_pool->blk_free_cnt > 0) {
				if (p_pool->blk_size < min_match_blk_size) {
					min_match_blk_size = p_pool->blk_size;
					best_match_pool = p_pool;
					has_match = TRUE;
				}
			}
		}
		p_pool++;
	}
	if (has_match) {
		//DBG_DUMP("best_match_pool = 0x%x, min_match_blk_size = 0x%x\r\n", (int)best_match_pool, (int)min_match_blk_size);
		return best_match_pool;
	}
	return NULL;
}

NVTMPP_VB_POOL_S *nvtmpp_vb_get_free_pool(NVTMPP_VB_POOL_S  *vb_pools, UINT32 total_pools_cnt)
{
	UINT32             pool_id;
	NVTMPP_VB_POOL_S  *p_pool = vb_pools;

	for (pool_id = 0; pool_id < total_pools_cnt; pool_id++) {
		if (p_pool->init_tag != NVTMPP_INITED_TAG) {
			return p_pool;
		}
		p_pool++;
	}
	return NULL;
}


void nvtmpp_vb_pool_set_struct_range(UINT32 start_addr, UINT32 size)
{
	pool_structs_start_addr = start_addr;
	pool_structs_size = size;
}

NVTMPP_ER nvtmpp_vb_pool_init(NVTMPP_VB_POOL_S *p_pool, CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, NVTMPP_DDR ddr, BOOL is_common, UINT32 pool_type)
{
	UINT32 pool_va, pool_pa, pool_size,  blk_id, blk_head_addr, blk_total_size;
	NVTMPP_VB_BLK_S *p_prevblk = NULL, *p_blk;
	BOOL   is_map_blk = FALSE;

	if (blk_cnt < 1) {
		return NVTMPP_ER_PARM;
	}
	if (blk_cnt > NVTMPP_VB_MAX_BLK_EACH_POOL) {
		DBG_ERR("blk_cnt exceeds limit %d\r\n", NVTMPP_VB_MAX_BLK_EACH_POOL);
		return NVTMPP_ER_PARM;
	}
	blk_total_size = ALIGN_CEIL(blk_size + nvtmpp_vb_get_blk_header_size() + nvtmpp_vb_get_blk_end_tag_size(), NVTMPP_BLK_ALIGN);
	pool_size = blk_total_size * blk_cnt;
	if (pool_name && (!strncmp(pool_name, NVTMPP_TEMP_POOL_NAME, strlen(NVTMPP_TEMP_POOL_NAME)))) {
		pool_va = (UINT32) nvtmpp_heap_malloc_from_max_freeblk_end(ddr, pool_size);
	} else {
		pool_va = (UINT32) nvtmpp_heap_malloc(ddr, pool_size);
	}
	if (pool_va == 0) {
		return NVTMPP_ER_NOBUF;
	}
	pool_pa = nvtmpp_sys_va2pa(pool_va);
	DBG_IND("pool va= 0x%x, pa = 0x%x, ddr = %d, blk_size=0x%x, blk_total_size=0x%x, blk_cnt=%d\r\n",
				(int)pool_va, (int) pool_pa, ddr, blk_size, blk_total_size, blk_cnt);
	p_pool->pool_addr = pool_pa;
	p_pool->pool_va = pool_va;
	p_pool->pool_type = pool_type;
	p_pool->pool_size = pool_size;
	p_pool->blk_size = blk_size;
	p_pool->blk_cnt = blk_cnt;
	p_pool->blk_free_cnt = blk_cnt;
	p_pool->blk_min_free_cnt = blk_cnt;
	p_pool->ddr = ddr;
	p_pool->first_free_blk = (NVTMPP_VB_BLK_S *)(ALIGN_CEIL(pool_pa, NVTMPP_BLK_ALIGN));
	#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
	// first block header use heap header reserved area
	p_pool->first_free_blk--;
	#endif
	p_pool->first_blk = p_pool->first_free_blk;
	p_pool->blk_total_size = blk_total_size;
	p_pool->first_used_blk = NULL;
	p_pool->next = NULL;
	p_pool->is_common = is_common;
	p_pool->dump_max_cnt = 0;
	if (pool_name) {
		DBG_IND("pool_name = %s\r\n", pool_name);
		strncpy(p_pool->pool_name, pool_name, NVTMPP_VB_MAX_POOL_NAME + 1);
		p_pool->pool_name[NVTMPP_VB_MAX_POOL_NAME] = 0;
	} else {
		p_pool->pool_name[0] = 0;
	}
	// init all blocks in pool
	blk_head_addr = (UINT32)p_pool->first_free_blk;
	for (blk_id = 0; blk_id < blk_cnt; blk_id++) {
		DBG_IND("blk_id= %d , blk_head_addr = 0x%x \r\n", blk_id, (int)blk_head_addr);
		if (is_common && pool_type != POOL_TYPE_COMMON) {
			is_map_blk = TRUE;
		}
		p_blk = nvtmpp_vb_init_blk((UINT32)p_pool, blk_id, blk_head_addr, blk_total_size, is_map_blk);
		if (p_blk == NULL) {
			return NVTMPP_ER_NOBUF;
		}
		blk_head_addr += blk_total_size;
		if (blk_id == 0) {
			p_pool->first_blk = p_pool->first_free_blk = p_blk;
		} else {
			p_prevblk->next = p_blk;
		}
		p_prevblk = p_blk;
	}
	DBG_IND("first_blk= 0x%x\r\n", (int)p_pool->first_blk);
	p_blk->next = NULL;
	p_pool->init_tag = NVTMPP_INITED_TAG;
	return NVTMPP_ER_OK;
}

INT32 nvtmpp_vb_pool_exit(NVTMPP_VB_POOL_S *p_pool)
{
	NVTMPP_VB_BLK_S *iterator;
	NVTMPP_VB_BLK_S *p_next;
	UINT32           loop_count = 0;

	// exit free blocks in pool
	iterator = p_pool->first_free_blk;
	while (iterator) {
		p_next = iterator->next;
		if (nvtmpp_vb_exit_blk(iterator) < 0)
			goto heap_free;
		iterator = p_next;
		if (loop_count++ > NVTMPP_VB_MAX_BLK_EACH_POOL) {
			DBG_ERR("link list error\r\n");
			goto heap_free;
		}
	}
	// exit used blocks in pool
	iterator = p_pool->first_used_blk;
	loop_count = 0;
	while (iterator) {
		p_next = iterator->next;
		if (nvtmpp_vb_exit_blk(iterator) < 0)
			goto heap_free;
		iterator = p_next;
		if (loop_count++ > NVTMPP_VB_MAX_BLK_EACH_POOL) {
			DBG_ERR("link list error\r\n");
			goto heap_free;
		}
	}
heap_free:
	// free heap memory of pool
	if (nvtmpp_heap_chk_addr_valid(p_pool->pool_addr) == FALSE) {
		DBG_ERR("Invalid pa 0x%x\r\n", (int)p_pool->pool_addr);
		return -1;
	}
	if (nvtmpp_heap_free(p_pool->ddr, (void *)p_pool->pool_va) < 0) {
		return -1;
	}
	// clear pool tag
	p_pool->init_tag = 0;
	return 0;
}

BOOL nvtmpp_vb_pool_chk_valid(UINT32 pool_addr)
{
	NVTMPP_VB_POOL_S     *p_pool;

	if (pool_addr < pool_structs_start_addr || (pool_addr > (pool_structs_start_addr + pool_structs_size))) {
		//DBG_ERR("pool = 0x%x, pool_structs_start_addr=0x%x, pool_structs_size=0x%x\r\n", (int)pool, pool_structs_start_addr, pool_structs_size);
		return FALSE;
	}
	p_pool = (NVTMPP_VB_POOL_S *)pool_addr;
	if (p_pool->init_tag == NVTMPP_INITED_TAG) {
		return TRUE;
	}
	return FALSE;
}

void nvtmpp_vb_pool_set_dump_max_cnt(NVTMPP_VB_POOL_S   *p_pool, UINT32 dump_max_cnt)
{
	p_pool->dump_max_cnt = dump_max_cnt;
}



NVTMPP_VB_BLK  nvtmpp_vb_get_free_block_from_pool(NVTMPP_VB_POOL_S   *p_pool, UINT32 want_size)
{
	NVTMPP_VB_BLK_S    *p_blk = p_pool->first_free_blk;

	if (!nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)p_blk)) {
		DBG_ERR("Invalid blk = 0x%x\r\n", (int)p_blk);
		return NVTMPP_VB_INVALID_BLK;
	}
	if (p_blk->total_ref_cnt != 0) {
		DBG_ERR("blk = 0x%x, ref_cnt != 0\r\n", (int)p_blk);
		return NVTMPP_VB_INVALID_BLK;
	}
	p_blk->want_size = want_size;
	nvtmpp_vb_remove_first_blk(&p_pool->first_free_blk);
	nvtmpp_vb_insert_first_blk(&p_pool->first_used_blk, p_blk);
	p_pool->blk_free_cnt--;
	if (p_pool->blk_free_cnt < p_pool->blk_min_free_cnt) {
		p_pool->blk_min_free_cnt = p_pool->blk_free_cnt;
	}
	DBG_IND("blk = 0x%x\r\n", (int)p_blk);
	return (NVTMPP_VB_BLK)p_blk;
}

void nvtmpp_vb_release_block_back_pool(NVTMPP_VB_POOL_S   *p_pool, NVTMPP_VB_BLK_S *p_blk)
{
	nvtmpp_vb_remove_blk(&p_pool->first_used_blk, p_blk);
	nvtmpp_vb_insert_first_blk(&p_pool->first_free_blk, p_blk);
}


UINT32 nvtmpp_vb_get_pool_reserved_size(void)
{
	// reserved for block align
	return NVTMPP_BLK_ALIGN;
}







