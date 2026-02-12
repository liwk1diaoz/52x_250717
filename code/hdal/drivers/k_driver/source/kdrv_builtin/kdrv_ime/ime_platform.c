/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_platform.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_platform.h"
#include "ime_int.h"


static SEM_HANDLE semid_ime;
static ID     flg_id_ime;
static UINT32 ime_eng_clock_rate;

BOOL ime_ctrl_flow_to_do = DISABLE;


// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int ime_debug_level = NVT_DBG_IND;
#else
#endif

static BOOL g_ime_clk_en = FALSE;

//=========================================================================
#if defined (__KERNEL__)

static VK_DEFINE_SPINLOCK(ime_spin_lock);
//static unsigned long ime_spin_flags;

struct clk *ime_clk[IME_CLK_NUM];
struct clk *ime_pclk[IME_CLK_NUM];


UINT32 _ime_reg_io_base;


//=========================================================================
#elif defined (__FREERTOS)

static VK_DEFINE_SPINLOCK(ime_spin_lock);
//static unsigned long ime_spin_flags;


//=========================================================================
#elif defined (__UITRON) || defined(__ECOS)

#else

#endif


static BOOL g_ime_module_init_done = FALSE;
BOOL fw_ime_power_saving_en = TRUE;


VOID ime_platform_set_init_status(BOOL set_status)
{
	g_ime_module_init_done = set_status;
}

BOOL ime_platform_get_init_status(VOID)
{
	return g_ime_module_init_done;
}

ER ime_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_ime, flg);
}
//---------------------------------------------------------------

ER ime_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, flg_id_ime, flg, TWF_CLR | TWF_ORW);
}
//---------------------------------------------------------------

ER ime_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_ime, flg);
}
//---------------------------------------------------------------

ER ime_platform_sem_wait(VOID)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_ime);
#else
	return SEM_WAIT(semid_ime);
#endif
}
//---------------------------------------------------------------

ER ime_platform_sem_signal(VOID)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_ime);
#else
	SEM_SIGNAL(semid_ime);
	return E_OK;
#endif
}
//---------------------------------------------------------------

unsigned long ime_platform_spin_lock(VOID)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long loc_status;
	vk_spin_lock_irqsave(&ime_spin_lock, loc_status);
	return loc_status;
#endif
}
//---------------------------------------------------------------

VOID ime_platform_spin_unlock(unsigned long loc_status)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&ime_spin_lock, loc_status);
#endif
}
//---------------------------------------------------------------

VOID ime_platform_int_enable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_IME);
#endif
}
//---------------------------------------------------------------

VOID ime_platform_int_disable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_IME);
#endif
}
//---------------------------------------------------------------

VOID ime_platform_enable_clk(VOID)
{
	if (g_ime_clk_en == FALSE) {
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_enableClock(IME_CLK);
#else
		if (IS_ERR(ime_clk[0])) {
			DBG_WRN("IME: get clk fail 0x%p\r\n", ime_clk[0]);
		} else {
			clk_enable(ime_clk[0]);
		}
#endif
		g_ime_clk_en = TRUE;
	}
}
//---------------------------------------------------------------

VOID ime_platform_disable_clk(VOID)
{
	if (g_ime_clk_en == TRUE) {

#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_disableClock(IME_CLK);
#else
		if (IS_ERR(ime_clk[0])) {
			DBG_WRN("IME: get clk fail 0x%p\r\n", ime_clk[0]);
		} else {
			clk_disable(ime_clk[0]);
		}
#endif

		g_ime_clk_en = FALSE;
	}
}
//---------------------------------------------------------------

VOID ime_platform_prepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(ime_clk[0])) {
		DBG_WRN("IME: get clk fail 0x%p\r\n", ime_clk[0]);
	} else {
		clk_prepare(ime_clk[0]);
	}
#else
	pll_enableSystemReset(IME_RSTN);
	pll_disableSystemReset(IME_RSTN);
#endif
}
//---------------------------------------------------------------

VOID ime_platform_unprepare_clk(VOID)
{
#if defined (__KERNEL__)
	if (IS_ERR(ime_clk[0])) {
		DBG_WRN("IME: get clk fail 0x%p\r\n", ime_clk[0]);
	} else {
		clk_unprepare(ime_clk[0]);
	}
#endif
}
//---------------------------------------------------------------

VOID ime_platform_disable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_disable_sram_shutdown(IME_SD);
#elif defined (__FREERTOS)
#if defined (_BSP_NA51055_)
	nvt_disable_sram_shutdown(IME_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_disableSramShutDown(IME_RSTN);
#else
#endif
}
//---------------------------------------------------------------

