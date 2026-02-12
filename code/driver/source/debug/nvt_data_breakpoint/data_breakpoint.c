/*
 * data_breakpoint.c - Sample HW Breakpoint file to watch kernel data address
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usage:
 *	modprobe nvt_data_breakpoint ksym=<ksym_name> wp_chk_val=<Value>
 *	modprobe nvt_data_breakpoint wp_addr=<Address> wp_chk_val=<Value>
 *
 * This file is a kernel module that places a breakpoint over ksym_name kernel
 * variable using Hardware Breakpoint register. The corresponding handler which
 * prints a backtrace is invoked every time a write operation is performed on
 * that variable.
 *
 * Copyright (C) IBM Corporation, 2009
 *
 * Author: K.Prasad <prasad@linux.vnet.ibm.com>
 * Modified by: Wayne Lin <Wayne_Lin@novatek.com.tw>
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/kprobes.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <mach/nvt-io.h>

#define CONFIG_PER_CORE					1
#define ENABLE_CACHE_FLUSH_WORKAROUND	1	// enable for 172 (ca53 arm_v8)
#define ENABLE_SYSFS					0

static unsigned int wp_addr = 0xe00020f0;
module_param(wp_addr, uint, S_IRUGO);
MODULE_PARM_DESC(wp_addr, "Config write detect address");
static unsigned int wp_chk_val = 0;
module_param(wp_chk_val, uint, S_IRUGO);
MODULE_PARM_DESC(wp_chk_val, "Config write detect address checked value");
static char func_name[KSYM_NAME_LEN] = "is_dump_stack";
module_param_string(func, func_name, KSYM_NAME_LEN, S_IRUGO);
static char ksym_name[KSYM_NAME_LEN] = "monitor_func";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
MODULE_PARM_DESC(ksym, "Kernel symbol to monitor; this module will report any"
			" write operations on the kernel symbol");

/* return 1 : print dump_stack()      *
 * return 0 : not print dump_stack()  */
static int is_dump_stack(void)
{
	unsigned long val = 0;
	val = nvt_readl(wp_addr);
	/* Step 9. Check for constrains */
	/* print mem for checking */
	pr_err("Address: 0x%08x value: 0x%08lx\n", wp_addr, val);

	/* condition holds: return 1 to print */
	if (val == wp_chk_val)
		return 1;

	/* default: return 0 and not print */
	return 0;
}

static int (*func)(void) = is_dump_stack;

static struct perf_event * __percpu *sample_hbp;
static struct perf_event * __percpu *sample_hbp_x;
static struct perf_event_attr attr;

#if ENABLE_SYSFS
static struct class *g_dev_class;
static char *DevNode(struct device *dev, umode_t *mode)
{
    *mode = 0666;
	return NULL;
}
#endif

static void sample_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs);

static void sample_hbp_handler_x(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	int this_cpu = get_cpu();

	#if CONFIG_PER_CORE
	unregister_hw_breakpoint(per_cpu(*sample_hbp_x, this_cpu));
	#else
	unregister_wide_hw_breakpoint(sample_hbp_x);
	#endif

	/* Step 7/8. Next PC address is executed. Call func to check if constrain is matched. */
	if (func()) {

		// dump stack / backtrace for wp_addr
		dump_stack();
		pr_err("x trap addr:0x%lX\n", regs->ARM_pc);
		pr_err("hit ! [0x%08X] 0x%08X\n", wp_addr, *(unsigned int *)wp_addr);
	}

	#if CONFIG_PER_CORE
	per_cpu(*sample_hbp, this_cpu) = perf_event_create_kernel_counter(&attr, this_cpu, NULL, sample_hbp_handler, NULL);
	#else
	sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
	#endif
	put_cpu();
}

