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
#if defined __KERNEL__
#include <linux/string.h> // for memset
#include <linux/kernel.h> // for snprintf
#else
#include <stdio.h>
#include <string.h>
#endif
#include "nvtmpp_init.h"
#include "nvtmpp_blk.h"
#include "nvtmpp_int.h"
#include "nvtmpp_heap.h"
#include "nvtmpp_module.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_platform.h"
#include "nvtmpp_log.h"

#define NVTMPP_BLK_TAG             MAKEFOURCC('N', 'B', 'L', 'K') ///< a key value
#define NVTMPP_BLK_END_TAG         MAKEFOURCC('N', 'B', 'K', 'E') ///< a key value
#define NVTMPP_BLK_END_TAG_SIZE    4
#define NVTMPP_BLK_END_TAG_CHK     0

UINT32 nvtmpp_vb_get_blk_header_size(void)
{
	return sizeof(NVTMPP_VB_BLK_S);
}

UINT32 nvtmpp_vb_get_blk_end_tag_size(void)
{
	return NVTMPP_BLK_END_TAG_SIZE;
}

NVTMPP_VB_BLK_S *nvtmpp_vb_init_blk(UINT32 pool, UINT32 blk_id, UINT32 blk_head_addr, UINT32 blk_total_size, BOOL is_map_blk)
{
	NVTMPP_VB_BLK_S *p_blk;
	UINT32 map_size;
	BOOL   is_dynamic_map;

	is_dynamic_map = nvtmpp_is_dynamic_map();
	map_size = NVTMPP_BLK_MAP_SIZE;
	if (!is_dynamic_map) {
		p_blk = (NVTMPP_VB_BLK_S *)nvtmpp_sys_pa2va(blk_head_addr);
	} else {
		p_blk = (NVTMPP_VB_BLK_S *)nvtmpp_ioremap_cache(blk_head_addr, map_size);
		if (p_blk == NULL) {
			return NULL;
		}
	}
	DBG_IND("map_size = 0x%x, p_blk= 0x%x\r\n", map_size, (int)p_blk);
	memset((void *)p_blk, 0x00, sizeof(NVTMPP_VB_BLK_S));
	p_blk->pool = pool;
	p_blk->blk_id = blk_id;
	p_blk->blk_head_addr = blk_head_addr;
	p_blk->buf_addr = blk_head_addr + sizeof(NVTMPP_VB_BLK_S);
	p_blk->blk_head_va = (UINT32)p_blk;
	DBG_IND("pool = 0x%x, blk_head_addr = 0x%x, buf_addr = 0x%x, blk_head_va = 0x%x, is_map_blk = %d\r\n",
				(int)pool, (int)blk_head_addr, (int)p_blk->buf_addr, (int) p_blk->blk_head_va, (int)is_map_blk);
	if (!is_dynamic_map) {
		p_blk->buf_va = (UINT32)p_blk + sizeof(NVTMPP_VB_BLK_S);
	} else if (is_map_blk) {
		p_blk->buf_va = (UINT32)nvtmpp_ioremap_cache(p_blk->buf_addr, blk_total_size - sizeof(NVTMPP_VB_BLK_S));
		if (p_blk->buf_va == 0) {
			nvtmpp_iounmap(p_blk);
			return NULL;
		}
	}
	DBG_IND("buf_va = 0x%x , blk_size = 0x%x\r\n", p_blk->buf_va, blk_total_size - sizeof(NVTMPP_VB_BLK_S));
	p_blk->init_tag = NVTMPP_BLK_TAG;
	p_blk->blk_size = blk_total_size - NVTMPP_BLK_END_TAG_SIZE - sizeof(NVTMPP_VB_BLK_S);
	#if NVTMPP_BLK_END_TAG_CHK
	p_blk->p_end_tag = (UINT32*)(blk_head_addr + blk_total_size - NVTMPP_BLK_END_TAG_SIZE);
	*p_blk->p_end_tag = NVTMPP_BLK_END_TAG;
	#endif
	return p_blk;
	//debug_dumpmem(blk_head_addr,0x100);
}

INT32 nvtmpp_vb_exit_blk(NVTMPP_VB_BLK_S *p_blk)
{
	if (nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)p_blk) == FALSE) {
		DBG_ERR("memory corruption, Invalid blk 0x%x\r\n", (int)p_blk);
		return -1;
	}
	if (nvtmpp_is_dynamic_map() && p_blk->buf_va != 0) {
		nvtmpp_iounmap((void *)p_blk->buf_va);
		p_blk->buf_va = 0;
	}
	p_blk->init_tag = 0;
	if (nvtmpp_is_dynamic_map()) {
		nvtmpp_iounmap((void *)p_blk);
	}
	return 0;
}


