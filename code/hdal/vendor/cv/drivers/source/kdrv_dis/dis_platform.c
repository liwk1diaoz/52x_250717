#include "dis_platform.h"

#if defined(__LINUX)
#include <linux/clk.h>
#endif


#define DIS_MODULE_CLK_NUM 1

#if defined(__FREERTOS)
ID 		  FLG_ID_DIS;
#else
FLGPTN     FLG_ID_DIS;
#endif

static UINT32 dis_freq_mhz=0;
#if defined(__FREERTOS)
#else
extern struct clk* dis_clk[DIS_MODULE_CLK_NUM];
#endif

#if defined(__FREERTOS)
static UINT32 g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
#else
#endif

VOID dis_platform_request_irq(VOID *isr_handler)
{
#if defined(__FREERTOS)
	irq_handler_t isr_func = (irq_handler_t) isr_handler;
    request_irq(INT_ID_DIS, isr_func, IRQF_TRIGGER_HIGH, "dis", 0);
#endif
	return;
}

void dis_platform_clk_enable(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(DIS_CLK);
#else
	clk_prepare(dis_clk[0]);
	clk_enable(dis_clk[0]);
#endif
}

void dis_platform_clk_disable(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(DIS_CLK);
#else
	clk_disable(dis_clk[0]);
	clk_unprepare(dis_clk[0]);
#endif
}

void dis_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(DIS_RSTN);
	pll_disableSystemReset(DIS_RSTN);
#else
	clk_prepare(dis_clk[0]);
#endif
}

void dis_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
    //pll_enableSystemReset(DIS_RSTN);
#else
	clk_unprepare(dis_clk[0]);
#endif
}

/**
	DIS Create Resource
*/
VOID dis_platform_create_resource(UINT32 clk_freq)
{
	dis_freq_mhz = clk_freq / DIS_1M_HZ;
}

VOID dis_platform_flg_sel(VOID)
{	
	rel_flg(FLG_ID_DIS);
}

ER dis_platform_flg_clear(UINT32 flag_id)
{
	return clr_flg(FLG_ID_DIS, flag_id);
}

void dis_platform_flg_set(UINT32 flag_id)
{
	iset_flg(FLG_ID_DIS, flag_id);
}

void dis_platform_flg_wait(FLGPTN *uiFlag, UINT32 flag_id)
{
	wai_flg(uiFlag, FLG_ID_DIS, flag_id, TWF_CLR | TWF_ORW);
}

UINT32 dis_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;

}

UINT32 dis_platform_dma_flush(UINT32 addr, UINT32 size)
{
	//nvt_dbg(IND, "cache flush: addr(0x%x) size(0x%x)\r\n", addr, size);
    vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
    return 0;
}

UINT32 dis_platform_dma_flush_mem2device(UINT32 addr, UINT32 size)
{
	//nvt_dbg(IND, "cache flush: addr(0x%x) size(0x%x)\r\n", addr, size);
    vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE);
    return 0;
}

/**
    DIS get operation clock

    Get DIS clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
UINT32 dis_platform_get_clock_rate(VOID)
{
	return dis_freq_mhz;
}

/**
    DIS set operation clock

    Set DIS clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_platform_set_clock_rate(UINT32 uiClock)
{
#if defined __UITRON || defined __ECOS
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);

#endif

#else
#if 0
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);

#else
	struct clk *pclk;

	if(uiClock >= 480) {
		pclk = clk_get(NULL, "fix480m");
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		pclk = clk_get(NULL, "pll13");
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		pclk = clk_get(NULL, "pllf320");
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		pclk = clk_get(NULL, "fix240m");
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		pclk = clk_get(NULL, "fix240m");
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}

	if (IS_ERR(pclk)) {
		printk("%s: get source clock fail\r\n", __func__);
	}

	clk_set_parent(dis_clk[0], pclk);
	clk_put(pclk);
#endif

#endif //#if defined(__FREERTOS)

	return E_OK;
}


//@}

UINT32 dis_platform_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev)
{
    UINT32 va = 0;

#if defined(__FREERTOS)
    va = pa;
#else

    if (sz == 0) {
        return va;
    }
    if (pfn_valid(__phys_to_pfn(pa))) {
        va = (UINT32)__va(pa);
    } else {
        va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
	}
	//fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
	if (is_mem2dev == 1) {
    	vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	} else if (is_mem2dev == 0) {
		vos_cpu_dcache_sync(va, sz, VOS_DMA_FROM_DEVICE); ///< cache clean - output from engine
	} else {
		//Do not cache sync.
	}
#endif
    return va;
} 

VOID dis_platform_pa2va_unmap(UINT32 va, UINT32 pa)
{
#if defined(__FREERTOS)
	return;
#else
    if (va == 0) {
        return;
    }

    if (!pfn_valid(__phys_to_pfn(pa))) {
        iounmap((VOID *)va);
    }
#endif
}