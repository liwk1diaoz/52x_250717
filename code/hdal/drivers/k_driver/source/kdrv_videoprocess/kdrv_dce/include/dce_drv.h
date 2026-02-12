#ifndef __DCE_DRV_H__
#define __DCE_DRV_H__
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

typedef struct module_info {
	struct completion dce_completion;
	struct vk_semaphore dce_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct clk *p_pclk[MODULE_CLK_NUM];
	struct tasklet_struct dce_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t dce_wait_queue;
	vk_spinlock_t dce_spinlock;
} DCE_INFO, *PDCE_INFO;

int nvt_dce_drv_open(PDCE_INFO pmodule_info, unsigned char if_id);
int nvt_dce_drv_release(PDCE_INFO pmodule_info, unsigned char if_id);
int nvt_dce_drv_init(PDCE_INFO pmodule_info);
int nvt_dce_drv_remove(PDCE_INFO pmodule_info);
int nvt_dce_drv_suspend(DCE_INFO *pmodule_info);
int nvt_dce_drv_resume(DCE_INFO *pmodule_info);
//int nvt_dce_drv_ioctl(unsigned char if_id, DCE_INFO* pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_dce_drv_write_reg(PDCE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_dce_drv_read_reg(PDCE_INFO pmodule_info, unsigned long addr);

irqreturn_t nvt_dce_drv_isr(int irq, void *devid);

#endif

