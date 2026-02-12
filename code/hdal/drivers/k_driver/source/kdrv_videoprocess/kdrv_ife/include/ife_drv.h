#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

extern struct clk *ife_clk[MODULE_CLK_NUM];

typedef struct ife_module_info {
	struct completion ife_completion;
	struct vk_semaphore ife_sem;
	struct clk *pclk[MODULE_CLK_NUM];
   	struct clk *p_pclk[MODULE_CLK_NUM];
	struct tasklet_struct ife_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t ife_wait_queue;
	vk_spinlock_t ife_spinlock;
} IFE_MODULE_INFO, *PIFE_MODULE_INFO;

int nvt_ife_drv_open(PIFE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_ife_drv_release(PIFE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_ife_drv_init(PIFE_MODULE_INFO pmodule_info);
int nvt_ife_drv_remove(PIFE_MODULE_INFO pmodule_info);
int nvt_ife_drv_suspend(IFE_MODULE_INFO *pmodule_info);
int nvt_ife_drv_resume(IFE_MODULE_INFO *pmodule_info);
int nvt_ife_drv_ioctl(unsigned char ucIF, IFE_MODULE_INFO *pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_ife_drv_write_reg(PIFE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_ife_drv_read_reg(PIFE_MODULE_INFO pmodule_info, unsigned long addr);

irqreturn_t nvt_ife_drv_isr(int irq, void *devid);

#endif

