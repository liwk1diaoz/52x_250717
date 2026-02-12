#include <stdio.h>
#include <stdlib.h>
#include <kwrap/nvt_type.h>
#include <kwrap/debug.h>
#include <comm/bin_info.h>
#include <plat/rtosfdt.h>
#include <libfdt.h>

extern HEADINFO bin_info;

const void *fdt_get_base(void)
{
	// check if valid
	int er;
	const void *fdt = (const void *)(bin_info.Resv1[HEADINFO_RESV_IDX_FDT_ADDR]);
	if ((er = fdt_check_header(fdt)) != 0) {
		vk_printk("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)fdt, er);
		return NULL;
	}
	return fdt;
}

