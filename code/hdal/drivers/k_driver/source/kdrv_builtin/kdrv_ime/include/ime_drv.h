#ifndef __IME_DRV_H__
#define __IME_DRV_H__
#include <linux/io.h>
#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>


#define IME_IRQ_NUM          1
#define IME_REG_NUM          1
#define IME_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)


typedef struct ime_info {
	struct completion ime_completion;
	struct vk_semaphore ime_sem;
	struct clk *pclk[IME_CLK_NUM];
	struct clk *p_pclk[IME_CLK_NUM];
	struct tasklet_struct ime_tasklet;
	void __iomem *io_addr[IME_REG_NUM];
	int iinterrupt_id[IME_IRQ_NUM];
	wait_queue_head_t ime_wait_queue;
	vk_spinlock_t ime_spinlock;
} IME_INFO, *PIME_INFO;


//extern struct clk *ime_clk[IME_CLK_NUM];

int nvt_ime_drv_open(PIME_INFO pime_info, unsigned char uc_if);
int nvt_ime_drv_release(PIME_INFO pime_info, unsigned char uc_if);
int nvt_ime_drv_init(PIME_INFO pime_info);
int nvt_ime_drv_remove(PIME_INFO pime_info);
int nvt_ime_drv_suspend(IME_INFO *pime_info);
int nvt_ime_drv_resume(IME_INFO *pime_info);
//int nvt_ime_drv_ioctl(unsigned char uc_if, IME_INFO *pime_info, unsigned int cmd, unsigned long arg);
int nvt_ime_drv_write_reg(PIME_INFO pime_info, unsigned long addr, unsigned long value);
int nvt_ime_drv_read_reg(PIME_INFO pime_info, unsigned long addr);

irqreturn_t nvt_ime_drv_isr(int irq, void *devid);

#endif

