#include <hdal.h>
#include "sys_mempool.h"
#include "vendor_common.h"

UINT32 mempool_msdcnvt = 0;
UINT32 mempool_msdcnvt_pa = 0;

void mempool_init(void)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret;

	if (mempool_msdcnvt == 0) {
		ret = vendor_common_mem_alloc_fixed_pool("msdcnvt", &pa, (void **)&va, POOL_SIZE_MSDCNVT, DDR_ID0);
		if (ret != HD_OK) {
			return;
		}
		mempool_msdcnvt = (UINT32)va;
		mempool_msdcnvt_pa = (UINT32)pa;
	}
}