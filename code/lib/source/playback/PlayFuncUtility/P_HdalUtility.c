#include <string.h>
#include "PlaybackTsk.h"
#include "hdal.h"
#include "hd_type.h"
#include <kwrap/debug.h>
#include "kflow_common/nvtmpp.h"

UINT32 PB_get_hd_phy_addr(void *va)
{
    if (va) {
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
        vir_meminfo.va = va;
        if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) == HD_OK) {
                return vir_meminfo.pa;
        }
    }
	
    return 0;
}

HD_RESULT PB_get_hd_common_buf(HD_COMMON_MEM_VB_BLK *pblk, UINT32 *pPa, UINT32 *pVa, UINT32 blk_size)
{
	// get memory
	*pblk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0); // Get block from mem pool
	if (*pblk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pPa = hd_common_mem_blk2pa(*pblk); // get physical addr
	if (*pPa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pVa = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pPa, blk_size); 
	if (*pVa == 0) {
		DBG_ERR("Convert to VA failed for file buffer for decoded buffer!\r\n");
		return HD_ERR_SYS;
	}

	return HD_OK;
}
 

HD_RESULT PB_AllocPrivateMem(UINT32 *pVa, UINT32 *pPa, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = NVTMPP_DDR_1;

	// allocate mem
	ret = hd_common_mem_alloc("XML", &pa, (void **)&va, blk_size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("alloc size(0x%x), ddr(%d)\r\n", (unsigned int)(blk_size), (int)ddr_id);
		return HD_ERR_SYS;
	}
	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
	
	*pVa = (UINT32)va;
	*pPa = (UINT32)pa;
	
	return HD_OK;
}