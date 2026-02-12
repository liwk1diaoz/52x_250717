/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_blk.h

    @brief      nvtmpp video buffer block handling

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#ifndef _NVTMPP_BLK_H
#define _NVTMPP_BLK_H
#include "kflow_common/nvtmpp.h"
#include "nvtmpp_int.h"
#include "nvtmpp_module.h"

#define CACHE_LINE_ALIGN              64
#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
#define NVTMPP_BLK_ALIGN              0x1000
#else
#define NVTMPP_BLK_ALIGN              CACHE_LINE_ALIGN
#endif
#define NVTMPP_BLK_HEADER_ALIGN       CACHE_LINE_ALIGN
#define NVTMPP_BLK_MAP_SIZE           NVTMPP_BLK_HEADER_ALIGN
#define NVTMPP_BLK_ALIGN_MASK         (NVTMPP_BLK_ALIGN-1)
#define NVTMPP_BLK_HEADER_ALIGN_MASK  (NVTMPP_BLK_HEADER_ALIGN-1)
#define NVTMPP_BLK_REF_REACH_ZERO      NVTMPP_ER_OK+1

#define NVTMPP_BLK2POOL(pool, blk)  {pool = ((NVTMPP_VB_BLK_S *)blk)->pool; }

typedef struct _NVTMPP_VB_BLK_S {
	UINT32                    init_tag;         ///< block init tag
	UINT32                    pool;             ///< this block belongs to which pool
	UINT32                    blk_id;           ///< block ID
	UINT32                    blk_head_addr;    ///< block header physical address
	UINT32                    buf_addr;         ///< block buffer physical address
	UINT32                    blk_head_va;      ///< block header virtual address
	UINT32                    buf_va;           ///< block buffer virtual address
	struct  _NVTMPP_VB_BLK_S *next;             ///< next block
	UINT8                     total_ref_cnt;    ///< block total reference count
	UINT8                     module_ref_cnt[NVTMPP_MODULE_MAX_NUM];    ///< module reference count of this block
	UINT32                    max_ref_cnt;      ///< the maximum reference count happened
	UINT32                    want_size;        ///< want to get block size
	UINT32                    *p_end_tag;
	UINT32                    blk_size;
} NVTMPP_VB_BLK_S;

STATIC_ASSERT(sizeof(NVTMPP_VB_BLK_S) == NVTMPP_BLK_HEADER_ALIGN);

extern UINT32      nvtmpp_vb_get_blk_header_size(void);
extern UINT32      nvtmpp_vb_get_blk_end_tag_size(void);
extern NVTMPP_VB_BLK_S *nvtmpp_vb_init_blk(UINT32 pool, UINT32 blk_id, UINT32 blk_head_addr, UINT32 blk_total_size, BOOL is_map_blk);
extern INT32       nvtmpp_vb_exit_blk(NVTMPP_VB_BLK_S *p_blk);
extern void        nvtmpp_vb_insert_first_blk(NVTMPP_VB_BLK_S **start, NVTMPP_VB_BLK_S *node);
extern void        nvtmpp_vb_remove_first_blk(NVTMPP_VB_BLK_S **start);
extern void        nvtmpp_vb_remove_blk(NVTMPP_VB_BLK_S **start, NVTMPP_VB_BLK_S *node);
extern BOOL        nvtmpp_vb_chk_blk_valid(NVTMPP_VB_BLK blk);
extern NVTMPP_ER   nvtmpp_vb_add_blk_ref(UINT32 module_id, NVTMPP_VB_BLK blk);
extern NVTMPP_ER   nvtmpp_vb_minus_blk_ref(UINT32 module_id, NVTMPP_VB_BLK blk);
extern NVTMPP_ER   nvtmpp_vb_dump_blk_ref(int (*dump)(const char *fmt, ...), NVTMPP_VB_BLK blk, UINT32 module_max_count, UINT32 *module_strlen_list);
extern void        *nvtmpp_vb_blk_map_va(NVTMPP_VB_BLK blk);
extern void        nvtmpp_vb_blk_unmap_va(NVTMPP_VB_BLK blk);
#endif

