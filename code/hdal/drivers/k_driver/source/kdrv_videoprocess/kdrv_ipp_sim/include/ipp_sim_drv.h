#ifndef __IPP_SIM_DRV_H__
#define __IPP_SIM_DRV_H__

#if defined(__LINUX)
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif

#define IPP_SIM_IRQ_NUM          1
#define IPP_SIM_REG_NUM          1
#define IPP_SIM_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

extern struct clk *ipp_sim_clk[IPP_SIM_CLK_NUM];

typedef struct ipp_sim_info {
#if defined(__LINUX)
	struct completion ipp_sim_completion;
	struct semaphore ipp_sim_sem;
	struct clk *pclk[IPP_SIM_CLK_NUM];
	struct tasklet_struct ipp_sim_tasklet;
	void __iomem *io_addr[IPP_SIM_REG_NUM];
	int iinterrupt_id[IPP_SIM_IRQ_NUM];
	wait_queue_head_t ipp_sim_wait_queue;
	spinlock_t ipp_sim_spinlock;
#endif
} IPP_SIM_INFO, *PIPP_SIM_INFO;

#if defined(__LINUX)
int nvt_ipp_sim_drv_open(PIPP_SIM_INFO pipp_sim_info, unsigned char ucIF);
int nvt_ipp_sim_drv_release(PIPP_SIM_INFO pipp_sim_info, unsigned char ucIF);
int nvt_ipp_sim_drv_init(PIPP_SIM_INFO pipp_sim_info);
int nvt_ipp_sim_drv_remove(PIPP_SIM_INFO pipp_sim_info);
int nvt_ipp_sim_drv_suspend(IPP_SIM_INFO *pipp_sim_info);
int nvt_ipp_sim_drv_resume(IPP_SIM_INFO *pipp_sim_info);
int nvt_ipp_sim_drv_ioctl(unsigned char ucIF, IPP_SIM_INFO *pipp_sim_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_ipp_sim_drv_write_reg(PIPP_SIM_INFO pipp_sim_info, unsigned long addr, unsigned long value);
int nvt_ipp_sim_drv_read_reg(PIPP_SIM_INFO pipp_sim_info, unsigned long addr);
#endif

#endif

