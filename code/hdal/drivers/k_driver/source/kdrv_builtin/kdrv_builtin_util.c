#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/io.h>
#include <plat/nvt-sramctl.h>
#include "kdrv_builtin.h"

#define SRAMCTRL_BASE_ADDR  0xF0D60000

static int m_rtos_boot = 0;
static int m_fastboot = 0;
static int m_fastboot_init = 0;
static int m_ai_fastboot = 0;
static int m_ai_fastboot_init = 0;
static FBOOT_SRAMCTRL m_fastboot_sramctrl_eng = 0;

#if !defined(MODULE)
static int __init early_param_rtos_boot(char *p)
{
	if (p == NULL) {
		return 0;
	}
	if (strncmp(p, "on", 3) == 0) {
		m_rtos_boot = 1;
	}
	return 0;
}

early_param("rtos_boot", early_param_rtos_boot);
#endif

int kdrv_builtin_is_fastboot(void)
{
	if (m_fastboot_init) {
		return m_fastboot;
	}

	if (m_rtos_boot) {
		struct device_node* of_node = of_find_node_by_path("/fastboot");

		if (of_node) {
			if (of_property_read_u32(of_node, "enable", &m_fastboot) != 0) {
				pr_err("cannot find /fastboot/enable");
			}
		}
	}

	m_fastboot_init = 1;

	return m_fastboot;
}

int kdrv_builtin_ai_is_fastboot(void)
{
	if (m_ai_fastboot_init) {
		return m_ai_fastboot;
	}
	{
		char *path = "/fastboot/ai_buf";
		struct device_node* dt_node;

		dt_node = of_find_node_by_path(path);
		if (dt_node) {
			m_ai_fastboot = 1;
		} else {
			m_ai_fastboot = 0;
		}
		m_ai_fastboot_init = 1;
		return m_ai_fastboot;
	}
}

int kdrv_builtin_set_sram_ctrl(FBOOT_SRAMCTRL eng)
{
	unsigned int reg_addr, val;

	// mmap va
	reg_addr = (unsigned int)ioremap_nocache(SRAMCTRL_BASE_ADDR, 4);

	// get reg value
	val = ioread32((void *)reg_addr);

	// disable shutdown
	nvt_disable_sram_shutdown(CNN2_SD);

	// set reg value
	val |= 0x3000000;
	if (eng == FBOOT_SRAMCTRL_CNN2) {
		val &= 0xFCFFFFFF;
		iowrite32(val, (void *)reg_addr);
		m_fastboot_sramctrl_eng = eng;
	} else if (eng == FBOOT_SRAMCTRL_DCE) {
		val &= 0xFDFFFFFF;
		iowrite32(val, (void *)reg_addr);
		m_fastboot_sramctrl_eng = eng;
	} else if (eng == FBOOT_SRAMCTRL_CPU) {
		val &= 0xFEFFFFFF;
		iowrite32(val, (void *)reg_addr);
		m_fastboot_sramctrl_eng = eng;
	} else {
		printk("%s invalid ctrl eng %d\n",__func__, (int)eng);
		return -1;
	}

	// unmap va
	iounmap((void *)reg_addr);

	return 0;
}

FBOOT_SRAMCTRL kdrv_builtin_get_sram_ctrl(void)
{
	return m_fastboot_sramctrl_eng;
}

EXPORT_SYMBOL(kdrv_builtin_is_fastboot);
EXPORT_SYMBOL(kdrv_builtin_set_sram_ctrl);
EXPORT_SYMBOL(kdrv_builtin_get_sram_ctrl);

#else
#endif