static void sample_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	struct perf_event_attr attr_x;
	int this_cpu = get_cpu();
	int ret = 0;

	ret = strncmp("monitor_func", ksym_name, strlen(ksym_name));
	if (ret == 0)
		pr_err("Value will be changed by cpu %d: data_addr:0x%08X data_val:0x%08x inst_addr:0x%lx\n", this_cpu, wp_addr, nvt_readl(wp_addr), regs->ARM_pc);
	else
		pr_err("%s value will be changed by cpu %d: data_addr:0x%08X data_val:0x%08x inst_addr:0x%lx\n", ksym_name, this_cpu, wp_addr, nvt_readl(wp_addr), regs->ARM_pc);

	#if CONFIG_PER_CORE
	unregister_hw_breakpoint(per_cpu(*sample_hbp, this_cpu));
	#else
	unregister_wide_hw_breakpoint(sample_hbp);
	#endif

	/* Step 2/3/4/5/6. Setup for HW breakpoint on next PC address if new value is written to specific address. */
	hw_breakpoint_init(&attr_x);
	attr_x.bp_addr = regs->ARM_pc + 4;
	attr_x.bp_len = HW_BREAKPOINT_LEN_4;
	attr_x.bp_type = HW_BREAKPOINT_X;

	#if CONFIG_PER_CORE
	per_cpu(*sample_hbp_x, this_cpu) = perf_event_create_kernel_counter(&attr_x, this_cpu, NULL, sample_hbp_handler_x, NULL);
	#else
	sample_hbp_x = register_wide_hw_breakpoint(&attr_x, sample_hbp_handler_x, NULL);
	if (IS_ERR((void __force *)sample_hbp_x)) {
		ret = PTR_ERR((void __force *)sample_hbp_x);
		pr_err("Fail register x\n");
	}
	#endif

	put_cpu();
}

#if ENABLE_SYSFS
static ssize_t sys_store_func(struct device* dev, struct device_attribute* attr, const char* buf, size_t len)
{
	int (*new_func)(void);
	char name[KSYM_NAME_LEN];

	if (len >= KSYM_NAME_LEN) {
		pr_err("error: input func_name is too long.\n");
		return len;
	}

	strncpy(name, buf, KSYM_NAME_LEN - 1);

	if (name[len - 1] == '\n' || name[len - 1] == '\r') {
		name[len - 1] = 0;
		if (name[len - 2] == '\n' || name[len - 2] == '\r')
			name[len - 2] = 0;
	}

	new_func = (int (*)(void))kallsyms_lookup_name(name);
	if (!new_func) {
		pr_err("error: cannot found func_name %s. Nothing change.\n", name);
	}
	else {
		strncpy(func_name, name, KSYM_NAME_LEN - 1);

		func = new_func;

		pr_err("use %s() at 0x%08x\n", func_name, (unsigned int)func);
	}

	return len;
}

static ssize_t sys_show_func(struct device* dev, struct device_attribute* attr, char* buf)
{
	return snprintf(buf, PAGE_SIZE, "%s() at 0x%08x\n", func_name, (unsigned int)func);
}

static DEVICE_ATTR(show_func, S_IRUGO | S_IWUSR, sys_show_func, sys_store_func);

static struct attribute *hw_wp_attrs[] = {
	&dev_attr_show_func.attr,
	NULL,
};

static struct attribute_group hw_wp_attr_grp = {
	.attrs = hw_wp_attrs,
};
#endif /* end of ENABLE_SYSFS */

