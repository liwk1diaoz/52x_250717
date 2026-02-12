#ifndef __IPE_DRV_H__
#define __IPE_DRV_H__
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

// NOTE: XXXXX
//extern struct clk *ipe_clk[MODULE_CLK_NUM];

typedef struct ipe_info {
	struct completion ipe_completion;
	struct vk_semaphore ipe_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct clk *p_pclk[MODULE_CLK_NUM];
	struct tasklet_struct ipe_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t ipe_wait_queue;
	vk_spinlock_t ipe_spinlock;
} IPE_INFO, *PIPE_INFO;

int nvt_ipe_drv_open(PIPE_INFO pmodule_info, unsigned char if_id);
int nvt_ipe_drv_release(PIPE_INFO pmodule_info, unsigned char if_id);
int nvt_ipe_drv_init(PIPE_INFO pmodule_info);
int nvt_ipe_drv_remove(PIPE_INFO pmodule_info);
int nvt_ipe_drv_suspend(IPE_INFO *pmodule_info);
int nvt_ipe_drv_resume(IPE_INFO *pmodule_info);
int nvt_ipe_drv_ioctl(unsigned char if_id, IPE_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_ipe_drv_write_reg(PIPE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_ipe_drv_read_reg(PIPE_INFO pmodule_info, unsigned long addr);

irqreturn_t nvt_ipe_drv_isr(int irq, void *devid);

#endif