VOID ime_platform_enable_sram_shutdown(VOID)
{
#if defined (__KERNEL__)
	nvt_enable_sram_shutdown(IME_SD);
#elif defined (__FREERTOS)
#if defined (_BSP_NA51055_)
	nvt_enable_sram_shutdown(IME_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_enableSramShutDown(IME_RSTN);
#else
#endif
}
//---------------------------------------------------------------


ER ime_platform_set_clk_rate(UINT32 source_clk)
{
#if defined (__KERNEL__)

	struct clk *parent_clk;

	if (IS_ERR(ime_clk[0])) {
		DBG_ERR("IME: get clk fail...0x%p\r\n", ime_clk[0]);
		return E_SYS;
	}
#if 0
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk == 280) {
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 280;
		} else {
			DBG_WRN("IME: input frequency %d round to 280MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 280;
		}
	} else {
		if (source_clk < 240) {
			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			ime_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			parent_clk = clk_get(NULL, "fix320m");
			ime_eng_clock_rate = 320;
		} else if (source_clk == 420) {
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 420;
		} else {
			DBG_WRN("IME: input frequency %d round to 420MHz\r\n", (int)source_clk);
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 420;
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else {
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 0;
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
			ime_eng_clock_rate = 320;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			ime_eng_clock_rate = 240;
		} else {
			parent_clk = clk_get(NULL, "pll13");
			ime_eng_clock_rate = 0;
		}
	} else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }
#endif
	clk_set_parent(ime_clk[0], parent_clk);
	clk_put(parent_clk);

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

	UINT32 selected_clock;;

	// Turn on power
	pmc_turnonPower(PMC_MODULE_IME);

#if 0
	// select clock
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk < 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;

			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;
		} else if (source_clk < 280) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;

			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 280) {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 280;
		} else {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 280;

			DBG_WRN("IME: input frequency %d round to 280MHz\r\n", (int)source_clk);
		}

		if (selected_clock == PLL_CLKSEL_IME_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
		if (source_clk < 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;

			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;
		} else if (source_clk < 320) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;

			DBG_WRN("IME: input frequency %d round to 240MHz\r\n", (int)source_clk);
		} else if (source_clk == 320) {
			selected_clock = PLL_CLKSEL_IME_320;
			ime_eng_clock_rate = 320;
		} else if (source_clk < 420) {
			selected_clock = PLL_CLKSEL_IME_320;
			ime_eng_clock_rate = 320;
			DBG_WRN("IME: input frequency %d round to 320MHz\r\n", (int)source_clk);
		} else if (source_clk == 420) {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 420;
			DBG_WRN("IME: input frequency %d round to 420MHz\r\n", (int)source_clk);
		} else {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 420;

			DBG_WRN("IME: input frequency %d round to 420MHz\r\n", (int)source_clk);
		}

		if (selected_clock == PLL_CLKSEL_IME_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}

		if (selected_clock == PLL_CLKSEL_IME_320) {
			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;
		} else {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}

	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			selected_clock = PLL_CLKSEL_IME_320;
			ime_eng_clock_rate = 320;

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IME_240;
			ime_eng_clock_rate = 240;
		} else {
			selected_clock = PLL_CLKSEL_IME_PLL13;
			ime_eng_clock_rate = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }
#endif

	pll_setClockRate(PLL_CLKSEL_IME, selected_clock);
#else

#endif

	return E_OK;

}
//---------------------------------------------------------------


UINT32 ime_platform_get_clk_rate(VOID)
{
	if (ime_eng_clock_rate == 0) {
		DBG_WRN("IME: clock source: PLL13, input frequency is defined by system\r\n");
	}
	return ime_eng_clock_rate;
}
//---------------------------------------------------------------

UINT32 ime_platform_va2pa(UINT32 addr)
{
	return vos_cpu_get_phy_addr(addr);
}
//---------------------------------------------------------------

UINT32 ime_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}
//---------------------------------------------------------------

UINT32 ime_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ime_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ime_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------

UINT32 ime_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------

UINT32 ime_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------


UINT32 ime_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	ret = 0;

	return ret;
}
//---------------------------------------------------------------






#if !(defined __UITRON || defined __ECOS)
INT32 is_ime_create = 0;

#if defined (__FREERTOS)
irqreturn_t ime_platform_isr(int irq, VOID *devid)
{
	ime_isr();
	return IRQ_HANDLED;
}

VOID ime_platform_create_resource(VOID)
{
	if (!is_ime_create) {
		OS_CONFIG_FLAG(flg_id_ime);
		SEM_CREATE(semid_ime, 1);

		request_irq(INT_ID_IME, ime_platform_isr, IRQF_TRIGGER_HIGH, "ime_int", 0);

		is_ime_create = 1;
		imeg = (NT98520_IME_REGISTER_STRUCT *)((UINT32)_IME_REG_BASE_ADDR);
		ime_platform_set_init_status(TRUE);
		ime_ctrl_flow_to_do = ENABLE;
	}
}
#else
VOID ime_platform_create_resource(IME_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(flg_id_ime);
	SEM_CREATE(semid_ime, 1);

	ime_clk[0] = pmodule_info->pclk[0];
	ime_pclk[0] = pmodule_info->p_pclk[0];

	_ime_reg_io_base = (UINT32)pmodule_info->io_addr[0];

	imeg = (NT98520_IME_REGISTER_STRUCT *)((UINT32)_IME_REG_BASE_ADDR);
	ime_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		ime_ctrl_flow_to_do = ENABLE;
	} else {
		ime_ctrl_flow_to_do = DISABLE;
	}
}
#endif

VOID ime_platform_release_resource(VOID)
{
	ime_platform_set_init_status(FALSE);
	is_ime_create = 0;

	rel_flg(flg_id_ime);
	SEM_DESTROY(semid_ime);

	imeg = NULL;
}
#endif



#if defined (__KERNEL__)
EXPORT_SYMBOL(ime_platform_prepare_clk);
EXPORT_SYMBOL(ime_platform_create_resource);
EXPORT_SYMBOL(ime_platform_unprepare_clk);

EXPORT_SYMBOL(ime_platform_set_clk_rate);
#endif



