
#if defined (__KERNEL__)

//#include <linux/version.h>
//#include <linux/module.h>
//#include <linux/seq_file.h>
//#include <linux/proc_fs.h>
//#include <linux/bitops.h>
//#include <linux/interrupt.h>
//#include <linux/mm.h>
//#include <asm/uaccess.h>
//#include <asm/atomic.h>
//#include <asm/io.h>
//#include <linux/dma-mapping.h>

#include <linux/slab.h>

#elif defined (__FREERTOS)

#else

#endif


//#include "frammap/frammap_if.h"


#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_lib.h"

#include "kdrv_videoprocess/kdrv_vpe.h"
#include "kdrv_vpe_int.h"
#include "vpe_int.h"

//#include <mach/nvt_pcie.h>

/*
 * global var define here
 */
//struct ssca100_ll_cmd_pool * llcmd_pool;
#define VPE_SPIN_LOCK_IRQSAVE(x,flags)               vk_spin_lock_irqsave(&(x),flags)
#define VPE_SPIN_UNLOCK_IRQRESTORE(x,flags)          vk_spin_unlock_irqrestore(&(x),flags)
dma_addr_t          current_ll_addr;


int vpe_ll_enable(INT32 llmode)
{
	if (llmode) {
		vpe_hw_write(VPE_REG_INTS_1, 0x5);

		vpe_hw_write(VPE_REG_INTS_1 + 4, 0x4);
	} else {
		vpe_hw_write(VPE_REG_INTS_1, 0x00);
	}
	return 0;
}


int vpe_ll_fire_llcmd(VPE_LLCMD_POOL *llcmd_pool, VPE_LLCMD_DATA *cmdll_data)
{
	UINT32 fire_val = 0;
	//UINT32  dev_iobase;

	fire_val = (0x1 << VPE_BIT_LLFIRE);

	vpe_hw_write(VPE_REG_LLADDR, cmdll_data->paddr);

	current_ll_addr = cmdll_data->paddr;
	vpe_hw_write(VPE_REG_GLOCTL, fire_val);

	//CNT_INC(eng->fire_cnt);
	llcmd_pool->fire_cnt++;
	return 0;
}

dma_addr_t vpe_ll_addr_get(void)
{
	return current_ll_addr;
}



static int vpe_ll_destory_free_list(VPE_LLCMD_POOL *llcmd_pool)
{
	unsigned long flags;
	VPE_LLCMD_DATA *llcmd_pos, * llcmd_nxt;

	VPE_SPIN_LOCK_IRQSAVE(llcmd_pool->llcmd_lock, flags);

	list_for_each_entry_safe(llcmd_pos, llcmd_nxt, &llcmd_pool->llcmd_free_list, llcmd_list) {
		list_del(&llcmd_pos->llcmd_list);
		llcmd_pool->llcmd_tal_cnt --;
		kmem_cache_free(llcmd_pool->llcmd_cache, llcmd_pos);
	}

	VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
	return 0;
}

VPE_LLCMD_DATA *vpe_ll_find_cmdlist(VPE_LLCMD_POOL *llcmd_pool, INT32 idx)
{
	unsigned long flags;
	VPE_LLCMD_DATA *llcmd_pos, * llcmd_nxt;

	VPE_SPIN_LOCK_IRQSAVE(llcmd_pool->llcmd_lock, flags);

	list_for_each_entry_safe(llcmd_pos, llcmd_nxt, &llcmd_pool->llcmd_free_list, llcmd_list) {
		if (llcmd_pos->is_used && idx == llcmd_pos->llcmd_id) {
			VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
			return llcmd_pos;
		}
	}

	VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
	return NULL;

}

VPE_LLCMD_DATA *vpe_ll_get_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool)
{
	unsigned long flags;
	VPE_LLCMD_DATA *llcmd_pos, * llcmd_nxt;

	VPE_SPIN_LOCK_IRQSAVE(llcmd_pool->llcmd_lock, flags);

	list_for_each_entry_safe(llcmd_pos, llcmd_nxt, &llcmd_pool->llcmd_free_list, llcmd_list) {
		if (!llcmd_pos->is_used) {
			llcmd_pos->is_used = 1;
			llcmd_pos->cmd_num = 0;
			CNT_DEC(llcmd_pool->llcmd_free_cnt);
			CNT_INC(llcmd_pool->llcmd_work_cnt);
			VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
			return llcmd_pos;
		}
	}

	VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);

	return NULL;

}

int vpe_ll_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool, UINT16 llcmd_id)
{
	unsigned long flags;
	VPE_LLCMD_DATA *llcmd_pos, * llcmd_nxt;

	VPE_SPIN_LOCK_IRQSAVE(llcmd_pool->llcmd_lock, flags);

	list_for_each_entry_safe(llcmd_pos, llcmd_nxt, &llcmd_pool->llcmd_free_list, llcmd_list) {

		if (llcmd_pos->llcmd_id == llcmd_id) {
			llcmd_pos->is_used = 0;
			llcmd_pos->cmd_num = 0;
			CNT_INC(llcmd_pool->llcmd_free_cnt);
			CNT_DEC(llcmd_pool->llcmd_work_cnt);
			VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
			return 0;
		}
	}

	VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
	printk("getback to freelist not found (%d) \n", llcmd_id);
	return -1;
}

