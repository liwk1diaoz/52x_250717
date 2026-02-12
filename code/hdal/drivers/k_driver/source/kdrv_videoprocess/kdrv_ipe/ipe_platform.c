

#include "ipe_platform.h"
#include "ipe_int.h"

// semaphore & interrupt flag id
static SEM_HANDLE SEMID_IPE;
static ID FLG_ID_IPE;
static UINT32 ipe_clock;

// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int ipe_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
#else
#endif

UINT32 ipe_dbg_cmd = IPE_DBG_DEFAULTON;

#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
static VK_DEFINE_SPINLOCK(ipe_spin_lock);
#else
static VK_DEFINE_SPINLOCK(ipe_spin_lock);
struct clk *ipe_clk[MODULE_CLK_NUM];
struct clk *ipe_pclk[MODULE_CLK_NUM];

// NOTE: XXXXX
UINT32 _IPE_REG_BASE_ADDR;
#endif

static BOOL g_ipe_module_init_done = FALSE;
BOOL ipe_ctrl_flow_to_do = DISABLE;
BOOL fw_ipe_power_saving_en = TRUE;



void ipe_platform_set_init_status(BOOL set_status)
{
	g_ipe_module_init_done = set_status;
}

BOOL ipe_platform_get_init_status(void)
{
	return g_ipe_module_init_done;
}


ER ipe_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(FLG_ID_IPE, flg);
}

ER ipe_platform_flg_set(FLGPTN flg)
{
	return iset_flg(FLG_ID_IPE, flg);
}

ER ipe_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, FLG_ID_IPE, flg, TWF_CLR | TWF_ORW);
}

ER ipe_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_IPE);
#else
	return SEM_WAIT(SEMID_IPE);
#endif
}

ER ipe_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_IPE);
#else
	SEM_SIGNAL(SEMID_IPE);
	return E_OK;
#endif
}

unsigned long ipe_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long flag;

	vk_spin_lock_irqsave(&ipe_spin_lock, flag);
	return flag;
#endif
}

void ipe_platform_spin_unlock(unsigned long flag)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&ipe_spin_lock, flag);
#endif
}

void ipe_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_IPE);
#endif
}

void ipe_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_IPE);
#endif
}

void ipe_platform_set_pinmux(UINT32 id, BOOL en)
{
#if defined __UITRON || defined __ECOS
	if (en)
		;
	else
		;
#else
#endif
}


int ipe_platform_is_err_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return 0;
#else
	return IS_ERR(ipe_clk[0]);
#endif
}

void ipe_platform_enable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(IPE_CLK);
#else
	clk_enable(ipe_clk[0]);
#endif
}

void ipe_platform_disable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(IPE_CLK);
#else
	clk_disable(ipe_clk[0]);
#endif
}

void ipe_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(IPE_RSTN);
	pll_disableSystemReset(IPE_RSTN);
#else
	clk_prepare(ipe_clk[0]);
#endif
}

void ipe_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	//pll_enableSystemReset(IPE_RSTN);
#else
	clk_unprepare(ipe_clk[0]);
#endif
}

void ipe_platform_set_clk_rate(UINT32 source_clk)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	static UINT32 ipe_true_clock    = PLL_CLKSEL_IPE_240;

#if 0
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk >= 280) {
			ipe_true_clock = PLL_CLKSEL_IPE_PLL13;
			ipe_clock = 280;
			if (source_clk > 280) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 280\r\n", (int)source_clk);
			}
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		} else if (source_clk >= 240) {
			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
			if (source_clk > 240) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
			}
		} else {
			//DBG_ERR("ipe_set_clock_rate() illegal input frequency %d\r\n", (int)source_clk);

			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
			DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
		}
	} else {
		if (source_clk >= 420) {
			ipe_true_clock = PLL_CLKSEL_IPE_PLL13;
			ipe_clock = 420;
			if (source_clk > 420) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 420\r\n", (int)source_clk);
			}
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		} else if (source_clk >= 320) {
			ipe_true_clock = PLL_CLKSEL_IPE_320;
			ipe_clock = 320;
			if (source_clk > 320) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 320\r\n", (int)source_clk);
			}

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk >= 240) {
			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
			if (source_clk > 240) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
			}
		} else {
			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
			DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
		} else {
			ipe_true_clock = PLL_CLKSEL_IPE_PLL13;
			ipe_clock = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			ipe_true_clock = PLL_CLKSEL_IPE_320;
			ipe_clock = 320;

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk == 240) {
			ipe_true_clock = PLL_CLKSEL_IPE_240;
			ipe_clock = 240;
		} else {
			ipe_true_clock = PLL_CLKSEL_IPE_PLL13;
			ipe_clock = 0;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	} else {
        ipe_true_clock = PLL_CLKSEL_IPE_240;
        ipe_clock = 240;

        DBG_ERR("IPE: get chip id %d out of range\r\n", nvt_get_chip_id());
    }
