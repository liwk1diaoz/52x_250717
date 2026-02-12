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
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <asm/signal.h>

#include "audlib_emu_drv.h"
#include "audlib_emu_main.h"
#include "audlib_emu_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int audlib_emu_debug_level = NVT_DBG_WRN;
module_param_named(audlib_emu_debug_level, audlib_emu_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audlib_emu_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static AUDLIB_DRV_INFO *paudlib_drv_info;

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_audlib_emu_module_init(void);
void __exit nvt_audlib_emu_module_exit(void);

//=============================================================================
// function define
//=============================================================================
extern int nvt_audlib_emu_thread(void * __audlib_emu);



static struct platform_driver nvt_audlib_driver = {
	.driver = {
		.name   = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

int __init nvt_audlib_emu_module_init(void)
{
	int ret;
	struct task_struct *th;

	nvt_dbg(WRN, "\n");

	paudlib_drv_info = kzalloc(sizeof(AUDLIB_DRV_INFO), GFP_KERNEL);
	if (!paudlib_drv_info) {
		printk("failed to allocate memory\n");
		goto FAIL_INIT;
	}

	nvt_audlib_emu_drv_init(&paudlib_drv_info->module_info);

	ret = platform_driver_register(&nvt_audlib_driver);



	th = kthread_run(nvt_audlib_emu_thread, paudlib_drv_info, "nvt_audlib_emu_thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto FAIL_INIT;
	}
	paudlib_drv_info->emu_thread = th;



	return ret;

FAIL_INIT:
	kfree(paudlib_drv_info);
	paudlib_drv_info = NULL;
	return -ENOMEM;
}

void __exit nvt_audlib_emu_module_exit(void)
{

	nvt_audlib_emu_drv_remove(&paudlib_drv_info->module_info);

	kfree(paudlib_drv_info);
	paudlib_drv_info = NULL;

	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_audlib_driver);

}

module_init(nvt_audlib_emu_module_init);
module_exit(nvt_audlib_emu_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("audlib_emu driver");
MODULE_LICENSE("GPL");
