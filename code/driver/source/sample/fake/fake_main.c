#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <asm/signal.h>

int __init hello_module_init(void)
{
	return 0;
}

void __exit hello_module_exit(void)
{
}

module_init(hello_module_init);
module_exit(hello_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("hello driver");
MODULE_LICENSE("GPL");
