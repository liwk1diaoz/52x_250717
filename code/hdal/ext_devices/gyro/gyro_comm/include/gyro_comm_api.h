#ifndef __module_api_h_
#define __module_api_h_
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          5
#define MODULE_CLK_NUM          1

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

typedef struct module_info {
	struct completion xxx_completion;
	struct semaphore xxx_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct xxx_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t xxx_wait_queue;
	spinlock_t xxx_spinlock;
} MODULE_INFO, *PMODULE_INFO;


int nvt_gyro_comm_init(void);
int nvt_gyro_comm_uninit(void);
long nvt_gyro_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#if defined(__LINUX)
int gyro_comm_cmd_execute(unsigned char argc, char **argv);

#endif

#endif