void nvtmpp_vb_insert_first_blk(NVTMPP_VB_BLK_S **start, NVTMPP_VB_BLK_S *node)
{
	node->next = *start;
	*start = node;
}

void nvtmpp_vb_remove_first_blk(NVTMPP_VB_BLK_S **start)
{
	// remove first free
	*start = (*start)->next;
}

void nvtmpp_vb_remove_blk(NVTMPP_VB_BLK_S **start, NVTMPP_VB_BLK_S *node)
{
	NVTMPP_VB_BLK_S *iterator = *start;
	UINT32           loop_count = 0;

	// if first node
	if (node == *start) {
		*start = iterator->next;
		return;
	}
	// not first node
	while (iterator && iterator->init_tag == NVTMPP_BLK_TAG) {
		if (iterator->next == node) {
			iterator->next = node->next;
			return;
		}
		iterator = iterator->next;
		if (loop_count++ > NVTMPP_VB_MAX_BLK_EACH_POOL) {
			DBG_ERR("link list error\r\n");
		}
	}
}

static NVTMPP_VB_BLK_S *nvtmpp_vb_blk2pblk(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk;

	p_blk = (NVTMPP_VB_BLK_S *)blk;
	return p_blk;
}

BOOL nvtmpp_vb_chk_blk_valid(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk;
	UINT32           pa;

	#if 1
	pa = nvtmpp_sys_va2pa((UINT32)blk);
	if (pa == 0) {
		return FALSE;
	}
	if (!nvtmpp_heap_chk_addr_valid(pa)) {
		return FALSE;
	}
	#endif
	if (blk & NVTMPP_BLK_HEADER_ALIGN_MASK) {
		return FALSE;
	}
	p_blk = nvtmpp_vb_blk2pblk(blk);
	if (p_blk->blk_head_va != (UINT32)blk) {
		DBG_ERR("blk corruption => blk_head_va=0x%x != blk=0x%x\r\n", (int)p_blk->blk_head_addr,
			    (int)blk);
		return FALSE;
	}
	if (p_blk->init_tag != NVTMPP_BLK_TAG) {
		DBG_ERR("blk corruption => blk_head_addr=0x%x, blk=0x%x, begin_tag 0x%x != 0x%x\r\n", (int)p_blk->blk_head_addr,
			    (int)blk, (int)p_blk->init_tag, NVTMPP_BLK_TAG);
		return FALSE;
	}
	#if NVTMPP_BLK_END_TAG_CHK
	if (p_blk->buf_addr + p_blk->blk_size != (UINT32)p_blk->p_end_tag) {
		DBG_ERR("blk corruption => blk=0x%x, blk_size = 0x%x, end_tag_addr = 0x%x\r\n", (int)p_blk->blk_head_addr,
			    (int)p_blk->blk_size, (int)p_blk->p_end_tag);
		return FALSE;
	}
	if (NVTMPP_BLK_END_TAG != (*p_blk->p_end_tag)) {
		DBG_ERR("blk corruption => blk_head_addr=0x%x, blk=0x%x, end_tag_addr = 0x%x, end_tag 0x%x != 0x%x\r\n", (int)p_blk->blk_head_addr,
			    (int)blk, (int)p_blk->p_end_tag, (int)*p_blk->p_end_tag , NVTMPP_BLK_END_TAG);
		return FALSE;
	}
	#endif
	return TRUE;
}

NVTMPP_ER  nvtmpp_vb_add_blk_ref(UINT32 module_id, NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk = nvtmpp_vb_blk2pblk(blk);

	DBG_IND("lock b module_id = %d, blk = 0x%x , ref_cnt = %d\r\n", module_id, (int)blk, p_blk->total_ref_cnt);

	if (module_id >= NVTMPP_MODULE_MAX_NUM) {
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	#if 0
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return NVTMPP_ER_BLK_UNEXIST;
	}
	#endif
	p_blk->module_ref_cnt[module_id]++;
	p_blk->total_ref_cnt++;
	if (p_blk->total_ref_cnt > p_blk->max_ref_cnt) {
		p_blk->max_ref_cnt = p_blk->total_ref_cnt;
	}
	DBG_IND("lock e module_id = %d, blk = 0x%x , ref_cnt = %d\r\n", module_id, (int)blk, p_blk->total_ref_cnt);
	return NVTMPP_ER_OK;
}

