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

#include "test_drv.h"
#include "test_main.h"
#include "test_dbg.h"
#include "kdrv_videocapture/kdrv_ssenif.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int test_debug_level = NVT_DBG_WRN;
module_param_named(test_debug_level, test_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(test_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static TEST_DRV_INFO *ptest_drv_info;

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_test_module_init(void);
void __exit nvt_test_module_exit(void);

//=============================================================================
// function define
//=============================================================================
extern int nvt_test_thread(void *__test);



static struct platform_driver nvt_test_driver = {
	.driver = {
		.name   = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

int __init nvt_test_module_init(void)
{
	int ret;
	struct task_struct *th;

	nvt_dbg(WRN, "\n");

	ptest_drv_info = kzalloc(sizeof(TEST_DRV_INFO), GFP_KERNEL);
	if (!ptest_drv_info) {
		printk("failed to allocate memory\n");
		goto FAIL_INIT;
	}

	nvt_test_drv_init(&ptest_drv_info->module_info);

	ret = platform_driver_register(&nvt_test_driver);


	nvt_dbg(WRN, "run\n");

	th = kthread_run(nvt_test_thread, ptest_drv_info, "nvt_test_thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n", __func__, __LINE__);
		goto FAIL_INIT;
	}
	ptest_drv_info->emu_thread = th;



	return ret;

FAIL_INIT:
	kfree(ptest_drv_info);
	return -ENOMEM;
}

void __exit nvt_test_module_exit(void)
{

	kdrv_ssenif_trigger(KDRV_DEV_ID(0, KDRV_SSENIF_ENGINE_CSI0, 0), DISABLE);
	kdrv_ssenif_close(0, KDRV_SSENIF_ENGINE_CSI0);

	nvt_test_drv_remove(&ptest_drv_info->module_info);

	kfree(ptest_drv_info);

	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_test_driver);

}

module_init(nvt_test_module_init);
module_exit(nvt_test_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_ssenif test driver");
MODULE_LICENSE("GPL");
