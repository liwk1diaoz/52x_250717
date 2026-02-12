#include <hdal.h>
#include "sys_mempool.h"
#include "vendor_common.h"

UINT32 mempool_storage_sdio;
UINT32 mempool_storage_nand;
UINT32 mempool_filesys;
UINT32 mempool_msdcnvt;
UINT32 mempool_msdcnvt_pa;

void mempool_init(void)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret;

	ret = vendor_common_mem_alloc_fixed_pool("sdio", &pa, (void **)&va, POOL_SIZE_STORAGE_SDIO, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_storage_sdio = (UINT32)va;
	ret = vendor_common_mem_alloc_fixed_pool("nand", &pa, (void **)&va, POOL_SIZE_STORAGE_NAND, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_storage_nand = (UINT32)va;
	ret = vendor_common_mem_alloc_fixed_pool("filesys", &pa, (void **)&va, POOL_SIZE_FILESYS, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_filesys = (UINT32)va;
	ret = vendor_common_mem_alloc_fixed_pool("msdcnvt", &pa, (void **)&va, POOL_SIZE_MSDCNVT, DDR_ID0);
	if (ret != HD_OK) {
		return;
	}
	mempool_msdcnvt = (UINT32)va;
	mempool_msdcnvt_pa = (UINT32)pa;
}