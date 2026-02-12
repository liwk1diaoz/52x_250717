#ifndef __MSDCNVT_DRV_H
#define __MSDCNVT_DRV_H
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _MSDCNVT_INFO {
	int reversed;
} MSDCNVT_INFO, *PMSDCNVT_INFO;

int msdcnvt_drv_open(MSDCNVT_INFO *pmodule_info, unsigned char minor);
int msdcnvt_drv_release(MSDCNVT_INFO *pmodule_info, unsigned char minor);
int msdcnvt_drv_init(MSDCNVT_INFO *pmodule_info);
int msdcnvt_drv_remove(MSDCNVT_INFO *pmodule_info);
int msdcnvt_drv_suspend(MSDCNVT_INFO *pmodule_info);
int msdcnvt_drv_resume(MSDCNVT_INFO *pmodule_info);
int msdcnvt_drv_ioctl(unsigned char minor, MSDCNVT_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

