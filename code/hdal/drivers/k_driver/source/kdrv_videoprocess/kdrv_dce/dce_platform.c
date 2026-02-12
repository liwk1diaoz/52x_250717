#include "dce_platform.h"

// semaphore & interrupt flag id
static SEM_HANDLE SEMID_DCE;
static ID FLG_ID_DCE;

static UINT32 dce_clock;

#if !defined (CONFIG_NVT_SMALL_HDAL)
char g_kdrv_dce_ctrl_api_log[KDRV_DCE_LOG_NUM][KDRV_DCE_LOG_MSG_LENGTH] = {""};
int log_idx = 0;
unsigned int total_log_idx = 0;
#endif

// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int dce_debug_level = NVT_DBG_WRN;
#else
#endif

#if defined __UITRON || defined __ECOS

#elif defined(__FREERTOS)
static VK_DEFINE_SPINLOCK(dce_spin_lock);
#else
static VK_DEFINE_SPINLOCK(dce_spin_lock);
//clock
static struct clk *dce_clk[MODULE_CLK_NUM];
//base address
UINT32 _DCE_REG_BASE_ADDR;
#endif


static BOOL g_dce_module_init_done = FALSE;
BOOL dce_ctrl_flow_to_do = DISABLE;


void dce_platform_set_init_status(BOOL set_status)
{
	g_dce_module_init_done = set_status;
}

BOOL dce_platform_get_init_status(void)
{
	return g_dce_module_init_done;
}


ER dce_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(FLG_ID_DCE, flg);
}

ER dce_platform_flg_set(FLGPTN flg)
{
	return iset_flg(FLG_ID_DCE, flg);
}

ER dce_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, FLG_ID_DCE, flg, TWF_CLR | TWF_ORW);
}

ER dce_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_DCE);
#else
	return SEM_WAIT(SEMID_DCE);
#endif
}

ER dce_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_DCE);
#else
	SEM_SIGNAL(SEMID_DCE);
	return E_OK;
#endif
}

unsigned long dce_platform_spin_lock(void)
{
	unsigned long flag;
	vk_spin_lock_irqsave(&dce_spin_lock, flag);
	return flag;
}

void dce_platform_spin_unlock(unsigned long flag)
{
	vk_spin_unlock_irqrestore(&dce_spin_lock, flag);
}

void dce_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_DCE);
#endif
}

void dce_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_DCE);
#endif
}

void dce_platform_set_pinmux(UINT32 id, BOOL en)
{
#if defined __UITRON || defined __ECOS
	if (en)
		;
	else
		;
#else
#endif
}

int dce_platform_is_err_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return 0;
#else
	return IS_ERR(dce_clk[0]);
#endif
}

void dce_platform_enable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(DCE_CLK);
#else
	clk_enable(dce_clk[0]);
#endif
}

void dce_platform_disable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(DCE_CLK);
#else
	clk_disable(dce_clk[0]);
#endif
}

void dce_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(DCE_RSTN);
	pll_disableSystemReset(DCE_RSTN);
#else
	clk_prepare(dce_clk[0]);
#endif
}

void dce_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	//pll_enableSystemReset(DCE_RSTN);
#else
	clk_unprepare(dce_clk[0]);
#endif
}

void dce_platform_set_clk_rate(UINT32 source_clk)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	static UINT32 dce_true_clock    = PLL_CLKSEL_DCE_240;

