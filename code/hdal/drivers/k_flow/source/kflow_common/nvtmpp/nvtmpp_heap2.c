/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_heap.c

    @brief      nvtmpp heap memory alloc , free.


    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/

#if defined __KERNEL__
#include <linux/string.h>   // for memset
#else
#include <string.h>
#endif

#include "nvtmpp_init.h"
#include "kflow_common/nvtmpp.h"
#include "nvtmpp_id.h"
#include "nvtmpp_heap.h"
#include "nvtmpp_platform.h"
#if 1
#include "nvtmpp_debug.h"
#else
#define THIS_DBGLVL         NVT_DBG_MSG
#define __MODULE__          nvtmpp_heap
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
unsigned int nvtmpp_heap_debug_level = __DBGLVL__;
#endif


#define NVTMPP_HEAP_INITED_TAG       MAKEFOURCC('N', 'H', 'P', 'B') ///< a key value
#define NVTMPP_HEAP_END_TAG          MAKEFOURCC('N', 'H', 'P', 'E') ///< a key value
#define NVTMPP_HEAP_FREE_TAG         MAKEFOURCC('F', 'R', 'E', 'E') ///< a key value
#define NVTMPP_HEAP_END_TAG_CHK      1


typedef struct {
	UINT32              init_tag;            /*<< The heap init tag. */
	UINT32              pa_start;
	UINT32              pa_end;
	UINT32              va_start;
	NVTMPP_HEAP_LINK_S  free_block_start;
	NVTMPP_HEAP_LINK_S *free_block_end;
	UINT32              freebytes_remain;
	NVTMPP_HEAP_LINK_S  used_block_start;
	NVTMPP_HEAP_LINK_S *used_block_end;
	NVTMPP_HEAP_LINK_S *link_start;
	NVTMPP_HEAP_LINK_S *link_end;
	NVTMPP_HEAP_LINK_S *link_current;
	UINT32              link_max_num;
} HEAP_OBJ_S;

static  HEAP_OBJ_S g_heap_obj[NVTMPP_DDR_MAX_NUM];

#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
STATIC_ASSERT((sizeof(NVTMPP_HEAP_BLOCK_S)) == NVTMPP_HEAP_ALIGN/2);
#else
STATIC_ASSERT((sizeof(NVTMPP_HEAP_BLOCK_S)) == NVTMPP_HEAP_ALIGN);
#endif





static void _nvtmpp_heap_lock(void)
{
	SEM_WAIT(NVTMPP_HEAP_SEM_ID);
}

static void _nvtmpp_heap_unlock(void)
{
	SEM_SIGNAL(NVTMPP_HEAP_SEM_ID);
}

static NVTMPP_HEAP_LINK_S *_nvtmpp_heap_get_free_link(HEAP_OBJ_S *heap_obj)
{
	UINT32 i;

	for (i = 0; i < heap_obj->link_max_num; i++) {
		if (heap_obj->link_current->init_tag == 0)
			return heap_obj->link_current;
		heap_obj->link_current++;
		if (heap_obj->link_current > heap_obj->link_end)
			heap_obj->link_current = heap_obj->link_start;
	}
	return NULL;
}

static void _nvtmpp_heap_release_link(NVTMPP_HEAP_LINK_S *link)
{
	NVTMPP_HEAP_BLOCK_S *p_block;

	DBG_IND("link = 0x%x, tag = 0x%x, block_addr = 0x%x\r\n", (int)link, (int)link->init_tag, (int)link->block_addr);
	p_block = (NVTMPP_HEAP_BLOCK_S *)link->block_va;
	p_block->init_tag = NVTMPP_HEAP_FREE_TAG;
	if (nvtmpp_is_dynamic_map()) {
		nvtmpp_iounmap((void *)link->block_va);
	}
	link->init_tag = 0;
}

static void _nvtmpp_heap_dump_link(HEAP_OBJ_S *heap_obj)
{
	debug_dumpmem((UINT32)heap_obj->link_start, sizeof(NVTMPP_HEAP_LINK_S) * heap_obj->link_max_num);
}

