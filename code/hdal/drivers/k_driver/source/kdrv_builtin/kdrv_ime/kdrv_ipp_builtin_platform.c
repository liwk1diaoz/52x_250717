#if defined (__KERNEL__)
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <plat/nvt-sramctl.h>
#include "kdrv_ipp_builtin.h"
#include "kdrv_ipp_builtin_int.h"
#include "ime_dbg.h"

void* kdrv_ipp_builtin_plat_ioremap_nocache(UINT32 addr, UINT32 size)
{
	return ioremap_nocache(addr, size);
}

void kdrv_ipp_builtin_plat_iounmap(void *addr)
{
	iounmap(addr);
}


INT32 kdrv_ipp_builtin_plat_init_clk(void)
{
	CHAR *engine_clk_name[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"f0c70000.ife",
		"f0c20000.dce",
		"f0c30000.ipe",
		"f0c40000.ime",
		"f0d00000.ife2",
	};
	struct clk *source_clk = NULL;
	struct clk *eng_clk = NULL;
	UINT32 i;

	source_clk = clk_get(NULL, "pll13");
	if (IS_ERR(source_clk)) {
		DBG_ERR("get pll13 clk failed\r\n");
		return -1;
	}

	for (i = 0; i < KDRV_IPP_BUILTIN_ENG_MAX; i++) {
		eng_clk = clk_get(NULL, engine_clk_name[i]);
		if (IS_ERR(eng_clk)) {
			DBG_ERR("get clk %s failed\r\n", engine_clk_name[i]);
		} else {
			clk_set_parent(eng_clk, source_clk);
			clk_prepare(eng_clk);
			clk_enable(eng_clk);
			clk_put(eng_clk);
		}
	}
	clk_put(source_clk);
	KDRV_IPP_BUILTIN_DUMP("ipp engine clock init end\r\n");

	return 0;
}

INT32 kdrv_ipp_builtin_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num)
{
	struct device_node *of_node;
	INT32 rt;

	of_node = of_find_node_by_path(node);
	rt = of_property_read_u32_array(of_node, tag, (UINT32 *)buf, num);

	return rt;
}

INT32 kdrv_ipp_builtin_plat_read_dtsi_string(CHAR *node, CHAR *tag, const CHAR **str)
{
	struct device_node *of_node;
	INT32 rt;

	of_node = of_find_node_by_path(node);
	rt = of_property_read_string(of_node, tag, str);

	return rt;
}

void *kdrv_ipp_builtin_plat_malloc(UINT32 want_size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kzalloc(want_size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(want_size);
#endif

	return p_buf;
}

void kdrv_ipp_builtin_plat_free(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}

#if 0
#endif

EXPORT_SYMBOL(kdrv_ipp_builtin_update_timestamp);
EXPORT_SYMBOL(kdrv_ipp_builtin_exit);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_ctl_info);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_eng_enable);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_isp_info);

EXPORT_SYMBOL(kdrv_ipp_set_reg);
EXPORT_SYMBOL(kdrv_ipp_builtin_reg_dump);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_reg_dtsi);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_reg_num);
EXPORT_SYMBOL(kdrv_ipp_builtin_reg_dtsi_cb);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_hdal_reg_dtsi);
EXPORT_SYMBOL(kdrv_ipp_builtin_get_hdal_hdl_list);

EXPORT_SYMBOL(kdrv_ipp_builtin_frc_dump);

EXPORT_SYMBOL(kdrv_ipp_builtin_debug);

#endif