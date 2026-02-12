#ifndef __IFE2_DRV_H__
#define __IFE2_DRV_H__


#include <linux/io.h>
#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define IFE2_IRQ_NUM          1
#define IFE2_REG_NUM          1
#define IFE2_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)


typedef struct ife2_info {
	struct completion ife2_completion;
	struct vk_semaphore ife2_sem;
	struct clk *pclk[IFE2_CLK_NUM];
	struct clk *p_pclk[IFE2_CLK_NUM];
	struct tasklet_struct ife2_tasklet;
	void __iomem *io_addr[IFE2_REG_NUM];
	int iinterrupt_id[IFE2_IRQ_NUM];
	wait_queue_head_t ife2_wait_queue;
	vk_spinlock_t ife2_spinlock;
} IFE2_INFO, *PIFE2_INFO;

//extern struct clk *ife2_clk[IFE2_CLK_NUM];

int nvt_ife2_drv_open(PIFE2_INFO pife2_info, unsigned char ucIF);
int nvt_ife2_drv_release(PIFE2_INFO pife2_info, unsigned char ucIF);
int nvt_ife2_drv_init(PIFE2_INFO pife2_info);
int nvt_ife2_drv_remove(PIFE2_INFO pife2_info);
int nvt_ife2_drv_suspend(IFE2_INFO *pife2_info);
int nvt_ife2_drv_resume(IFE2_INFO *pife2_info);
//int nvt_ife2_drv_ioctl(unsigned char ucIF, IFE2_INFO *pife2_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_ife2_drv_write_reg(PIFE2_INFO pife2_info, unsigned long addr, unsigned long value);
int nvt_ife2_drv_read_reg(PIFE2_INFO pife2_info, unsigned long addr);

irqreturn_t nvt_ife2_drv_isr(int irq, void *devid);


#endif

