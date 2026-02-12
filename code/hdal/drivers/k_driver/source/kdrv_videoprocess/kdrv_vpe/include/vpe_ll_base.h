#ifndef __VPE_LL_H__
#define __VPE_LL_H__

//#include "vpe_platform.h"
#include <kwrap/spinlock.h>

#include "kdrv_videoprocess/kdrv_vpe.h"

#define VPE_LLPOOL_SZ       (1*1024*1024)
#define VPE_LLCMD_SZ        (4*1024*3)
#define VPE_LLCMD_NUM       (VPE_LLPOOL_SZ/VPE_LLCMD_SZ)

struct vpe_llcmd_table_s {
	void         *lltable_vaddr;
	dma_addr_t   lltable_paddr;
	UINT32 lltable_size;

};

typedef struct _VPE_LLCMD_POOL_ {
	KDRV_VPE_MEM_CACHE             *llcmd_cache;
	KDRV_VPE_LIST_HEAD              llcmd_free_list;
	//KDRV_VPE_LIST_HEAD            llcmd_work_list;
	//struct frammap_buf_info       llcmd_buf_info;
	struct vpe_llcmd_table_s   llcmd_buf_info;
	vk_spinlock_t                    llcmd_lock;
	UINT32                           llcmd_tal_cnt;
	atomic_t                      llcmd_free_cnt;
	atomic_t                      llcmd_work_cnt;
	UINT32                           fire_cnt;
	UINT32                           INT32_cnt;
} VPE_LLCMD_POOL;

typedef struct _VPE_LLCMD_DATA_ {
	UINT16                 llcmd_id;
	UINT16                 is_used;
	KDRV_VPE_LIST_HEAD    llcmd_list;
	void               *vaddr;
	dma_addr_t          paddr;
	UINT32                 sz;
	UINT32                 cmd_num;
	UINT16                nxt_update;
} VPE_LLCMD_DATA;

#define VPE_LLCMD_BIT_TYPE            29
#define VPE_LLCMD_BIT_NORM_BYTE_EN        24
#define VPE_LLCMD_BIT_NORM_REG_OFFSET     0
#define VPE_LLCMD_BIT_NORM_REG_VALUE      0

typedef enum {
	VPE_LLCMD_MODE_FIRE             = 0x0,
	VPE_LLCMD_MODE_NEXTCMD_FIRE     = 0x1,
	VPE_LLCMD_MODE_NEXTCMD          = 0x2,
	//VPE_CONFIG_CMD_IDEL             = 0x3,
	VPE_LLCMD_MODE_NORMAL           = 0x4
} vpe_llcmd_cmd_type_t;


#define VPE_HI_NXTADDR_MASK          0xff000000
#define VPE_LO_NXTADDR_MASK          0x00ffffff
#define VPE_HI_OFFSET_BIT            24
#define VPE_HI_LLCMD_NXTADDR_BIT     0
#define VPE_LO_LLCMD_NXTADDR_BIT     8

#define VPE_LLCMD_NORM(byte_en,reg)              ((VPE_LLCMD_MODE_NORMAL << VPE_LLCMD_BIT_TYPE) | (byte_en << VPE_LLCMD_BIT_NORM_BYTE_EN) | (reg << VPE_LLCMD_BIT_NORM_REG_OFFSET))
#define VPE_LLCMD_NORM_VAL(val)                      (val << VPE_LLCMD_BIT_NORM_REG_VALUE)
#define VPE_LLCMD_FIRE                          (VPE_LLCMD_MODE_FIRE)

#define VPE_LLCMD_FIRE_VAL(jobid)               (jobid&0xff)

#define VPE_LLCMD_NXTFIRE(addr)                ((VPE_LLCMD_MODE_NEXTCMD_FIRE<<VPE_LLCMD_BIT_TYPE) \
		|(((addr&VPE_HI_NXTADDR_MASK)>>VPE_HI_OFFSET_BIT)<<VPE_HI_LLCMD_NXTADDR_BIT))

#define VPE_LLCMD_NXTFIRE_VAL(addr,jobid)      (((addr&VPE_LO_NXTADDR_MASK)<<VPE_LO_LLCMD_NXTADDR_BIT)|(jobid&0xff))

#define VPE_LLCMD_NXTUPDATE(addr)                ((VPE_LLCMD_MODE_NEXTCMD << VPE_LLCMD_BIT_TYPE) \
		|(((addr&VPE_HI_NXTADDR_MASK)>>VPE_HI_OFFSET_BIT)<<VPE_HI_LLCMD_NXTADDR_BIT))

#define VPE_LLCMD_NXTUPDATE_VAL(addr)      (((addr&VPE_LO_NXTADDR_MASK)<<VPE_LO_LLCMD_NXTADDR_BIT)|(0&0xff))




#define CNT_INC(x)                     atomic_inc(&(x))
#define CNT_DEC(x)                     do {if (atomic_read(&(x))) atomic_dec(&(x));} while(0)
#define CNT_SET(x,val)                 atomic_set(&(x),val)
#define ADDR_IS_ALIGN_8(x)             (((x) & 0x7) ? 0 : 1)

VPE_LLCMD_DATA *vpe_ll_get_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool);
VPE_LLCMD_DATA *vpe_ll_find_cmdlist(VPE_LLCMD_POOL *llcmd_pool, INT32 idx);


int vpe_ll_fire_llcmd(VPE_LLCMD_POOL *llcmd_pool, VPE_LLCMD_DATA *cmdll_data);
int vpe_ll_enable(INT32 llmode);





int vpe_ll_putback_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool, VPE_LLCMD_DATA *cmdll_data);
int vpe_ll_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool, UINT16 llcmd_id);


int vpe_ll_init(VPE_LLCMD_POOL *llcmd_pool);
int vpe_ll_exit(VPE_LLCMD_POOL *llcmd_pool);
dma_addr_t vpe_ll_addr_get(void);

#endif
