/*
    IFE2 module driver

    NT98520 IFE2 module driver.

    @file       ife2_platform.c
    @ingroup    mIIPPIFE2
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "ife2_platform.h"
#include "ife2_int.h"

static SEM_HANDLE semid_ife2;
static ID     flg_id_ife2;

static UINT32 ife2_eng_clock_rate = 0;

// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int ife2_debug_level = NVT_DBG_IND;
#else
#endif

static BOOL g_ife2_clk_en = FALSE;
BOOL ife2_ctrl_flow_to_do = FALSE;
BOOL fw_ife2_power_saving_en = TRUE;


//=========================================================================
#if defined (__KERNEL__)

static VK_DEFINE_SPINLOCK(ife2_spin_lock);
//static unsigned long ife2_spin_flags;

struct clk *ife2_clk[IFE2_CLK_NUM];
struct clk *ife2_pclk[IFE2_CLK_NUM];


UINT32 _IFE2_REGIOBASE;


//=========================================================================
#elif defined (__FREERTOS)

static VK_DEFINE_SPINLOCK(ife2_spin_lock);
//static unsigned long ife2_spin_flags;


//=========================================================================
#elif defined (__UITRON) || defined(__ECOS)

#else

#endif

static BOOL g_ife2_module_init_done = FALSE;

VOID ife2_platform_set_init_status(BOOL set_status)
{
	g_ife2_module_init_done = set_status;
}

BOOL ife2_platform_get_init_status(VOID)
{
	return g_ife2_module_init_done;
}


ER ife2_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_ife2, flg);
}
//---------------------------------------------------------------

ER ife2_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, flg_id_ife2, flg, TWF_CLR | TWF_ORW);
}
//---------------------------------------------------------------

ER ife2_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_ife2, flg);
}
//---------------------------------------------------------------

ER ife2_platform_sem_wait(VOID)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_ife2);
#else
	return SEM_WAIT(semid_ife2);
#endif
}
//---------------------------------------------------------------

ER ife2_platform_sem_signal(VOID)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_ife2);
#else
	SEM_SIGNAL(semid_ife2);
	return E_OK;
#endif
}
//---------------------------------------------------------------

unsigned long ife2_platform_spin_lock(VOID)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long loc_status;

	vk_spin_lock_irqsave(&ife2_spin_lock, loc_status);

	return loc_status;
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_spin_unlock(unsigned long loc_status)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
	return 0;
#else
	vk_spin_unlock_irqrestore(&ife2_spin_lock, loc_status);
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_int_enable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_IFE2);
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_int_disable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_IFE2);
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_enable_clk(VOID)
{
	if (g_ife2_clk_en == FALSE) {

#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_enableClock(IFE2_CLK);
#else
		if (IS_ERR(ife2_clk[0])) {
			DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
		} else {
			clk_enable(ife2_clk[0]);
		}
#endif
		g_ife2_clk_en = TRUE;
	}
}
//---------------------------------------------------------------

VOID ife2_platform_disable_clk(VOID)
{
	if (g_ife2_clk_en == TRUE) {

#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_disableClock(IFE2_CLK);
#else
		if (IS_ERR(ife2_clk[0])) {
			DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
		} else {
			clk_disable(ife2_clk[0]);
		}
#endif
		g_ife2_clk_en = FALSE;
	}
}
//---------------------------------------------------------------

VOID ife2_platform_prepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(ife2_clk[0])) {
		DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
	} else {
		clk_prepare(ife2_clk[0]);
	}
#else
	pll_enableSystemReset(IFE2_RSTN);
	pll_disableSystemReset(IFE2_RSTN);
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_unprepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(ife2_clk[0])) {
		DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
	} else {
		clk_unprepare(ife2_clk[0]);
	}
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_disable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_disable_sram_shutdown(IFE2_SD);
#elif defined (__FREERTOS)

#if defined (_BSP_NA51055_)
	nvt_disable_sram_shutdown(IFE2_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_disableSramShutDown(IFE2_RSTN);
#else
#endif
}
//---------------------------------------------------------------

VOID ife2_platform_enable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_enable_sram_shutdown(IFE2_SD);
#elif defined (__FREERTOS)

#if defined (_BSP_NA51055_)
	nvt_enable_sram_shutdown(IFE2_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_enableSramShutDown(IFE2_RSTN);
#else
#endif
}
//---------------------------------------------------------------


ER ife2_platform_set_clk_rate(UINT32 source_clk)
{
#if defined (__KERNEL__)

	struct clk *parent_clk;

	if (IS_ERR(ife2_clk[0])) {
		DBG_ERR("IFE2: get clk fail...0x%p\r\n", ife2_clk[0]);
		return E_SYS;
	}

#if 0
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk == 280) {
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 280;
		} else {
			DBG_WRN("IFE2: input frequency %d round to 280MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 280;
		}
	} else {
		if (source_clk < 240) {
			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			ife2_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			DBG_WRN("IFE2: input frequency %d round to 320MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix320m");
			ife2_eng_clock_rate = 320;
		} else if (source_clk == 420) {
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 420;
		} else {
			DBG_WRN("IFE2: input frequency %d round to 420MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 420;
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else {
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 0;
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			ife2_eng_clock_rate = 320;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ife2_eng_clock_rate = 240;
		} else {
			parent_clk = clk_get(NULL, "pll13");
			ife2_eng_clock_rate = 0;
		}
	} else {
        DBG_ERR("IFE2: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }
#endif

	clk_set_parent(ife2_clk[0], parent_clk);
	clk_put(parent_clk);

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS )

	UINT32 uiSelectedClock;

	// Turn on power
	pmc_turnonPower(PMC_MODULE_IME);

#if 0
	// select clock
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;

			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;

			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 280) {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 280;
		} else {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 280;

			DBG_WRN("IFE2: input frequency %d round to 280MHz\r\n", (int)source_clk);
		}

		if (uiSelectedClock == PLL_CLKSEL_IFE2_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
		if (source_clk < 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;

			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;

			DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 320) {
			uiSelectedClock = PLL_CLKSEL_IFE2_320;
			ife2_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			uiSelectedClock = PLL_CLKSEL_IFE2_320;
			ife2_eng_clock_rate = 320;

			DBG_WRN("IFE2: input frequency %d round to 320MHz\r\n", (int)source_clk);
		} else if (source_clk == 420) {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 420;
		} else {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 420;

			DBG_WRN("IFE2: input frequency %d round to 420MHz\r\n", (int)source_clk);
		}

		if (uiSelectedClock == PLL_CLKSEL_IFE2_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}

		if (uiSelectedClock == PLL_CLKSEL_IFE2_320) {
			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		}
	}
#else
	// select clock
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;
		} else {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			uiSelectedClock = PLL_CLKSEL_IFE2_320;
			ife2_eng_clock_rate = 320;

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk == 240) {
			uiSelectedClock = PLL_CLKSEL_IFE2_240;
			ife2_eng_clock_rate = 240;
		} else {
			uiSelectedClock = PLL_CLKSEL_IFE2_PLL13;
			ife2_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
        DBG_ERR("IFE2: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }
#endif
	pll_setClockRate(PLL_CLKSEL_IFE2, uiSelectedClock);
#else

#endif

	return E_OK;

}
//---------------------------------------------------------------


UINT32 ife2_platform_get_clk_rate(VOID)
{
	if (ife2_eng_clock_rate == 0) {
		DBG_WRN("IFE2: clock source: PLL13, input frequency is defined by system\r\n");
	}

	return ife2_eng_clock_rate;
}
//---------------------------------------------------------------

UINT32 ife2_platform_va2pa(UINT32 addr)
{
	return vos_cpu_get_phy_addr(addr);
}
//---------------------------------------------------------------

UINT32 ife2_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}
//---------------------------------------------------------------

UINT32 ife2_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ife2_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ife2_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------

UINT32 ife2_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------

UINT32 ife2_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ife2_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------






#if !(defined __UITRON || defined __ECOS)
static INT32 is_ife2_create = 0;

#if defined (__FREERTOS)
irqreturn_t ife2_platform_isr(int irq, VOID *devid)
{
	ife2_isr();
	return IRQ_HANDLED;
}

VOID ife2_platform_create_resource(VOID)
{
	if (!is_ife2_create) {
		OS_CONFIG_FLAG(flg_id_ife2);
		SEM_CREATE(semid_ife2, 1);

		request_irq(INT_ID_IFE2, ife2_platform_isr, IRQF_TRIGGER_HIGH, "ife2_int", 0);

		is_ife2_create = 1;

		ifeg2 = (NT98520_IFE2_REGISTER_STRUCT *)((UINT32)_IFE2_REG_BASE_ADDR);
		ife2_platform_set_init_status(TRUE);

		ife2_ctrl_flow_to_do = ENABLE;
	}
}
#else
VOID ife2_platform_create_resource(IFE2_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(flg_id_ife2);
	SEM_CREATE(semid_ife2, 1);

	ife2_clk[0] = pmodule_info->pclk[0];
	ife2_pclk[0] = pmodule_info->p_pclk[0];
	_IFE2_REGIOBASE = (UINT32)pmodule_info->io_addr[0];

	ifeg2 = (NT98520_IFE2_REGISTER_STRUCT *)((UINT32)_IFE2_REG_BASE_ADDR);
	ife2_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		ife2_ctrl_flow_to_do = ENABLE;
	} else {
		ife2_ctrl_flow_to_do = DISABLE;
	}
}
#endif

VOID ife2_platform_release_resource(VOID)
{
	ife2_platform_set_init_status(FALSE);
	is_ife2_create = 0;

	rel_flg(flg_id_ife2);
	SEM_DESTROY(semid_ife2);

	ifeg2 = NULL;
}
#endif