static INT32 _nvtmpp_heap_chk_block(HEAP_OBJ_S *heap_obj, NVTMPP_HEAP_BLOCK_S *p_block, BOOL err_abort)
{
	INT32                ret = 0;

	if (p_block->init_tag == NVTMPP_HEAP_FREE_TAG) {
		DBG_ERR("block was already freed, addr = 0x%x\r\n", (int)p_block);
		ret = -1;
		goto chk_end;
	}
	if (p_block->init_tag != NVTMPP_HEAP_INITED_TAG) {
		DBG_ERR("block memory corruption, addr = 0x%x, start tag 0x%x != 0x%x\r\n", (int)p_block, (int)p_block->init_tag, (int)NVTMPP_HEAP_INITED_TAG);
		debug_dumpmem((UINT32)p_block, 0x80);
		ret = -1;
		goto chk_end;
	}
	#if NVTMPP_HEAP_END_TAG_CHK
	if (nvtmpp_is_dynamic_map()) {
		return ret;
	}
	if ((UINT32)p_block->end_tag < p_block->block_va || (UINT32)p_block->end_tag > p_block->block_va + p_block->block_size) {
		DBG_ERR("block memory corruption, Invalid p_block->end_tag addr = 0x%x, block_va = 0x%x, size 0x%x \r\n", (int)p_block->end_tag, (int)p_block->block_va, (int)p_block->block_size);
		debug_dumpmem((UINT32)p_block, 0x100);
		ret = -1;
		goto chk_end;
	}
	if ((*p_block->end_tag) != NVTMPP_HEAP_END_TAG) {
		DBG_ERR("block memory corruption, addr = 0x%x, end tag addr = 0x%x, value 0x%x != 0x%x\r\n", (int)p_block, (int)p_block->end_tag, (int)*p_block->end_tag, (int)NVTMPP_HEAP_END_TAG);
		debug_dumpmem((UINT32)p_block->end_tag-0x80, 0x100);
		ret = -1;
		goto chk_end;
	}
	#endif
chk_end:
	if (ret < 0) {
		nvtmpp_dump_mem_range(vk_printk);
		nvtmpp_dump_stack();
		if (TRUE == err_abort) {
			abort();
		}
	}
	return ret;
}


static INT32 _nvtmpp_heap_chk_link(HEAP_OBJ_S *heap_obj, NVTMPP_HEAP_LINK_S *link)
{
	INT32                ret = 0;

	if (link->init_tag != NVTMPP_HEAP_INITED_TAG) {
		DBG_ERR("link memory corruption, addr = 0x%x, tag 0x%x != 0x%x\r\n", (int)link, (int)link->init_tag, (int)NVTMPP_HEAP_INITED_TAG);
		_nvtmpp_heap_dump_link(heap_obj);
		ret = -1;
		goto chk_end;
	}
chk_end:
	if (ret < 0) {
		nvtmpp_dump_mem_range(vk_printk);
	}
	return ret;
}


