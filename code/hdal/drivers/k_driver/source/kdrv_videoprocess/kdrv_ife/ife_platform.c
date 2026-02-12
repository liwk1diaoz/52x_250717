
#include "ife_platform.h"

static SEM_HANDLE SEMID_IFE;
static ID FLG_ID_IFE;
// NOTE: XXXXX
#if 0
UINT32 _IFE_REG_BASE_ADDR;
#endif

// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int ife_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
#else
#endif


#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
static VK_DEFINE_SPINLOCK(ife_spin_lock);
#else
static VK_DEFINE_SPINLOCK(ife_spin_lock);

struct clk *ife_clk[MODULE_CLK_NUM];
// NOTE: XXXXX
UINT32 _IFE_REG_BASE_ADDR;
#endif

static UINT32 g_uiIfeClock;

static BOOL g_ife_module_init_done = FALSE;

BOOL ife_ctrl_flow_to_do = DISABLE;

VOID ife_platform_set_init_status(BOOL set_status)
{
	g_ife_module_init_done = set_status;
}

BOOL ife_platform_get_init_status(VOID)
{
	return g_ife_module_init_done;
}

ER ife_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_IFE);
#else
	SEM_SIGNAL(SEMID_IFE);
	return E_OK;
#endif
}

ER ife_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_IFE);
#else
	return SEM_WAIT(SEMID_IFE);
#endif
}

ER ife_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(FLG_ID_IFE, flg);
}
ER ife_platform_flg_set(FLGPTN flg)
{
	return iset_flg(FLG_ID_IFE, flg);
}
ER ife_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, FLG_ID_IFE, flg, TWF_ORW | TWF_CLR);
}

void ife_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_IFE);
#endif
}

void ife_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_IFE);
#endif
}

void ife_platform_disable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_disable_sram_shutdown(IFE_SD);
#endif
#else
	nvt_disable_sram_shutdown(IFE_SD);
#endif
}
void ife_platform_enable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	nvt_enable_sram_shutdown(IFE_SD);
#endif
#else
	nvt_enable_sram_shutdown(IFE_SD);
#endif
}

UINT32 ife_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long ife_spin_flag;
	vk_spin_lock_irqsave(&ife_spin_lock, ife_spin_flag);
	return ife_spin_flag;
#endif
}
//---------------------------------------------------------------

void ife_platform_spin_unlock(UINT32 ife_spin_flag)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&ife_spin_lock, ife_spin_flag);
#endif
}

void ife_platform_enable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(IFE_CLK);
#else
	clk_enable(ife_clk[0]);
#endif
}

void ife_platform_disable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(IFE_CLK);
#else
	clk_disable(ife_clk[0]);
#endif
}

void ife_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	//pll_enableSystemReset(IFE_RSTN);
	pll_enableSystemReset(IFE_RSTN);
	pll_disableSystemReset(IFE_RSTN);
#else
	clk_prepare(ife_clk[0]);
#endif
}

void ife_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	//pll_enableSystemReset(IFE_RSTN);
#else
	clk_unprepare(ife_clk[0]);
#endif
}