static int hw_break_module_init(void)
{
	int ret;
	int i;
	unsigned long addr;
#if ENABLE_SYSFS
	struct device *dev;
#endif

	func = (int (*)(void))kallsyms_lookup_name(func_name);
	if (!func) {
		func = is_dump_stack;
		pr_err("cannot found func_name %s, use default function.\n", func_name);
	}
	else {
		pr_err("use %s()\n", func_name);
	}

#if ENABLE_SYSFS
	g_dev_class = class_create(THIS_MODULE, ksym_name);
	if (IS_ERR(g_dev_class))
	{
		pr_err("error: cannot create class\n");
	    return PTR_ERR(g_dev_class);
	}
	g_dev_class->devnode = DevNode;

	dev = device_create(g_dev_class, NULL, 0, NULL, ksym_name);
	if (IS_ERR(dev))
	{
		pr_err("error: cannot create device in class\n");
	    return PTR_ERR(dev);
	}

	ret = sysfs_create_group(&dev->kobj, &hw_wp_attr_grp);
	if (ret < 0) {
		pr_err("failed to create /sys endpoint\n");
		return ret;
	}
#endif

#if ENABLE_CACHE_FLUSH_WORKAROUND
	// patch DCIMVAC -> DCCIMVAC in v7_dma_inv_range()
	addr = kallsyms_lookup_name("v7_dma_inv_range");
	pr_err("v7_dma_inv_range: 0x%08lx\n", addr);
	// ee070f36 mcr p15, 0, r0, cr7, cr6, {1}
	#define ARM_v8_INSTRUCTION_DCIMVAC_r0	0xee070f36
	// ee070f3e mcr p15, 0, r0, cr7, cr14, {1}
	#define ARM_v8_INSTRUCTION_DCCIMVAC_r0	0xee070f3e
	for (i = 0; i < 0x40; i += 4)
		if (*(unsigned int *)(addr + i) == ARM_v8_INSTRUCTION_DCIMVAC_r0) {
			pr_err("found DCIMVAC\n");
			pr_err("[0x%08lx] = 0x%08x\n", (addr + i), *(unsigned int *)(addr + i));
			*(unsigned int *)(addr + i) = ARM_v8_INSTRUCTION_DCCIMVAC_r0;
			dma_sync_single_for_cpu( NULL, virt_to_phys((void *)(addr + i)), 4, DMA_FROM_DEVICE);
			pr_err("[0x%08lx] = 0x%08x\n", (addr + i), *(unsigned int *)(addr + i));
			break;
		}
#endif

	/* Step 1. Watchpoint for specific address watch */
	sample_hbp_x = alloc_percpu(typeof(*sample_hbp_x));


	hw_breakpoint_init(&attr);

	ret = strncmp("monitor_func", ksym_name, strlen(ksym_name));
	if (ret == 0) {
		attr.bp_addr = wp_addr;
		attr.bp_len = HW_BREAKPOINT_LEN_4;
		attr.bp_type = HW_BREAKPOINT_W;
		pr_info("Address type is configured\n");
	} else {
		attr.bp_addr = wp_addr = kallsyms_lookup_name(ksym_name);
		attr.bp_len = HW_BREAKPOINT_LEN_4;
		attr.bp_type = HW_BREAKPOINT_W;
		pr_info("Symbol type is configured\n");
	}

	sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
	if (IS_ERR((void __force *)sample_hbp)) {
		ret = PTR_ERR((void __force *)sample_hbp);
		goto fail;
	}

	ret = strncmp("monitor_func", ksym_name, strlen(ksym_name));
	if (ret == 0) {
		pr_info("HW Watchpoint for write installed at [0x%08llx]: 0x%08x\n", attr.bp_addr, nvt_readl((unsigned long)attr.bp_addr));
	} else {
		pr_info("HW Watchpoint for %s write installed at [0x%08llx]: 0x%08x\n", ksym_name, attr.bp_addr, nvt_readl((unsigned long)attr.bp_addr));
	}

	return 0;

fail:
	pr_err("Watchpoint registration failed\n");

	return ret;
}

static void __exit hw_break_module_exit(void)
{
	unregister_wide_hw_breakpoint(sample_hbp);
	pr_err("HW Watchpoint for %s write uninstalled\n", ksym_name);
}

module_init(hw_break_module_init);
module_exit(hw_break_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NOVATEK");
MODULE_DESCRIPTION("watchpoint");