#if 0
	if (source_clk >= 280) {
		dce_true_clock = PLL_CLKSEL_DCE_PLL13;
		dce_clock = 280;
		if (source_clk > 280) {
			DBG_WRN("dce_set_clock_rate() input frequency %d round to 280\r\n", (int)source_clk);
		}
		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
	} /*else if (source_clk >= 320) {
#if defined(_BSP_NA51055_)
        dce_true_clock = PLL_CLKSEL_DCE_320;
#else
        // TODO: temporal
        dce_true_clock = PLL_CLKSEL_DCE_PLL13;
#endif
        dce_clock = 320;
        if (source_clk > 320) {
            DBG_ERR("dce_set_clock_rate() input frequency %d round to 320\r\n", (int)source_clk);
        }
#if defined(_BSP_NA51055_)
        if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
            pll_setPLLEn(PLL_ID_FIXED320, TRUE);
        }
#else
        // TODO: temporal
        if (pll_getPLLEn(PLL_CLKSEL_DCE_PLL13) == FALSE) {
            pll_setPLLEn(PLL_CLKSEL_DCE_PLL13, TRUE);
        }
#endif
    } */else if (source_clk >= 240) {
		dce_true_clock = PLL_CLKSEL_DCE_240;
		dce_clock = 240;
		if (source_clk > 240) {
			DBG_WRN("dce_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
		}
	} else {
		//DBG_ERR("dce_set_clock_rate() illegal input frequency %d\r\n", (int)source_clk);

		dce_true_clock = PLL_CLKSEL_DCE_240;
		dce_clock = 240;
		DBG_WRN("dce_set_clock_rate() input frequency %d round to 240\r\n", (int)source_clk);
	}

#else
	if (source_clk == 240) {
		dce_true_clock = PLL_CLKSEL_DCE_240;
		dce_clock = 240;
	} else {
		dce_true_clock = PLL_CLKSEL_DCE_PLL13;
		dce_clock = 0;

		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
	}
#endif

	pll_setClockRate(PLL_CLKSEL_DCE, dce_true_clock);

#else
	struct clk *pclk = NULL;

#if 0
	if (source_clk >= 280) {
		dce_clock = 280;
		pclk = clk_get(NULL, "pll13");
		if (source_clk > 280) {
			DBG_WRN("dce_set_clock_rate() input frequency %d round to 280\r\n", source_clk);
		}
	} /*else if (source_clk >= 320) {
        dce_clock = 320;
        pclk = clk_get(NULL, "fix320m");
        if (source_clk > 320) {
            DBG_ERR("dce_set_clock_rate() input frequency %d round to 320\r\n", source_clk);
        }
    } */else if (source_clk >= 240) {
		dce_clock = 240;
		pclk = clk_get(NULL, "fix240m");
		if (source_clk > 240) {
			DBG_WRN("dce_set_clock_rate() input frequency %d round to 240\r\n", source_clk);
		}
	} else {
		//DBG_ERR("dce_setClockRate() illegal input frequency %u\r\n", source_clk);

		dce_clock = 240;
		pclk = clk_get(NULL, "fix240m");
		DBG_WRN("dce_set_clock_rate() input frequency %d round to 240\r\n", source_clk);
	}
#else
	if (source_clk == 240) {
		dce_clock = 240;
		pclk = clk_get(NULL, "fix240m");

	} else {
		dce_clock = 0;
		pclk = clk_get(NULL, "pll13");
	}
#endif

	if (IS_ERR(pclk)) {
		printk("%s: get source fix480m fail\r\n", __func__);
	}

	clk_set_parent(dce_clk[0], pclk);
	clk_put(pclk);

#endif
}

UINT32 dce_platform_get_clk_rate(void)
{
	if (dce_clock == 0) {
		DBG_WRN("DCE: clock source: PLL13, input frequency is defined by system\r\n");
	}

	return dce_clock;
}

#if 0
UINT32 dce_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);

	return 0;
}

UINT32 dce_platform_dma_flush_dev2mem_width_neq_lofs(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL);

	return 0;
}

UINT32 dce_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync_vb((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);

	return 0;
}

UINT32 dce_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE);

	return 0;
}
#endif

UINT32 dce_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;

	rt = vos_cpu_get_phy_addr((VOS_ADDR)addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

void dce_platform_disable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_disable_sram_shutdown(DCE_SD);
#endif
#else
	nvt_disable_sram_shutdown(DCE_SD);
#endif
}

void dce_platform_enable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_enable_sram_shutdown(DCE_SD);
#endif
#else
	nvt_enable_sram_shutdown(DCE_SD);
#endif
}

void cnn2_platform_disable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_disable_sram_shutdown(CNN2_SD);
#endif
#else
	nvt_disable_sram_shutdown(CNN2_SD);
#endif
}

#if !(defined __UITRON || defined __ECOS)
static int is_create = 0;

#if defined __FREERTOS
irqreturn_t dce_platform_isr(int irq, void *devid)
{
	dce_isr();
	return IRQ_HANDLED;
}

void dce_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_DCE);
		SEM_CREATE(SEMID_DCE, 1);

		request_irq(INT_ID_DCE, dce_platform_isr, IRQF_TRIGGER_HIGH, "dce", 0);

		is_create = 1;

		dce_platform_set_init_status(TRUE);

		dce_ctrl_flow_to_do = ENABLE;
	}
}
#else
void dce_platform_create_resource(DCE_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(FLG_ID_DCE);
	SEM_CREATE(SEMID_DCE, 1);

	dce_clk[0] = pmodule_info->pclk[0];
	_DCE_REG_BASE_ADDR = (UINT32)pmodule_info->io_addr[0];

	dce_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		dce_ctrl_flow_to_do = ENABLE;
	} else {
		dce_ctrl_flow_to_do = DISABLE;
	}
}
#endif
void dce_platform_release_resource(void)
{
	dce_platform_set_init_status(FALSE);
	is_create = 0;

	rel_flg(FLG_ID_DCE);
	SEM_DESTROY(SEMID_DCE);
}
#endif

