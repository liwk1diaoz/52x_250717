/*
    VPE module driver

    NT96520 VPE module driver.

    @file       vpe_platform.c
    @ingroup    mIIPPVPE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "vpe_platform.h"


static SEM_HANDLE semid_vpe;
static ID     flg_id_vpe;
static UINT32 vpe_eng_clock_rate;




// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int vpe_debug_level = NVT_DBG_IND;
#else
#endif

static BOOL g_vpe_clk_en = FALSE;

//=========================================================================
#if defined (__KERNEL__)

static VK_DEFINE_SPINLOCK(vpe_spin_lock);
//static unsigned long vpe_spin_flags;

struct clk *vpe_clk[VPE_CLK_NUM];

UINT32 _vpe_reg_io_base;

//=========================================================================
#elif defined (__FREERTOS)

static VK_DEFINE_SPINLOCK(vpe_spin_lock);
//static unsigned long vpe_spin_flags;


//=========================================================================
#elif defined (__UITRON) || defined(__ECOS)

#else

#endif

static BOOL g_vpe_module_init_done = FALSE;
static BOOL vpe_ctrl_flow_to_do = DISABLE;


VOID vpe_platform_set_init_status(BOOL set_status)
{
	g_vpe_module_init_done = set_status;
}

BOOL vpe_platform_get_init_status(VOID)
{
	return g_vpe_module_init_done;
}

ER vpe_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_vpe, flg);
}
//---------------------------------------------------------------

ER vpe_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, flg_id_vpe, flg, TWF_CLR | TWF_ORW);
}
//---------------------------------------------------------------

ER vpe_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_vpe, flg);
}
//---------------------------------------------------------------

ER vpe_platform_sem_wait(VOID)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_vpe);
#else
	return SEM_WAIT(semid_vpe);
#endif
}
//---------------------------------------------------------------

ER vpe_platform_sem_signal(VOID)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_vpe);
#else
	SEM_SIGNAL(semid_vpe);
	return E_OK;
#endif
}
//---------------------------------------------------------------

unsigned long vpe_platform_spin_lock(VOID)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();

	return 0;
#else
	unsigned long loc_status;

	vk_spin_lock_irqsave(&vpe_spin_lock, loc_status);

	return loc_status;
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_spin_unlock(unsigned long loc_status)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&vpe_spin_lock, loc_status);
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_int_enable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_VPE);
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_int_disable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_VPE);
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_enable_clk(VOID)
{
	if (g_vpe_clk_en == FALSE) {
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_enableClock(VPE_CLK);
#else
		if (IS_ERR(vpe_clk[0])) {
			DBG_WRN("VPE: get clk fail 0x%p\r\n", vpe_clk[0]);
		} else {
			clk_enable(vpe_clk[0]);
		}
#endif
		g_vpe_clk_en = TRUE;
	}
}
//---------------------------------------------------------------

VOID vpe_platform_disable_clk(VOID)
{
	if (g_vpe_clk_en == TRUE) {

#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_disableClock(VPE_CLK);
#else
		if (IS_ERR(vpe_clk[0])) {
			//DBG_WRN("VPE: get clk fail 0x%p\r\n", vpe_clk[0]);
			vk_print_isr("VPE: get clk fail 0x%p\r\n", vpe_clk[0]);
		} else {
			clk_disable(vpe_clk[0]);
		}
#endif

		g_vpe_clk_en = FALSE;
	}
}
//---------------------------------------------------------------

VOID vpe_platform_prepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(vpe_clk[0])) {
		DBG_WRN("VPE: get clk fail 0x%p\r\n", vpe_clk[0]);
	} else {
		clk_prepare(vpe_clk[0]);
	}
#else
	pll_enableSystemReset(VPE_RSTN);
	pll_disableSystemReset(VPE_RSTN);
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_unprepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(vpe_clk[0])) {
		DBG_WRN("VPE: get clk fail 0x%p\r\n", vpe_clk[0]);
	} else {
		clk_unprepare(vpe_clk[0]);
	}
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_disable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_disable_sram_shutdown(VPE_SD);
#elif defined (__FREERTOS)
#if defined (_BSP_NA51055_)
	nvt_disable_sram_shutdown(VPE_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_disableSramShutDown(VPE_RSTN);
#else
#endif
}
//---------------------------------------------------------------

VOID vpe_platform_enable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_enable_sram_shutdown(VPE_SD);
#elif defined (__FREERTOS)
#if defined (_BSP_NA51055_)
	nvt_enable_sram_shutdown(VPE_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_enableSramShutDown(VPE_RSTN);
#else
#endif
}
//---------------------------------------------------------------


ER vpe_platform_set_clk_rate(UINT32 source_clk)
{
#if defined (__KERNEL__)

	struct clk *parent_clk;

	if (IS_ERR(vpe_clk[0])) {
		DBG_ERR("VPE: get clk fail...0x%p\r\n", vpe_clk[0]);
		return E_SYS;
	}
#if 0
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk == 280) {
			parent_clk = clk_get(NULL, "pll13");
			vpe_eng_clock_rate = 280;
		} else {
			DBG_WRN("VPE: input frequency %d round to 280MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			vpe_eng_clock_rate = 280;
		}
	} else {
		if (source_clk < 240) {
			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			vpe_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			parent_clk = clk_get(NULL, "fix320m");
			vpe_eng_clock_rate = 320;
		} else if (source_clk == 420) {
			parent_clk = clk_get(NULL, "pll13");
			vpe_eng_clock_rate = 420;
		} else {
			DBG_WRN("VPE: input frequency %d round to 420MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			vpe_eng_clock_rate = 420;
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		DBG_ERR("VPE: do not support\r\n");
	} else {
		if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			vpe_eng_clock_rate = 320;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			vpe_eng_clock_rate = 240;
		} else if ((source_clk > 320) && (source_clk <= 360)) {
			parent_clk = clk_get(NULL, "pll13");
			vpe_eng_clock_rate = 0;
		} else {
			parent_clk = clk_get(NULL, "pll17");
			vpe_eng_clock_rate = 0;
		}

		clk_set_parent(vpe_clk[0], parent_clk);
		clk_put(parent_clk);
	}
#endif


#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

	UINT32 selected_clock;;

	// Turn on power
	pmc_turnonPower(PMC_MODULE_VPE);

#if 0
	// select clock
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;

			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;

			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 280) {
			selected_clock = PLL_CLKSEL_VPE_PLL13;
			vpe_eng_clock_rate = 280;
		} else {
			selected_clock = PLL_CLKSEL_VPE_PLL13;
			vpe_eng_clock_rate = 280;

			DBG_WRN("VPE: input frequency %d round to 280MHz\r\n", (int)source_clk);
		}

		if (selected_clock == PLL_CLKSEL_VPE_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
		if (source_clk < 240) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;

			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;

			DBG_WRN("VPE: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 320) {
			selected_clock = PLL_CLKSEL_VPE_320;
			vpe_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			selected_clock = PLL_CLKSEL_VPE_320;
			vpe_eng_clock_rate = 320;
			DBG_WRN("VPE: input frequency %d round to 320MHz\r\n", (int)source_clk);
		} else if (source_clk == 420) {
			selected_clock = PLL_CLKSEL_VPE_PLL13;
			vpe_eng_clock_rate = 420;
			DBG_WRN("VPE: input frequency %d round to 420MHz\r\n", (int)source_clk);
		} else {
			selected_clock = PLL_CLKSEL_VPE_PLL13;
			vpe_eng_clock_rate = 420;

			DBG_WRN("VPE: input frequency %d round to 420MHz\r\n", (int)source_clk);
		}

		if (selected_clock == PLL_CLKSEL_VPE_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}

		if (selected_clock == PLL_CLKSEL_VPE_320) {
			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		DBG_ERR("VPE: do not support\r\n");
	} else {
		if (source_clk == 320) {
			selected_clock = PLL_CLKSEL_VPE_320;
			vpe_eng_clock_rate = 320;

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_VPE_240;
			vpe_eng_clock_rate = 240;
		} else if ((source_clk > 320) && (source_clk <= 360)) {
			selected_clock = PLL_CLKSEL_VPE_PLL13;
			vpe_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		} else {
			selected_clock = PLL_CLKSEL_VPE_PLL17;
			vpe_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_17) == FALSE) {
				pll_setPLLEn(PLL_ID_17, TRUE);
			}
		}
	}
#endif

	pll_setClockRate(PLL_CLKSEL_VPE, selected_clock);
#else

#endif

	return E_OK;

}
//---------------------------------------------------------------


UINT32 vpe_platform_get_clk_rate(VOID)
{
	if (vpe_eng_clock_rate == 0) {
		DBG_WRN("VPE: clock source: PLL13, input frequency is defined by system\r\n");
	}
	return vpe_eng_clock_rate;
}
//---------------------------------------------------------------

UINT32 vpe_platform_va2pa(UINT32 addr)
{
#if defined (__KERNEL__)

	return vos_cpu_get_phy_addr(addr);

#elif defined (__FREERTOS)

	return vos_cpu_get_phy_addr(addr);

#elif defined (__UITRON) || defined (__ECOS)

	return vos_cpu_get_phy_addr(addr);

#else

	return addr;

#endif
}
//---------------------------------------------------------------

UINT32 vpe_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}
//---------------------------------------------------------------

UINT32 vpe_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)
	//fmem_dcache_sync_vb((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);

	ret = 0;
#elif defined (__FREERTOS)
	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;
#else
	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------


UINT32 vpe_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)
	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);

	ret = 0;

#elif defined (__FREERTOS)
	//vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)

	//ret = dma_flushWriteCacheWithoutCheck(addr, size);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
	wmb();
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------


UINT32 vpe_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)

	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);

	ret = 0;

#elif defined (__FREERTOS)

	//vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);

	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	//ret = (UINT32)dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------

UINT32 vpe_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)

	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	ret = 0;

#elif defined (__FREERTOS)

	//vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	//ret = (UINT32)dma_flushReadCacheWithoutCheck(addr, size);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------

UINT32 vpe_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)

	//fmem_dcache_sync_vb((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);

	ret = 0;

#elif defined (__FREERTOS)

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------


UINT32 vpe_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)

	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	ret = 0;

#elif defined (__FREERTOS)

	//vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	//ret = (UINT32)dma_flushReadCacheWithoutCheck(addr, size);

	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------






#if !(defined __UITRON || defined __ECOS)
INT32 is_vpe_create = 0;

#if defined (__FREERTOS)
irqreturn_t vpe_platform_isr(int irq, VOID *devid)
{
	vpe_isr();
	return IRQ_HANDLED;
}

VOID vpe_platform_create_resource(VOID)
{
	if (!is_vpe_create) {
		OS_CONFIG_FLAG(flg_id_vpe);
		SEM_CREATE(semid_vpe, 1);

		request_irq(INT_ID_VPE, vpe_platform_isr, IRQF_TRIGGER_HIGH, "vpe_int", 0);

		is_vpe_create = 1;
		vpe_platform_set_init_status(TRUE);
		vpe_ctrl_flow_to_do = ENABLE;
	}
}
#else
VOID vpe_platform_create_resource(VPE_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(flg_id_vpe);
	SEM_CREATE(semid_vpe, 1);

	vpe_clk[0] = pmodule_info->pclk[0];
	_vpe_reg_io_base = (UINT32)pmodule_info->io_addr[0];

	//printk("vpe base addr: %08x\r\n", _vpe_reg_io_base);

	vpe_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		vpe_ctrl_flow_to_do = ENABLE;
	} else {
		vpe_ctrl_flow_to_do = DISABLE;
	}
}
#endif

VOID vpe_platform_release_resource(VOID)
{
	vpe_platform_set_init_status(FALSE);
	is_vpe_create = 0;

	rel_flg(flg_id_vpe);
	SEM_DESTROY(semid_vpe);
}
#endif



#if defined (__KERNEL__)
EXPORT_SYMBOL(vpe_platform_prepare_clk);
EXPORT_SYMBOL(vpe_platform_create_resource);
EXPORT_SYMBOL(vpe_platform_unprepare_clk);
#endif