ER ife_platform_set_clk_rate(UINT32 uiClock)
{
#ifdef __KERNEL__
	struct clk *source_clk = NULL;

#if 0
	/*
	    if (uiClock >= 360) {
	        if(uiClock > 360)
	            DBG_ERR("No such clock rate (set to 360Mhz)\r\n");
	        g_uiIfeClock = 360;
	        source_clk = clk_get(NULL, "pll13");
	    }  else if (uiClock >= 320) {
	        if(uiClock > 320)
	            DBG_ERR("No such clock rate (set to 320Mhz)\r\n");
	        g_uiIfeClock = 320;
	        source_clk = clk_get(NULL, "pllf320");
	    }*/
	if (uiClock >= 280) {
		if (uiClock > 280) {
			DBG_WRN("No such clock rate (set to 280Mhz)\r\n");
		}
		g_uiIfeClock = 280;
		source_clk = clk_get(NULL, "pll13");
	} else if (uiClock >= 240) {
		if (uiClock > 240) {
			DBG_WRN("No such clock rate (set to 240Mhz)\r\n");
		}
		g_uiIfeClock = 240;
		source_clk = clk_get(NULL, "fix240m");
	} else {
		//DBG_WRN("ife_setClockRate() illegal input frequency %u\r\n", (unsigned int)uiClock);
		//return E_SYS;

		g_uiIfeClock = 240;
		source_clk = clk_get(NULL, "fix240m");
	}

#else

	if (uiClock == 240) {
		g_uiIfeClock = 240;
		source_clk = clk_get(NULL, "fix240m");
	} else {
		g_uiIfeClock = 0;
		source_clk = clk_get(NULL, "pll13");
	}

#endif

	if (IS_ERR(source_clk)) {
		DBG_ERR("ife get clk source err\n");
		return E_SYS;
	}
	clk_set_parent(ife_clk[0], source_clk);
	clk_put(source_clk);
#else
	static UINT32 ife_true_clock = PLL_CLKSEL_IFE_240;

#if 0
	/*
	    if (uiClock >= 360) {
	        if(uiClock > 360)
	            DBG_ERR("No such clock rate (set to 360Mhz)\r\n");

	        g_uiIfeClock = 360;
	        ife_true_clock = PLL_CLKSEL_IFE_PLL13;

	        if (pll_getPLLEn(PLL_ID_13) == FALSE) {
	            pll_setPLLEn(PLL_ID_13, TRUE);
	        }

	        return E_PAR;
	    } else if (uiClock >= 320) {
	        if (uiClock > 320)
	            DBG_ERR("No such clock rate (set to 320Mhz)\r\n");

	        g_uiIfeClock = 320;

#if defined(_BSP_NA51055_)
	        ife_true_clock = PLL_CLKSEL_IFE_320;
	        if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
	            pll_setPLLEn(PLL_ID_FIXED320, TRUE);
	        }
#else
	        ife_true_clock = PLL_CLKSEL_IFE_PLL13;
	        if (pll_getPLLEn(PLL_ID_13) == FALSE) {
	            pll_setPLLEn(PLL_ID_13, TRUE);
	        }
#endif
	        return E_PAR;

	    }*/

	if (uiClock >= 280) {
		if (uiClock > 280) {
			DBG_ERR("No such clock rate (set to 280Mhz)\r\n");
		}

		g_uiIfeClock = 280;
		ife_true_clock = PLL_CLKSEL_IFE_PLL13;

		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
		//return E_PAR;
	} else if (uiClock >= 240) {
		if (uiClock > 240) {
			DBG_ERR("No such clock rate (set to 240Mhz)\r\n");
		}

		g_uiIfeClock = 240;
		ife_true_clock = PLL_CLKSEL_IFE_240;
		//return E_PAR;
	} else {
		//DBG_ERR("ife_setClockRate() illegal input frequency %u\r\n", (unsigned int)uiClock);
		//return E_SYS;

		g_uiIfeClock = 240;
		ife_true_clock = PLL_CLKSEL_IFE_240;
	}

#else

	if (uiClock == 240) {
		g_uiIfeClock = 240;
		ife_true_clock = PLL_CLKSEL_IFE_240;
	} else {
		g_uiIfeClock = 0;
		ife_true_clock = PLL_CLKSEL_IFE_PLL13;

		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
	}
#endif

	pll_setClockRate(PLL_CLKSEL_IFE, ife_true_clock);

#endif

	return E_OK;
}

UINT32 ife_platform_get_clk_rate(void)
{
	if (g_uiIfeClock == 0) {
		DBG_WRN("IFE: clock source: PLL13, input frequency is defined by system\r\n");
	}

	return g_uiIfeClock;
}

UINT32 ife_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size, UINT32 type)
{
	UINT32 ret = 0;
    if(type == 0){
    	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
    	ret = 0;
    } else{
    	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_FROM_DEVICE);
    	ret = 0;
    }
	return ret;
}

UINT32 ife_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size, UINT32 type)
{
	UINT32 ret = 0;
    if(type == 0){
    	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
    	ret = 0;
    } else{
    	vos_cpu_dcache_sync_vb(addr, size, VOS_DMA_TO_DEVICE);
    	ret = 0;
    }
	return ret;
}

UINT32 ife_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
	rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

#if !(defined __UITRON || defined __ECOS)
static int is_create = 0;
#if defined __FREERTOS
irqreturn_t ife_platform_isr(int irq, void *devid)
{
	ife_isr();
	return IRQ_HANDLED;
}

void ife_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_IFE);
		SEM_CREATE(SEMID_IFE, 1);

		request_irq(INT_ID_IFE, ife_platform_isr, IRQF_TRIGGER_HIGH, "ife", 0);

		is_create = 1;

		ife_platform_set_init_status(TRUE);
		ife_ctrl_flow_to_do = ENABLE;
	}
}
#else
void ife_platform_create_resource(IFE_MODULE_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(FLG_ID_IFE);
	SEM_CREATE(SEMID_IFE, 1);

	ife_clk[0] = pmodule_info->pclk[0];
	_IFE_REG_BASE_ADDR = (UINT32)pmodule_info->io_addr[0];

	ife_platform_set_init_status(TRUE);

	if (kdrv_builtin_is_fastboot() == DISABLE) {
		ife_ctrl_flow_to_do = ENABLE;
	} else {
		ife_ctrl_flow_to_do = DISABLE;
	}
}
#endif
void ife_platform_release_resource(void)
{
	ife_platform_set_init_status(FALSE);
	is_create = 0;

	rel_flg(FLG_ID_IFE);
	SEM_DESTROY(SEMID_IFE);
}
#endif
