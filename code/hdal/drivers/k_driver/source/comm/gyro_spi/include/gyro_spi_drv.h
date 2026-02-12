#ifndef __GYRO_DRV_H__
#define __GYRO_DRV_H__
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#include "gyro_spi_dbg.h"

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

#if (NVT_GYRO_528_PINMUX == ENABLE)
#define TOP_REG_ADDR            0xf0010000
#define PAD_REG_ADDR            0xf0030000
#elif (NVT_GYRO_530_PINMUX == ENABLE)
#define TOP_REG_ADDR            0x2f0010000
#define PAD_REG_ADDR            0x2f0030000
#endif

typedef struct gyro_module_info {
	struct semaphore gyro_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	void __iomem* io_addr[MODULE_REG_NUM];
    int intr_id[MODULE_IRQ_NUM];
    wait_queue_head_t gyro_wait_queue;
} GYRO_MODULE_INFO, *PGYRO_MODULE_INFO;

int nvt_gyro_spi_drv_open(GYRO_MODULE_INFO *pmodule_info, unsigned char ucIF);
int nvt_gyro_spi_drv_release(GYRO_MODULE_INFO *pmodule_info, unsigned char ucIF);
int nvt_gyro_spi_drv_init(GYRO_MODULE_INFO *pmodule_info);
int nvt_gyro_spi_drv_remove(GYRO_MODULE_INFO *pmodule_info);
int nvt_gyro_spi_drv_suspend(GYRO_MODULE_INFO* pmodule_info);
int nvt_gyro_spi_drv_resume(GYRO_MODULE_INFO* pmodule_info);

// workaround for 528 experiment code, force gyro pinmux
void nvt_gyro_spi_drv_pinmux_setting(void);

//test: test function
void emu_gyroAuto(u32 cur_clk_rate);
void emu_spi3Wire(u32 cur_clk_rate);
void pmc_read_id_new(u32 cur_clk_rate);
#endif

