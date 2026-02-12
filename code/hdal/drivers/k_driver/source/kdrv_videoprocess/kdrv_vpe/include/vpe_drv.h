#ifndef __VPE_DRV_H__
#define __VPE_DRV_H__

#include <linux/io.h>
//#include <linux/spinlock.h>
//#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>


#define VPE_IRQ_NUM          1
#define VPE_REG_NUM          1
#define VPE_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

struct vpe_drv_lut2d_info {
	//u8 ddr_id;
	void *va_addr;
	UINT32 pa_addr;
	uint size;
};

typedef struct vpe_info {
	struct completion vpe_completion;
	struct vk_semaphore vpe_sem;
	struct clk *pclk[VPE_CLK_NUM];
	struct tasklet_struct vpe_tasklet;
	struct vpe_drv_lut2d_info lut2d;  //for dce 2d lut rot used
	void __iomem *io_addr[VPE_REG_NUM];
	int iinterrupt_id[VPE_IRQ_NUM];
	wait_queue_head_t vpe_wait_queue;
	vk_spinlock_t vpe_spinlock;
} VPE_INFO, *PVPE_INFO;


//extern struct clk *vpe_clk[VPE_CLK_NUM];

int nvt_vpe_drv_open(PVPE_INFO pvpe_info, unsigned char uc_if);
int nvt_vpe_drv_release(PVPE_INFO pvpe_info, unsigned char uc_if);
int nvt_vpe_drv_init(PVPE_INFO pvpe_info);
int nvt_vpe_drv_remove(PVPE_INFO pvpe_info);
int nvt_vpe_drv_suspend(VPE_INFO *pvpe_info);
int nvt_vpe_drv_resume(VPE_INFO *pvpe_info);
//int nvt_vpe_drv_ioctl(unsigned char uc_if, VPE_INFO *pvpe_info, unsigned int cmd, unsigned long arg);
int nvt_vpe_drv_write_reg(PVPE_INFO pvpe_info, unsigned long addr, unsigned long value);
int nvt_vpe_drv_read_reg(PVPE_INFO pvpe_info, unsigned long addr);

irqreturn_t nvt_vpe_drv_isr(int irq, void *devid);

#endif