static void _nvtmpp_heap_insert_block_to_freelist(HEAP_OBJ_S *heap_obj, NVTMPP_HEAP_LINK_S *p_block2insert)
{
	UINT32         loop_count = 0;
	NVTMPP_HEAP_LINK_S *iterator;
	NVTMPP_HEAP_BLOCK_S *p_block;
	unsigned char *puc;

	DBG_IND("p_block2insert = 0x%x\r\n", (int)p_block2insert);
	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for (iterator = &heap_obj->free_block_start; iterator->next->block_addr < p_block2insert->block_addr; iterator = iterator->next) {
		/* Nothing to do here, just iterate to the right position. */
		//DBG_IND("iterator = 0x%x, block_addr = 0x%x, block_size = 0x%x\r\n",iterator,iterator->block_addr,iterator->block_size);
		if (loop_count++ > heap_obj->link_max_num) {
			DBG_ERR("memory corruption, link list error\r\n");
			abort();
			return;
		}
		if (loop_count > 1 && _nvtmpp_heap_chk_link(heap_obj, iterator) < 0)
			return;
	}
	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = (unsigned char *) iterator->block_addr;
	if (((puc + iterator->block_size) == (unsigned char *) p_block2insert->block_addr) &&
		(iterator->block_addr != p_block2insert->block_addr)) {
		iterator->block_size += p_block2insert->block_size;
		DBG_IND("1 Merge block 0x%x, size 0x%x, block 0x%x, size 0x%x \r\n",iterator->block_addr,iterator->block_size,p_block2insert->block_addr,p_block2insert->block_size);
		p_block = (NVTMPP_HEAP_BLOCK_S *)iterator->block_va;
		p_block->block_size = iterator->block_size;
		_nvtmpp_heap_release_link(p_block2insert);
		p_block2insert = iterator;
	}
	DBG_IND("iterator = 0x%x, iterator->next =0x%x, p_block2insert = 0x%x\r\n", (int)iterator, (int)iterator->next, (int)p_block2insert);

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = (unsigned char *) p_block2insert->block_addr;
	if ((puc + p_block2insert->block_size) == (unsigned char *) iterator->next->block_addr) {
		if (iterator->next != heap_obj->free_block_end) {
			DBG_IND("2 Merge block 0x%x, size 0x%x, block 0x%x, size 0x%x \r\n",p_block2insert->block_addr,p_block2insert->block_size,iterator->next->block_addr,iterator->next->block_size);
			/* Form one big block from the two blocks. */
			p_block2insert->block_size += iterator->next->block_size;
			_nvtmpp_heap_release_link(iterator->next);
			p_block2insert->next = iterator->next->next;
			p_block = (NVTMPP_HEAP_BLOCK_S *)p_block2insert->block_va;
			p_block->block_size = p_block2insert->block_size;
		} else {
			p_block2insert->next = heap_obj->free_block_end;
		}
	} else {
		p_block2insert->next = iterator->next;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if (iterator != p_block2insert) {
		iterator->next = p_block2insert;
	}
	//nvtmpp_heap_dump(0);
}

static void _nvtmpp_heap_insert_block_to_usedlist(HEAP_OBJ_S *heap_obj, NVTMPP_HEAP_LINK_S *p_block2insert)
{
	UINT32         loop_count = 0;
	NVTMPP_HEAP_LINK_S *iterator;

	DBG_IND("p_block2insert = 0x%x\r\n", (int)p_block2insert);
	for (iterator = &heap_obj->used_block_start; iterator->next->block_addr < p_block2insert->block_addr; iterator = iterator->next) {
		/* Nothing to do here, just iterate to the right position. */
		//DBG_IND("iterator = 0x%x, block_addr = 0x%x, block_size = 0x%x\r\n",iterator,iterator->block_addr,iterator->block_size);
		if (loop_count++ > heap_obj->link_max_num) {
			DBG_ERR("memory corruption, link list error\r\n");
			abort();
			return;
		}
		if (loop_count > 1 && _nvtmpp_heap_chk_link(heap_obj, iterator) < 0)
			return;
	}

	DBG_IND("iterator= 0x%x, p_block2insert= 0x%x\r\n", (int)iterator, (int)p_block2insert);
	p_block2insert->next = iterator->next;
	if (iterator != p_block2insert) {
		DBG_IND("iterator->next = 0x%x\r\n", (int)iterator->next);
		iterator->next = p_block2insert;
	}
	//nvtmpp_heap_dump(0);
}

static INT32 _nvtmpp_heap_remove_block_from_usedlist(HEAP_OBJ_S *heap_obj, NVTMPP_HEAP_LINK_S *node)
{
	UINT32              loop_count = 0;
	NVTMPP_HEAP_LINK_S *iterator = &heap_obj->used_block_start;

	DBG_IND("node = 0x%x\r\n", (int)node);
	while (iterator) {
		//DBG_IND("iterator = 0x%x, block_addr = 0x%x, block_size = 0x%x\r\n",iterator,iterator->block_addr,iterator->block_size);
		if (loop_count++ > heap_obj->link_max_num) {
			DBG_ERR("memory corruption, link list error\r\n");
			abort();
			return -1;
		}
		if (loop_count > 1 && _nvtmpp_heap_chk_link(heap_obj, iterator) < 0)
			return -1;
		if (iterator->next == node) {
			iterator->next = node->next;
			return 0;
		}
		iterator = iterator->next;
	}
	DBG_ERR("block 0x%x not in usedlist\r\n", node->block_addr);
	return -1;
	//nvtmpp_heap_dump(0);
}


static HEAP_OBJ_S *_nvtmpp_heap_getobj(NVTMPP_DDR ddr)
{
	if (ddr >= NVTMPP_DDR_MAX) {
		return NULL;
	}
	if (g_heap_obj[ddr].init_tag == NVTMPP_HEAP_INITED_TAG) {
		return &g_heap_obj[ddr];
	} else {
		return NULL;
	}
}

static UINT32 _nvtmpp_heap_get_min_size(UINT32 max_block_num)
{
	return ((sizeof(NVTMPP_HEAP_LINK_S) * (max_block_num << 1)) + HEAP_MIN_BLOCK_SIZE);
}

INT32 nvtmpp_heap_init(NVTMPP_DDR ddr, NVTMPP_MEMINFO_S  *heap_mem, UINT32 max_block_num)
{
	NVTMPP_HEAP_LINK_S *p_first_free_link;
	NVTMPP_HEAP_BLOCK_S *p_first_free_block, *p_last_free_block;

	UINT32          aligned_heap, heap_end;
	UINT32          heap_size, heap_pa, heap_va;
	HEAP_OBJ_S     *heap_obj;

	if (ddr >= NVTMPP_DDR_MAX) {
		return -1;
	}
	if (heap_mem == NULL) {
		return -1;
	}
	DBG_IND("ddr%d, heap phys_addr=0x%x, size=0x%x\r\n", ddr + 1, (int)heap_mem->phys_addr, (int)heap_mem->size);

	heap_obj = &g_heap_obj[ddr];
	if (heap_mem->size < _nvtmpp_heap_get_min_size(max_block_num))
		return -1;
	_nvtmpp_heap_lock();
	heap_obj->link_max_num = max_block_num << 1;
	heap_pa = ALIGN_CEIL(heap_mem->phys_addr, 4);
	if (heap_mem->virt_addr) {
		heap_va = ALIGN_CEIL(heap_mem->virt_addr, 4);
	} else {
		heap_va = (UINT32)nvtmpp_ioremap_cache(heap_pa, sizeof(NVTMPP_HEAP_LINK_S) * heap_obj->link_max_num);
		if (heap_va == 0) {
			return -1;
		}
	}
	heap_obj->link_start = (NVTMPP_HEAP_LINK_S *)heap_va;
	heap_obj->link_end   = heap_obj->link_start + heap_obj->link_max_num -1;
	memset(heap_obj->link_start, 0x00, (sizeof(NVTMPP_HEAP_LINK_S) * heap_obj->link_max_num));
	DBG_IND("link_start=0x%x, link_end=0x%x, link_max_num = %d\r\n", (int)heap_obj->link_start, (int)heap_obj->link_end, heap_obj->link_max_num);
	heap_pa += (sizeof(NVTMPP_HEAP_LINK_S) * heap_obj->link_max_num);

	/* Ensure the heap starts on a correctly aligned boundary. */
	aligned_heap = ALIGN_CEIL(heap_pa, NVTMPP_HEAP_ALIGN) + NVTMPP_HEAP_START_ADDR_OFFSET;
	heap_size = heap_mem->size + heap_mem->phys_addr - aligned_heap;
	heap_obj->pa_start = heap_mem->phys_addr;
	heap_obj->pa_end   = heap_mem->phys_addr + heap_mem->size;
	heap_obj->va_start = heap_mem->virt_addr;
	DBG_IND("aligned_heap=0x%x, heap_size=0x%x\r\n", (int)aligned_heap, (int)heap_size);


	heap_obj->free_block_start.init_tag = NVTMPP_HEAP_INITED_TAG;
	heap_obj->free_block_start.block_addr = aligned_heap;
	heap_obj->free_block_start.block_size = 0;
	heap_obj->free_block_start.next       = heap_obj->link_start;
	heap_obj->link_start->block_addr      = aligned_heap;
	heap_obj->link_start->block_size      = heap_size;


	heap_end = aligned_heap + heap_size - HEAP_STRUCT_SIZE;
	heap_obj->free_block_end = heap_obj->link_start+1;
	heap_obj->free_block_end->init_tag   = NVTMPP_HEAP_INITED_TAG;
	heap_obj->free_block_end->block_addr = heap_end;
	heap_obj->free_block_end->block_size = 0;
	heap_obj->free_block_end->next = NULL;
	heap_obj->link_current  = heap_obj->free_block_end + 1;

	if (heap_mem->virt_addr) {
		p_last_free_block = (NVTMPP_HEAP_BLOCK_S *)(heap_mem->virt_addr - heap_mem->phys_addr + heap_end);
	} else {
		p_last_free_block = (NVTMPP_HEAP_BLOCK_S *)nvtmpp_ioremap_cache(heap_end, sizeof(NVTMPP_HEAP_BLOCK_S));
	}
	if (p_last_free_block == NULL) {
		return -1;
	}
	p_last_free_block->init_tag = NVTMPP_HEAP_INITED_TAG;
	p_last_free_block->block_addr = heap_obj->free_block_end->block_addr;
	p_last_free_block->block_va = heap_obj->free_block_end->block_va = (UINT32)p_last_free_block;
	p_last_free_block->block_size = heap_obj->free_block_end->block_size;
	p_last_free_block->link = heap_obj->free_block_end;

	heap_obj->used_block_start.init_tag = NVTMPP_HEAP_INITED_TAG;
	heap_obj->used_block_end = heap_obj->free_block_end;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	p_first_free_link = heap_obj->link_start;
	p_first_free_link->init_tag   = NVTMPP_HEAP_INITED_TAG;
	p_first_free_link->block_addr = aligned_heap;
	p_first_free_link->block_size = heap_size - HEAP_STRUCT_SIZE;
	p_first_free_link->next = heap_obj->free_block_end;
	if (heap_mem->virt_addr) {
		p_first_free_block = (NVTMPP_HEAP_BLOCK_S *)(heap_mem->virt_addr - heap_mem->phys_addr + p_first_free_link->block_addr);
	} else {
		p_first_free_block = (NVTMPP_HEAP_BLOCK_S *)nvtmpp_ioremap_cache(p_first_free_link->block_addr, NVTMPP_HEAP_ALIGN);
	}
	if (p_first_free_block == NULL) {
		return -1;
	}
	p_first_free_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
	p_first_free_block->block_addr = p_first_free_link->block_addr;
	p_first_free_block->block_va = p_first_free_link->block_va = (UINT32)p_first_free_block;
	p_first_free_block->block_size = p_first_free_link->block_size;
	p_first_free_block->link = p_first_free_link;
	memset(p_first_free_block->reserved, 0x00, sizeof(p_first_free_block->reserved));

	heap_obj->used_block_start.next = (void *) heap_obj->free_block_end;
	heap_obj->used_block_start.block_size = 0;
	heap_obj->freebytes_remain = heap_size - HEAP_STRUCT_SIZE;
	heap_obj->init_tag = NVTMPP_HEAP_INITED_TAG;
	_nvtmpp_heap_unlock();
	//nvtmpp_heap_dump(0);
	return 0;
}

void nvtmpp_heap_exit(NVTMPP_DDR ddr)
{
	if (ddr >= NVTMPP_DDR_MAX) {
		return;
	}
	_nvtmpp_heap_lock();
	g_heap_obj[ddr].init_tag = 0;
	if (nvtmpp_is_dynamic_map()) {
		nvtmpp_iounmap(g_heap_obj[ddr].link_start);
	}
	_nvtmpp_heap_unlock();
}

static void nvtmpp_heap_fill_endtag(UINT32 *end_tag)
{
	#if NVTMPP_HEAP_END_TAG_CHK
	if (!nvtmpp_is_dynamic_map()) {
		*end_tag       = NVTMPP_HEAP_END_TAG;
		*(end_tag+1)   = NVTMPP_HEAP_END_TAG;
		*(end_tag+2)   = NVTMPP_HEAP_END_TAG;
		*(end_tag+3)   = NVTMPP_HEAP_END_TAG;
	}
	#endif
}

void *nvtmpp_heap_malloc(NVTMPP_DDR ddr, UINT32 wanted_size)
{
	NVTMPP_HEAP_LINK_S *p_block_link, *p_previous_block_link, *p_new_block_link;
	NVTMPP_HEAP_BLOCK_S *p_block, *p_new_block;
	void           *pv_return = NULL;
	HEAP_OBJ_S     *heap_obj;
	UINT32         align_wanted_size;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return NULL;
	}

	DBG_IND("ddr%d, wanted_size=0x%x\r\n", ddr + 1, (int)wanted_size);
	_nvtmpp_heap_lock();
	{
		/* The wanted size is increased so it can contain a xBlockLink
		structure in addition to the requested amount of bytes. */
		if (wanted_size > 0) {

			//#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
			//wanted_size += HEAP_STRUCT_SIZE;
			//#else
			wanted_size += (HEAP_STRUCT_SIZE+HEAP_END_TAG_SIZE);
			//#endif
			/* Byte alignment required. */
			align_wanted_size = (ALIGN_CEIL(wanted_size, NVTMPP_HEAP_ALIGN));

			/* Blocks are stored in byte order - traverse the list from the start
			(smallest) block until one of adequate size is found. */
			p_previous_block_link = &heap_obj->free_block_start;
			p_block_link = p_previous_block_link->next;


			DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x, wanted_size=0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size, (int)wanted_size);

			while ((p_block_link->block_size < align_wanted_size) && (p_block_link->next != NULL)) {
				p_previous_block_link = p_block_link;
				p_block_link = p_block_link->next;
				DBG_IND("Next p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
			}

			/* If we found the end marker then a block of adequate size was not found. */
			if (p_block_link != heap_obj->free_block_end) {
				/* Return the memory space - jumping over the xBlockLink structure
				at its start. */
				pv_return = (void *)(p_previous_block_link->next->block_va + HEAP_STRUCT_SIZE);

				/* This block is being returned for use so must be taken out of the
				list of free blocks. */
				p_previous_block_link->next = p_block_link->next;
				/* If the block is larger than required it can be split into two. */
				if ((p_block_link->block_size - align_wanted_size) > HEAP_MIN_BLOCK_SIZE) {
					/* This block is to be split into two.  Create a new block
					following the number of bytes requested. The void cast is
					used to prevent byte alignment warnings from the compiler. */
					p_new_block_link = _nvtmpp_heap_get_free_link(heap_obj);
					if (p_new_block_link == NULL) {
						DBG_ERR("No free link\r\n");
						_nvtmpp_heap_unlock();
						return NULL;
					}
					p_new_block_link->init_tag = NVTMPP_HEAP_INITED_TAG;
					p_new_block_link->block_addr = p_block_link->block_addr + align_wanted_size;

					/* Calculate the sizes of two blocks split from the single
					block. */
					p_new_block_link->block_size = p_block_link->block_size - align_wanted_size;
					p_block_link->block_size = align_wanted_size;
					p_block = (NVTMPP_HEAP_BLOCK_S *)p_block_link->block_va;
					p_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_block->block_size = p_block_link->block_size;
					p_block->end_tag    = (UINT32 *)(p_block_link->block_va+wanted_size-HEAP_END_TAG_SIZE);
					DBG_IND("1 block = 0x%x, size= 0x%x, end_tag addr= 0x%x\r\n", (int)p_block_link->block_va, (int)p_block_link->block_size, (int)p_block->end_tag);
					nvtmpp_heap_fill_endtag(p_block->end_tag);

					DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
					if (heap_obj->va_start) {
						p_new_block = (NVTMPP_HEAP_BLOCK_S *)(heap_obj->va_start - heap_obj->pa_start + p_new_block_link->block_addr);
					} else {
						p_new_block = (NVTMPP_HEAP_BLOCK_S *)nvtmpp_ioremap_cache(p_new_block_link->block_addr, NVTMPP_HEAP_ALIGN);
					}
					p_new_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_new_block->block_addr = p_new_block_link->block_addr;
					p_new_block->block_va   = (UINT32)p_new_block;
					p_new_block->block_size = p_new_block_link->block_size;
					p_new_block->link       = p_new_block_link;

					p_new_block_link->block_va = p_new_block->block_va;
					memset(p_new_block->reserved, 0x00, sizeof(p_new_block->reserved));
					/* Insert the new block into the list of free blocks. */
					_nvtmpp_heap_insert_block_to_freelist(heap_obj, (p_new_block_link));
				} else {
					p_block = (NVTMPP_HEAP_BLOCK_S *)p_block_link->block_va;
					p_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_block->block_size = p_block_link->block_size;
					p_block->end_tag    = (UINT32 *)(p_block_link->block_va+wanted_size-HEAP_END_TAG_SIZE);
					DBG_IND("2 block = 0x%x, size= 0x%x, end_tag addr= 0x%x\r\n", (int)p_block_link->block_va, (int)p_block_link->block_size, (int)p_block->end_tag);
					nvtmpp_heap_fill_endtag(p_block->end_tag);
				}
				heap_obj->freebytes_remain -= p_block_link->block_size;
				DBG_IND("freebytes_remain=0x%x\r\n", (int)heap_obj->freebytes_remain);
				_nvtmpp_heap_insert_block_to_usedlist(heap_obj, p_block_link);
			}
		}
	}
	//nvtmpp_dump_mem_range(debug_msg);
	_nvtmpp_heap_unlock();
	if (pv_return == NULL) {
		DBG_IND("ddr%d, wanted_size=0x%x , free_remain=0x%x, max_free_blk=0x%x\r\n", ddr + 1, (int)wanted_size, (int)heap_obj->freebytes_remain, (int)nvtmpp_heap_get_max_free_block_size(ddr));
	}
	DBG_IND("pvReturn = 0x%x\r\n", (int)pv_return);
	return pv_return;
}

