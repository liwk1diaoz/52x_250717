#if defined(__LINUX)
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/io.h>
#elif defined(__FREERTOS)
#include "comm/compiler.h"
#include "comm/libfdt.h"
#include <plat/rtosfdt.h>
#include <string.h>
#include <malloc.h>
#endif
#include "ipp_debug_int.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "ctl_ipp_util_int.h"

#define CTL_IPP_UTIL_CLOCK_DFT	(280)	/* select 280 for ipp to use pll_13 as default clk src */

void *ctl_ipp_util_os_malloc_wrap(UINT32 want_size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(want_size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(want_size);
#endif

	return p_buf;
}

void ctl_ipp_util_os_mfree_wrap(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}

void *ctl_ipp_util_os_ioremap_nocache_wrap(phys_addr_t offset, size_t size)
{
#if defined(__LINUX)
	return ioremap_nocache(offset, size);
#elif defined(__FREERTOS)
	return (void *)offset;
#endif
}

void ctl_ipp_util_os_iounmap_wrap(void *addr)
{
#if defined(__LINUX)
	iounmap(addr);
#endif
}

INT32 ctl_ipp_util_get_max_stripe(void)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		return CTL_IPP_520_MAX_STRIPE_W;
	} else {
		return CTL_IPP_528_MAX_STRIPE_W;
	}
}

UINT32 ctl_ipp_util_get_dtsi_clock(void)
{
#if defined(__LINUX)
	struct device_node* of_node = of_find_node_by_path("/kflow_ctl_ipp");
	UINT32 clk = 0;

	if (of_node) {
		if (of_property_read_u32(of_node, "clock-rate", &clk) != 0) {
			CTL_IPP_DBG_WRN("cannot find /kflow_ctl_ipp/clock-rate\r\n");
		}
	} else {
		CTL_IPP_DBG_WRN("cannot find node /kflow_ctl_ipp\r\n");
	}

	if (clk == 0) {
		clk = CTL_IPP_UTIL_CLOCK_DFT;
		CTL_IPP_DBG_WRN("get dtsi clk 0, force to %d\r\n", CTL_IPP_UTIL_CLOCK_DFT);
	}

	return clk;

#elif defined(__FREERTOS)
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();
	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */
	UINT32 clk = 0;

	// get linux space
	if (p_fdt != NULL) {
		nodeoffset = fdt_path_offset(p_fdt, "/kflow_ctl_ipp");
		if (nodeoffset < 0) {
			CTL_IPP_DBG_WRN("failed to offset for  %s = %d \r\n", "/kflow_ctl_ipp", nodeoffset);
		} else {
			nodep = fdt_getprop(p_fdt, nodeoffset, "clock-rate", &len);
			if (len == 0 || nodep == NULL) {
				CTL_IPP_DBG_WRN("failed to access clock-rate.\r\n");
			} else {
				clk = be32_to_cpu(*(unsigned int *)nodep);
			}
		}
	}

	if (clk == 0) {
		clk = CTL_IPP_UTIL_CLOCK_DFT;
		CTL_IPP_DBG_WRN("get dtsi clk 0, force to %d\r\n", CTL_IPP_UTIL_CLOCK_DFT);
	}

	return clk;
#endif
}