int vpe_ll_putback_free_cmdlist(VPE_LLCMD_POOL *llcmd_pool, VPE_LLCMD_DATA *cmdll_data)
{
	unsigned long flags;
	VPE_LLCMD_DATA *llcmd_pos, * llcmd_nxt;

	VPE_SPIN_LOCK_IRQSAVE(llcmd_pool->llcmd_lock, flags);

	list_for_each_entry_safe(llcmd_pos, llcmd_nxt, &llcmd_pool->llcmd_free_list, llcmd_list) {

		if (llcmd_pos->llcmd_id == cmdll_data->llcmd_id) {
			llcmd_pos->is_used = 0;
			llcmd_pos->cmd_num = 0;
			CNT_INC(llcmd_pool->llcmd_free_cnt);
			CNT_DEC(llcmd_pool->llcmd_work_cnt);
			VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
			return 0;
		}
	}

	VPE_SPIN_UNLOCK_IRQRESTORE(llcmd_pool->llcmd_lock, flags);
	printk("getback to freelist not found (%d) \n", cmdll_data->llcmd_id);
	return -1;

}


int vpe_ll_init(VPE_LLCMD_POOL *llcmd_pool)
{
	//char buf_name[20];
	INT32  i;
	VPE_LLCMD_DATA *llcmd_tmp = NULL;

	INIT_LIST_HEAD(&llcmd_pool->llcmd_free_list);


	llcmd_pool->llcmd_cache = kmem_cache_create("vpe_ll_cach", sizeof(VPE_LLCMD_DATA), 0, 0, NULL);
	if (ZERO_OR_NULL_PTR(llcmd_pool->llcmd_cache)) {
		panic("%s: kmem_cache_create fail for llcmd_cache!\n", __func__);
	}

#if 1
	llcmd_pool->llcmd_buf_info.lltable_size = PAGE_ALIGN(VPE_LLPOOL_SZ);
#if VPE_CPU_UTILIZATION
	llcmd_pool->llcmd_buf_info.lltable_vaddr = dma_alloc_coherent(NULL, llcmd_pool->llcmd_buf_info.lltable_size, &llcmd_pool->llcmd_buf_info.lltable_paddr, GFP_KERNEL);
#else
	llcmd_pool->llcmd_buf_info.lltable_vaddr = dma_alloc_coherent(NULL, llcmd_pool->llcmd_buf_info.lltable_size, &llcmd_pool->llcmd_buf_info.lltable_paddr, GFP_DMA | GFP_KERNEL);
#endif
#else
	llcmd_pool->llcmd_buf_info.alloc_type = ALLOC_NONCACHABLE;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
	sprINT32f(buf_name, "vpe_ll");
	llcmd_pool->llcmd_buf_info.name = buf_name;
#endif
	llcmd_pool->llcmd_buf_info.size = PAGE_ALIGN(VPE_LLPOOL_SZ);

	if (frm_get_buf_ddr(DDR_ID_SYSTEM, &llcmd_pool->llcmd_buf_info)) {
		panic("allcate frammap memory fail for vpe_llpool\n");
	}
#endif
	/*init llcmd spin lock*/
	vk_spin_lock_init(&llcmd_pool->llcmd_lock);

	/*init llcmd pool*/
	for (i = 0 ; i < VPE_LLCMD_NUM ; i++) {
		//coverity[overwrite_var]
		llcmd_tmp = (VPE_LLCMD_DATA *)kmem_cache_alloc(llcmd_pool->llcmd_cache, GFP_KERNEL);
		//llcmd_tmp = kmem_cache_alloc(llcmd_pool->llcmd_cache, GFP_KERNEL);
		if (llcmd_tmp == NULL) {
			panic("%s, no memory for llcmd data! \n", __func__);
		} else {
			memset((void *)llcmd_tmp, 0x0, sizeof(VPE_LLCMD_DATA));

			llcmd_tmp->llcmd_id = i + 1;
			llcmd_tmp->sz    = VPE_LLCMD_SZ;
#if 1
			llcmd_tmp->vaddr = (void *)((UINT32)llcmd_pool->llcmd_buf_info.lltable_vaddr + (i * VPE_LLCMD_SZ));

			llcmd_tmp->paddr = (dma_addr_t)((UINT32)llcmd_pool->llcmd_buf_info.lltable_paddr + (i * VPE_LLCMD_SZ));
#else
			llcmd_tmp->vaddr = (void *)((UINT32)llcmd_pool->llcmd_buf_info.va_addr + (i * VPE_LLCMD_SZ));
			llcmd_tmp->paddr = (dma_addr_t)((UINT32)llcmd_pool->llcmd_buf_info.phy_addr + (i * VPE_LLCMD_SZ));
#endif

			if (!ADDR_IS_ALIGN_8((UINT32)llcmd_tmp->paddr)) {
				panic("vpe allocate memory isn't align8!(0x%x), vaddr(0x%x) \n", (UINT32)llcmd_tmp->paddr, (UINT32)llcmd_tmp->vaddr);
			}

			INIT_LIST_HEAD(&llcmd_tmp->llcmd_list);

			list_add_tail(&llcmd_tmp->llcmd_list, &llcmd_pool->llcmd_free_list);
		}
	}

	llcmd_pool->llcmd_tal_cnt = VPE_LLCMD_NUM;
	CNT_SET(llcmd_pool->llcmd_free_cnt, VPE_LLCMD_NUM);

	return 0;
}

/*
 * function: vpe308_ll_exit
 * purpose :
 *
 */

int vpe_ll_exit(VPE_LLCMD_POOL *llcmd_pool)
{
	/*destory free list pool*/
	vpe_ll_destory_free_list(llcmd_pool);
	/*free buffer*/
#if 1
	dma_free_coherent(NULL, llcmd_pool->llcmd_buf_info.lltable_size, llcmd_pool->llcmd_buf_info.lltable_vaddr, llcmd_pool->llcmd_buf_info.lltable_paddr);
#else
	frm_free_buf_ddr(llcmd_pool->llcmd_buf_info.va_addr);
#endif
	/*destroy mem cache*/
	kmem_cache_destroy(llcmd_pool->llcmd_cache);
	return 0;

}