void *nvtmpp_heap_malloc_from_max_freeblk_end(NVTMPP_DDR ddr, UINT32 wanted_size)
{
	NVTMPP_HEAP_LINK_S *p_block_link, *p_previous_block_link, *p_new_block_link;
	NVTMPP_HEAP_BLOCK_S *p_block, *p_new_block;
	void           *pv_return = NULL;
	HEAP_OBJ_S     *heap_obj;
	UINT32          align_wanted_size; //, max_block_size;
	NVTMPP_HEAP_LINK_S *p_candidate_prev_link, *p_candidate_link;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return NULL;
	}

	DBG_IND("ddr%d, wanted_size=0x%x\r\n", ddr + 1, (int)wanted_size);
	_nvtmpp_heap_lock();
	{
		/* The wanted size is increased so it can contain a xBlockLink
		structure in addition to the requested amount of bytes. */
		if (wanted_size > 0) {
			//#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
			//wanted_size += HEAP_STRUCT_SIZE;
			//#else
			wanted_size += (HEAP_STRUCT_SIZE+HEAP_END_TAG_SIZE);
			//#endif
			/* Byte alignment required. */
			align_wanted_size = (ALIGN_CEIL(wanted_size, NVTMPP_HEAP_ALIGN));

			/* Blocks are stored in byte order - traverse the list from the start
			(smallest) block until one of adequate size is found. */
			p_previous_block_link = &heap_obj->free_block_start;
			p_block_link = p_previous_block_link->next;
			//max_block_size = p_block_link->block_size;

			DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x, wanted_size=0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size, (int)wanted_size);

			#if 0
			// find max free block
			while (p_block_link->next != NULL) {
				p_block_link = p_block_link->next;
				if (p_block_link->block_size > max_block_size)
					max_block_size = p_block_link->block_size;
				DBG_IND("Next p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
			}
			p_previous_block_link = &heap_obj->free_block_start;
			p_block_link = p_previous_block_link->next;
			while (p_block_link->next != NULL) {
				if (p_block_link->block_size == max_block_size)
					break;
				p_previous_block_link = p_block_link;
				p_block_link = p_block_link->next;
				DBG_IND("Next p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
			}
			#else
			// find last free block
			p_candidate_link = p_block_link;
			p_candidate_prev_link = p_previous_block_link;
			while (p_block_link->next != NULL) {
				DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
				if (p_block_link->block_size >= align_wanted_size && p_block_link->block_addr > p_candidate_link->block_addr) {
					p_candidate_link = p_block_link;
					p_candidate_prev_link = p_previous_block_link;
					DBG_IND("p_candidate_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_candidate_link, (int)p_candidate_link->block_addr, (int)p_candidate_link->block_size);
				}
				p_previous_block_link = p_block_link;
				p_block_link = p_block_link->next;
			}
			#endif
			p_block_link = p_candidate_link;
			p_previous_block_link = p_candidate_prev_link;
			DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
			DBG_IND("p_previous_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_previous_block_link, (int)p_previous_block_link->block_addr, (int)p_previous_block_link->block_size);
			// the block size < want size
			if (p_block_link->block_size < align_wanted_size) {
				pv_return = NULL;
			} else {
				/* If we found the end marker then a block of adequate size was not found. */
				/* This block is being returned for use so must be taken out of the
				list of free blocks. */
				p_previous_block_link->next = p_block_link->next;
				/* If the block is larger than required it can be split into two. */
				if ((p_block_link->block_size - align_wanted_size) > HEAP_MIN_BLOCK_SIZE) {
					/* This block is to be split into two.  Create a new block
					following the number of bytes requested. The void cast is
					used to prevent byte alignment warnings from the compiler. */
					p_new_block_link = _nvtmpp_heap_get_free_link(heap_obj);
					if (p_new_block_link == NULL) {
						DBG_ERR("No free link\r\n");
						_nvtmpp_heap_unlock();
						return NULL;
					}
					// new_block is the block that want to return

					p_new_block_link->init_tag = NVTMPP_HEAP_INITED_TAG;
					p_new_block_link->block_addr = p_block_link->block_addr + p_block_link->block_size - align_wanted_size;
					p_new_block_link->block_size = align_wanted_size;
					p_block_link->block_size = p_block_link->block_size - align_wanted_size;
					p_block = (NVTMPP_HEAP_BLOCK_S *)p_block_link->block_va;
					p_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_block->block_size = p_block_link->block_size;

					DBG_IND("p_block_link = 0x%x, BlockAddr = 0x%x, BlockSize = 0x%x\r\n", (int)p_block_link, (int)p_block_link->block_addr, (int)p_block_link->block_size);
					if (heap_obj->va_start) {
						p_new_block = (NVTMPP_HEAP_BLOCK_S *)(heap_obj->va_start - heap_obj->pa_start + p_new_block_link->block_addr);
					} else {
						p_new_block = (NVTMPP_HEAP_BLOCK_S *)nvtmpp_ioremap_cache(p_new_block_link->block_addr, NVTMPP_HEAP_ALIGN);
					}
					p_new_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_new_block->block_addr = p_new_block_link->block_addr;
					p_new_block->block_va   = (UINT32)p_new_block;
					p_new_block->block_size = p_new_block_link->block_size;
					p_new_block->link       = p_new_block_link;
					p_new_block_link->block_va = p_new_block->block_va;
					p_new_block->end_tag    = (UINT32 *)(p_new_block_link->block_va+wanted_size-HEAP_END_TAG_SIZE);
					DBG_DUMP("1 block = 0x%x, size= 0x%x, end_tag addr= 0x%x\r\n",p_new_block_link->block_va,p_new_block->block_size, (int)p_new_block->end_tag);
					nvtmpp_heap_fill_endtag(p_new_block->end_tag);
					memset(p_new_block->reserved, 0x00, sizeof(p_new_block->reserved));
					/* Insert the new block into the list of free blocks. */
					_nvtmpp_heap_insert_block_to_freelist(heap_obj, p_block_link);
					_nvtmpp_heap_insert_block_to_usedlist(heap_obj, p_new_block_link);
					pv_return = (void *)(p_new_block_link->block_va + HEAP_STRUCT_SIZE);
				} else {
					p_block = (NVTMPP_HEAP_BLOCK_S *)p_block_link->block_va;
					p_block->init_tag   = NVTMPP_HEAP_INITED_TAG;
					p_block->block_size = p_block_link->block_size;
					p_block->end_tag    = (UINT32 *)(p_block_link->block_va+wanted_size-HEAP_END_TAG_SIZE);
					DBG_DUMP("2 block = 0x%x, size= 0x%x, end_tag addr= 0x%x\r\n",p_block_link->block_va,p_block_link->block_size, (int)p_block->end_tag);
					nvtmpp_heap_fill_endtag(p_block->end_tag);
					_nvtmpp_heap_insert_block_to_usedlist(heap_obj, p_block_link);
					pv_return = (void *)(p_block_link->block_va + HEAP_STRUCT_SIZE);
				}
				heap_obj->freebytes_remain -= align_wanted_size;
				DBG_IND("freebytes_remain=0x%x\r\n", (int)heap_obj->freebytes_remain);
			}
		}
	}
	//nvtmpp_dump_mem_range(debug_msg);
	_nvtmpp_heap_unlock();
	if (pv_return == NULL) {
		DBG_ERR("ddr%d, wanted_size=0x%x , free_remain=0x%x, max_free_blk=0x%x\r\n", ddr + 1, (int)wanted_size, (int)heap_obj->freebytes_remain, (int)nvtmpp_heap_get_max_free_block_size(ddr));
	}
	DBG_IND("pv_return = 0x%x\r\n", (int)pv_return);
	return pv_return;
}


/*-----------------------------------------------------------*/

INT32 nvtmpp_heap_free(NVTMPP_DDR ddr, void *pv)
{
	unsigned char  *puc = (unsigned char *) pv;
	NVTMPP_HEAP_LINK_S *p_link;
	NVTMPP_HEAP_BLOCK_S *p_block;
	HEAP_OBJ_S     *heap_obj;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return -1;
	}
	DBG_IND("ddr%d, addr=0x%x\r\n", ddr + 1, (int)pv);
	if (pv != NULL) {
		/* The memory being freed will have an xBlockLink structure immediately
		before it. */
		puc -= HEAP_STRUCT_SIZE;

		/* This unexpected casting is to keep some compilers from issuing
		byte alignment warnings. */
		p_block = (void *) puc;

		_nvtmpp_heap_lock();


		if (_nvtmpp_heap_chk_block(heap_obj, p_block, TRUE) < 0) {
			_nvtmpp_heap_unlock();
			return -1;
		}
		p_link = p_block->link;
		{
			/* Add this block to the list of free blocks. */
			heap_obj->freebytes_remain += p_link->block_size;
			if (_nvtmpp_heap_remove_block_from_usedlist(heap_obj, ((NVTMPP_HEAP_LINK_S *) p_link)) < 0) {
				debug_dumpmem((UINT32)p_block, 0x100);
				_nvtmpp_heap_unlock();
				return -1;
			}
			_nvtmpp_heap_insert_block_to_freelist(heap_obj, ((NVTMPP_HEAP_LINK_S *) p_link));
		}
		//p_block->init_tag = NVTMPP_HEAP_FREE_TAG;
		DBG_IND("ddr%d, addr = 0x%x , freebytes_remain=0x%x\r\n", ddr + 1, (int)pv, (int)heap_obj->freebytes_remain);
		//DBG_DUMP("Free addr=0x%x\r\n", pv);
		//nvtmpp_dump_mem_range(debug_msg);
		_nvtmpp_heap_unlock();
	}
	return 0;
}


BOOL nvtmpp_heap_chk_addr_valid(UINT32 addr)
{
	UINT32 i;

	for (i = 0; i < NVTMPP_DDR_MAX_NUM; i++) {
		if (addr >= g_heap_obj[i].pa_start && addr < g_heap_obj[i].pa_end) {
			return TRUE;
		}
	}
	return FALSE;
}


UINT32  nvtmpp_heap_get_max_free_block_size(NVTMPP_DDR ddr)
{
	HEAP_OBJ_S     *heap_obj;
	NVTMPP_HEAP_LINK_S *p_link;
	UINT32         max_block_size;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return 0;
	}
	p_link = heap_obj->free_block_start.next;
	max_block_size = p_link->block_size;
	DBG_IND("ddr%d max_block_size = 0x%x, BlockSize = 0x%x\r\n", ddr + 1, (int)max_block_size, (int)p_link->block_size);
	while (p_link->next != NULL) {
		p_link = p_link->next;
		DBG_IND("ddr%d max_block_size = 0x%x, BlockSize = 0x%x\r\n", ddr + 1, (int)max_block_size, (int)p_link->block_size);
		if (p_link->block_size > max_block_size) {
			max_block_size = p_link->block_size;
		}
	}
	max_block_size = ALIGN_FLOOR(max_block_size, NVTMPP_HEAP_ALIGN);
	if (max_block_size > HEAP_MIN_BLOCK_SIZE) {
		return (max_block_size - HEAP_MIN_BLOCK_SIZE);
	} else {
		return 0;
	}
}

