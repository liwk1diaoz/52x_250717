#ifndef __SDE_DRV_H__
#define __SDE_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#include <kwrap/spinlock.h>




//#define MODULE_IRQ_NUM          1
//#define MODULE_REG_NUM          1
//#define MODULE_CLK_NUM          1

#define SDE_IRQ_NUM          1
#define SDE_REG_NUM          1
#define SDE_CLK_NUM          1

extern struct clk *sde_clk[SDE_CLK_NUM];

typedef struct sde_info {
	struct completion sde_completion;
	struct semaphore sde_sem;
	struct clk *pclk[SDE_CLK_NUM];
	struct clk *p_pclk[SDE_CLK_NUM];
	struct tasklet_struct sde_tasklet;
	void __iomem *io_addr[SDE_REG_NUM];
	int iinterrupt_id[SDE_IRQ_NUM];
	wait_queue_head_t sde_wait_queue;
	vk_spinlock_t sde_spinlock;
} SDE_INFO, *PSDE_INFO;

int nvt_sde_drv_open(PSDE_INFO pmodule_info, unsigned char if_id);
int nvt_sde_drv_release(PSDE_INFO pmodule_info, unsigned char if_id);
int nvt_sde_drv_init(PSDE_INFO pmodule_info);
int nvt_sde_drv_remove(PSDE_INFO pmodule_info);
int nvt_sde_drv_suspend(SDE_INFO *pmodule_info);
int nvt_sde_drv_resume(SDE_INFO *pmodule_info);
int nvt_sde_drv_ioctl(unsigned char if_id, SDE_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_sde_drv_write_reg(PSDE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_sde_drv_read_reg(PSDE_INFO pmodule_info, unsigned long addr);
#endif

