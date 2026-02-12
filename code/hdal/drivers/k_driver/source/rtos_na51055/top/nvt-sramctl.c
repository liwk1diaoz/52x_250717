/**
    NVT SRAM Contrl
    This file will Enable and disable SRAM shutdown
    @file       nvt-sramctl.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include "nvt-sramctl.h"
//#include <linux/mutex.h>
//#include <plat/hardware.h>
#include <kwrap/nvt_type.h>
#include "rcw_macro.h"
#include "io_address.h"
#include <kwrap/spinlock.h>

//struct mutex top_lock;

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define NVT_TOP_SETREG(ofs, value)	OUTW(IOADDR_TOP_REG_BASE + (ofs), value)
#define NVT_TOP_GETREG(ofs)		INW(IOADDR_TOP_REG_BASE + (ofs))


//#define loc_cpu() {mutex_lock(&top_lock);}
//#define unl_cpu() {mutex_unlock(&top_lock);}


#define SRAM_OFS 0x1000

void nvt_disable_sram_shutdown(SRAM_SD id)
{
	UINT32 reg_data, reg_ofs;
	unsigned long      flags;

	reg_ofs = (id >> 5) << 2;

	loc_cpu(flags);

	reg_data = NVT_TOP_GETREG(SRAM_OFS + reg_ofs);

	reg_data &= ~(1 << (id & 0x1F));

	NVT_TOP_SETREG(SRAM_OFS + reg_ofs, reg_data);

	unl_cpu(flags);
}

void nvt_enable_sram_shutdown(SRAM_SD id)
{
	UINT32 reg_data, reg_ofs;
	unsigned long      flags;

	reg_ofs = (id >> 5) << 2;

	loc_cpu(flags);

	reg_data = NVT_TOP_GETREG(SRAM_OFS + reg_ofs);

	reg_data |= (1 << (id & 0x1F));

	NVT_TOP_SETREG(SRAM_OFS + reg_ofs, reg_data);

	unl_cpu(flags);
}

//static int __init nvt_init_sram_mutex(void)
//{
//	mutex_init(&top_lock);
//	return 0;
//}

//core_initcall(nvt_init_sram_mutex);