UINT32  nvtmpp_heap_get_free_size(NVTMPP_DDR ddr)
{
	HEAP_OBJ_S     *heap_obj;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return 0;
	}
	return heap_obj->freebytes_remain;
}

void  nvtmpp_heap_dump(NVTMPP_DDR ddr)
{
	HEAP_OBJ_S     *heap_obj;
	NVTMPP_HEAP_LINK_S *p_link;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return;
	}
	p_link = heap_obj->free_block_start.next;
	while (p_link != NULL) {
		DBG_DUMP("ddr%d free BlockAddr = 0x%x, BlockSize = 0x%x\r\n", ddr + 1, (int)p_link, (int)p_link->block_size);
		p_link = p_link->next;
	}
	p_link = heap_obj->used_block_start.next;
	while (p_link != NULL) {
		DBG_DUMP("ddr%d used BlockAddr = 0x%x, BlockSize = 0x%x\r\n", ddr + 1, (int)p_link, (int)p_link->block_size);
		p_link = p_link->next;
	}
}

NVTMPP_HEAP_LINK_S *nvtmpp_heap_get_first_free_block(NVTMPP_DDR ddr)
{
	HEAP_OBJ_S     *heap_obj;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return NULL;
	}
	return heap_obj->free_block_start.next;
}

NVTMPP_HEAP_LINK_S *nvtmpp_heap_get_first_used_block(NVTMPP_DDR ddr)
{
	HEAP_OBJ_S     *heap_obj;

	heap_obj = _nvtmpp_heap_getobj(ddr);
	if (heap_obj == NULL) {
		return NULL;
	}
	return heap_obj->used_block_start.next;
}

INT32  nvtmpp_heap_check_mem_corrupt(void)
{
	HEAP_OBJ_S     *heap_obj;
	NVTMPP_HEAP_LINK_S *p_link;
	NVTMPP_HEAP_BLOCK_S *p_block;
	UINT32         i;

	for (i = NVTMPP_DDR_1; i < NVTMPP_DDR_MAX; i++) {
		heap_obj = _nvtmpp_heap_getobj(i);
		if (NULL == heap_obj) {
			break;
		}
		p_link = heap_obj->used_block_start.next;
		while (p_link != NULL && p_link != heap_obj->used_block_end) {
			if (_nvtmpp_heap_chk_link(heap_obj, p_link) < 0) {
				return -1;
			}
			p_block = (NVTMPP_HEAP_BLOCK_S *)p_link->block_va;
			if (_nvtmpp_heap_chk_block(heap_obj, p_block, FALSE) < 0) {
				return -1;
			}
			p_link = p_link->next;
		}
	}
	return 0;
}

