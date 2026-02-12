#include "kdrv_vdocdc_dbg.h"

#include "sim_vdocdc_mem.h"

#if defined(__LINUX)
void *vdocdc_get_mem(struct nvt_fmem_mem_info_t *pinfo, unsigned int size)
{
	int ret = 0;	
	void *handle = NULL;
	
	ret = nvt_fmem_mem_info_init(pinfo, NVT_FMEM_ALLOC_CACHE, size, NULL);

	if (ret >= 0)
		handle = fmem_alloc_from_cma(pinfo, 0);

	nvt_dbg(INFO, "buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n",(UINT32)pinfo->vaddr, (UINT32)pinfo->paddr);
		
	return handle;
}

void vdocdc_free_mem(void *handle)
{
	if (fmem_release_from_cma(handle, 0) < 0)
		nvt_dbg(ERR, "free_mem error\r\n");
}
#else

void *vdocdc_get_mem(struct nvt_fmem_mem_info_t *pinfo, unsigned int size)
{
	void *handle = NULL;
	
	if (pinfo->ddr_addr == 0) {
		pinfo->ddr_addr = 0x7700000;//OS_GetMempoolAddr(POOL_ID_APP);
		pinfo->ddr_size = 0x2000000;//OS_GetMempoolSize(POOL_ID_APP);
	}

	if (pinfo->ddr_size > size) {
		pinfo->vaddr = pinfo->ddr_addr;
		pinfo->size = size;

		pinfo->ddr_addr += pinfo->size;
		pinfo->ddr_size -= size;

		handle = pinfo;
	}	

	return handle;
}

void vdocdc_free_mem(void *handle)
{
	;
}

#endif