#endif

	pll_setClockRate(PLL_CLKSEL_IPE, ipe_true_clock);
#else
	struct clk *pclk = NULL;

#if 0
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk >= 280) {
			ipe_clock = 280;
			pclk = clk_get(NULL, "pll13");
			if (source_clk > 280) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 280\r\n", source_clk);
			}
		} else if (source_clk >= 240) {
			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");
			if (source_clk > 240) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", source_clk);
			}
		} else {
			//DBG_ERR("ipe_setClockRate() illegal input frequency %u\r\n", source_clk);

			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");
			DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", source_clk);
		}
	} else {
		if (source_clk >= 420) {
			ipe_clock = 420;
			pclk = clk_get(NULL, "pll13");
			if (source_clk > 420) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 420\r\n", source_clk);
			}
		} else if (source_clk >= 320) {
			ipe_clock = 320;
			pclk = clk_get(NULL, "fix320m");
			if (source_clk > 320) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 320\r\n", source_clk);
			}
		} else if (source_clk >= 240) {
			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");
			if (source_clk > 240) {
				DBG_WRN("ipe_set_clock_rate() input frequency %d round to 240\r\n", source_clk);
			}
		} else {
			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");
			DBG_WRN("ipe_setClockRate() illegal input frequency %u\r\n", source_clk);
		}
	}
#else
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");

		} else {
			ipe_clock = 0;
			pclk = clk_get(NULL, "pll13");
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		if (source_clk == 320) {
			ipe_clock = 320;
			pclk = clk_get(NULL, "fix320m");
		} else if (source_clk == 240) {
			ipe_clock = 240;
			pclk = clk_get(NULL, "fix240m");
		} else {
			ipe_clock = 0;
			pclk = clk_get(NULL, "pll13");
		}
	} else {
        ipe_clock = 240;
        pclk = clk_get(NULL, "fix240m");

        DBG_ERR("IPE: get chip id %d out of range\r\n", nvt_get_chip_id());
    }
#endif

	if (IS_ERR(pclk)) {
		printk("%s: get source fail\r\n", __func__);
	}

	clk_set_parent(ipe_clk[0], pclk);
	clk_put(pclk);
#endif
}

UINT32 ipe_platform_get_clk_rate(void)
{
	if (ipe_clock == 0) {
		DBG_WRN("IPE: clock source: PLL13, input frequency is defined by system\r\n");
	}

	return ipe_clock;
}

UINT32 ipe_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	return 0;
}

UINT32 ipe_platform_dma_flush_dev2mem_width_neq_lofs(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);

	return 0;
}

UINT32 ipe_platform_dma_flush_dev2mem_video_mode(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);

	return 0;
}

UINT32 ipe_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);

	return 0;
}


UINT32 ipe_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);

	return 0;
}


UINT32 ipe_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;

	rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

void ipe_platform_disable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_disable_sram_shutdown(IPE_SD);
#endif
#else
	nvt_disable_sram_shutdown(IPE_SD);
#endif
}

void ipe_platform_enable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_enable_sram_shutdown(IPE_SD);
#endif
#else
	nvt_enable_sram_shutdown(IPE_SD);
#endif
}

#if !(defined __UITRON || defined __ECOS)
static int ipe_is_create = 0;
#if defined __FREERTOS
irqreturn_t ipe_platform_isr(int irq, void *devid)
{
	ipe_isr();
	return IRQ_HANDLED;
}

void ipe_platform_create_resource(void)
{
	if (!ipe_is_create) {
		OS_CONFIG_FLAG(FLG_ID_IPE);
		SEM_CREATE(SEMID_IPE, 1);

		request_irq(INT_ID_IPE, ipe_platform_isr, IRQF_TRIGGER_HIGH, "ipe", 0);

		ipe_is_create = 1;

		ipeg = (NT98520_IPE_REGISTER_STRUCT *)((UINT32)_IPE_REG_BASE_ADDR);
		ipe_platform_set_init_status(TRUE);

		ipe_ctrl_flow_to_do = ENABLE;
	}
}
#else
void ipe_platform_create_resource(IPE_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(FLG_ID_IPE);
	SEM_CREATE(SEMID_IPE, 1);

	ipe_clk[0] = pmodule_info->pclk[0];
	ipe_pclk[0] = pmodule_info->p_pclk[0];
	_IPE_REG_BASE_ADDR = (UINT32)pmodule_info->io_addr[0];

	ipeg = (NT98520_IPE_REGISTER_STRUCT *)((UINT32)_IPE_REG_BASE_ADDR);
	ipe_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		ipe_ctrl_flow_to_do = ENABLE;
	} else {
		ipe_ctrl_flow_to_do = DISABLE;
	}
}
#endif
void ipe_platform_release_resource(void)
{
	ipe_platform_set_init_status(FALSE);
	ipe_is_create = 0;

	rel_flg(FLG_ID_IPE);
	SEM_DESTROY(SEMID_IPE);

	ipeg = NULL;
}
#endif

