#include <libfdt.h>
#include <compiler.h>
#include <rtosfdt.h>
#include <kflow_common/nvtmpp.h>
#include <hdal.h>
#include "sys_nvtmpp.h"

int nvtmpp_init(void)
{
#if defined(__FREERTOS)
	const void *p_fdt = fdt_get_base();
	if (p_fdt == NULL) {
		printf("p_fdt is NULL. \n");
		return -1;
	}

	// read /hdal-memory/media
	int nodeoffset = fdt_path_offset(p_fdt, "/hdal-memory/media");
	if (nodeoffset < 0) {
		printf("failed to offset for /hdal-memory/media = %d \n", nodeoffset);
		return -1;
	}

	int len;
	const void *nodep = fdt_getprop(p_fdt, nodeoffset, "reg", &len);
	if (len == 0 || nodep == NULL) {
		printf("failed to access /nvt_memory_cfg/rtos/reg.\n");
		return -1;
	}

	NVTMPP_ER          ret;
	NVTMPP_SYS_CONF_S  nvtmpp_sys_conf;
	unsigned int *p_data = (unsigned int *)nodep;

	//dram1_size = dma_getDramCapacity(DMA_ID_1);
	nvtmpp_install_id();
	memset((void *)&nvtmpp_sys_conf, 0x00, sizeof(nvtmpp_sys_conf));
	nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_1].virt_addr = (UINT32)be32_to_cpu(p_data[0]);
	nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_1].phys_addr = (UINT32)be32_to_cpu(p_data[0]);
	nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_1].size = (UINT32)be32_to_cpu(p_data[1]);
	if (len >= 16) {
		nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_2].virt_addr = (UINT32)be32_to_cpu(p_data[2]);
		nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_2].phys_addr = (UINT32)be32_to_cpu(p_data[2]);
		nvtmpp_sys_conf.ddr_mem[NVTMPP_DDR_2].size = (UINT32)be32_to_cpu(p_data[3]);
	}
	ret = nvtmpp_sys_init(&nvtmpp_sys_conf);
	if (NVTMPP_ER_OK != ret) {
		printf("nvtmpp sys init err: %d\r\n", ret);
		return -1;
	}
	return 0;
#else
	return 0;
#endif
}