NVTMPP_ER  nvtmpp_vb_minus_blk_ref(UINT32 module_id, NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk = nvtmpp_vb_blk2pblk(blk);

	DBG_IND("unlock b module_id = %d, blk = 0x%x , ref_cnt = %d\r\n", module_id, (int)blk, p_blk->total_ref_cnt);

	if (module_id >= NVTMPP_MODULE_MAX_NUM) {
		return NVTMPP_ER_UNKNOWW_MODULE;
	}
	#if 0
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return NVTMPP_ER_BLK_UNEXIST;
	}
	#endif
	if (p_blk->total_ref_cnt == 0) {
		return NVTMPP_ER_BLK_UNLOCK_REF;
	}
	if (p_blk->module_ref_cnt[module_id] == 0) {
		return NVTMPP_ER_BLK_UNLOCK_MODULE_REF;
	}
	p_blk->module_ref_cnt[module_id]--;
	p_blk->total_ref_cnt--;
	if (p_blk->total_ref_cnt == 0) {
		DBG_IND("unlock e module_id = %d, blk = 0x%x , ref_cnt = %d\r\n", module_id, (int)blk, p_blk->total_ref_cnt);
		return NVTMPP_BLK_REF_REACH_ZERO;
	}

	DBG_IND("unlock e module_id = %d, blk = 0x%x , ref_cnt = %d\r\n", module_id, (int)blk, p_blk->total_ref_cnt);
	return NVTMPP_ER_OK;
}

UINT32  nvtmpp_vb_blk2pa(NVTMPP_VB_BLK blk)
{
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return 0;
	}
	return ((NVTMPP_VB_BLK_S *)blk)->buf_addr;
}

UINT32  nvtmpp_vb_blk2va(NVTMPP_VB_BLK blk)
{
	NVTMPP_VB_BLK_S *p_blk;
	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return 0;
	}
	p_blk = (NVTMPP_VB_BLK_S *)blk;
	return (p_blk->buf_va);
}

NVTMPP_VB_BLK  nvtmpp_vb_va2blk(UINT32 buf_va)
{
	if (!nvtmpp_is_dynamic_map()) {
		NVTMPP_VB_BLK_S *blk;

		if (buf_va & NVTMPP_BLK_ALIGN_MASK) {
			return NVTMPP_VB_INVALID_BLK;
		}
		blk = (NVTMPP_VB_BLK_S *)(buf_va - sizeof(NVTMPP_VB_BLK_S));
		if (!nvtmpp_vb_chk_blk_valid((NVTMPP_VB_BLK)blk)) {
			return NVTMPP_VB_INVALID_BLK;
		}
		return (NVTMPP_VB_BLK)blk;
	} else {
		UINT32 pa;

		pa = nvtmpp_sys_va2pa(buf_va);
		return  nvtmpp_sys_get_blk_by_pa(pa);
		//DBG_ERR("Not support\r\n");
		//return NVTMPP_ER_BLK_UNEXIST;
	}
}

NVTMPP_ER  nvtmpp_vb_dump_blk_ref(int (*dump)(const char *fmt, ...), NVTMPP_VB_BLK blk, UINT32 module_max_count, UINT32 *module_strlen_list)
{
	#define BUFF_LEN      200

	NVTMPP_VB_BLK_S *p_blk = nvtmpp_vb_blk2pblk(blk);
	UINT32           i;
	CHAR            fmtstr[10] = "%8ld ";
	int             total_len = 0, len, remain_len;
	CHAR            str[BUFF_LEN];

	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return NVTMPP_ER_BLK_UNEXIST;
	}
	remain_len = BUFF_LEN;
	len = snprintf(&str[total_len], remain_len,"      %3d    0x%08X   0x%08X   0x%08X   ", (int)p_blk->blk_id, (int)p_blk, (int)p_blk->buf_addr, (int)p_blk->want_size);
	if (len < 0 || len >= remain_len) {
		goto len_err;
	}
	total_len += len;
	remain_len -= len;
	str[BUFF_LEN - 1] = 0;
	for (i = 0; i < module_max_count; i++) {
		fmtstr[1] = module_strlen_list[i] + 0x30;
		len = snprintf(&str[total_len], remain_len, fmtstr, p_blk->module_ref_cnt[i]);
		if (len < 0 || len >= remain_len) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		str[BUFF_LEN - 1] = 0;
	}
	len = snprintf(&str[total_len], remain_len, "\r\n");
	if (len < 0 || len >= remain_len) {
		goto len_err;
	}
	total_len += len;
	remain_len -= len;
	str[BUFF_LEN - 1] = 0;
	dump(str);
	return NVTMPP_ER_OK;
len_err:
	dump("string buffer\r\n");
	return NVTMPP_ER_SYS;
}

NVTMPP_MODULE nvtmpp_vb_get_block_lastref_module(NVTMPP_VB_BLK blk)
{
	#if 0
	NVTMPP_VB_BLK_S *p_blk = nvtmpp_vb_blk2pblk(blk);

	if (!nvtmpp_vb_chk_blk_valid(blk)) {
		return 0;
	}
	return nvtmpp_vb_index_to_module(p_blk->last_ref_module);
	#else
	return 0;
	#endif
}




