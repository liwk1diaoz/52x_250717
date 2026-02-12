/*
    SDE module driver

    NT96520 SDE module driver.

    @file       sde_platform.c
    @ingroup    mIIPPISE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "sde_platform.h"

#include "plat/top.h"

static SEM_HANDLE SEMID_SDE;
static ID     FLG_ID_SDE;
static UINT32 sde_eng_clock_rate = 0;
ID     g_SdeLockStatus     = NO_TASK_LOCKED;

UINT32 _sde_reg_io_base[2];

static BOOL     g_sde_platform_clk_en = FALSE;



//static BOOL g_sde_set_clk_en = FALSE;


// debug level
#if defined __UITRON || defined __ECOS

#elif defined(__FREERTOS)

unsigned int sde_debug_level = NVT_DBG_IND;

#else
//linux


#endif


//=========================================================================
#if defined (__KERNEL__)

static VK_DEFINE_SPINLOCK(sde_spin_lock);
static unsigned long sde_spin_flags;

struct clk *sde_set_clk[SDE_CLK_NUM];

UINT32  _SDE_REG_BASE_ADDR;


//=========================================================================
#elif defined (__FREERTOS)

static VK_DEFINE_SPINLOCK(sde_spin_lock);
static unsigned long sde_spin_flags;

//=========================================================================
#elif defined (__UITRON) || defined(__ECOS)

#else

#endif



#if !(defined __UITRON || defined __ECOS)
static INT32 is_sde_create = 0;

#if defined (__FREERTOS)
//FREERTOS
irqreturn_t sde_platform_isr(int irq, VOID *devid)
{
	sde_isr();
	return IRQ_HANDLED;
}

VOID sde_platform_create_resource(VOID)
{
	if (!is_sde_create) {
		OS_CONFIG_FLAG(FLG_ID_SDE);
		SEM_CREATE(SEMID_SDE, 1);
		request_irq(INT_ID_SDE, sde_platform_isr, IRQF_TRIGGER_HIGH, "sde_int", 0);
		is_sde_create = 1;
		_sde_reg_io_base[0] = (UINT32)_SDE_REG_BASE_ADDR;
	}
}

#else
//linux
VOID sde_platform_create_resource(SDE_INFO *pmodule_info)
{
	OS_CONFIG_FLAG(FLG_ID_SDE);
	SEM_CREATE(SEMID_SDE, 1);

	sde_set_clk[0] = pmodule_info->pclk[0];
	_sde_reg_io_base[0] = (UINT32)pmodule_info->io_addr[0];
}
#endif


VOID sde_platform_release_resource(VOID)
{
	rel_flg(FLG_ID_SDE);
	SEM_DESTROY(SEMID_SDE);

	is_sde_create = 0;
}
#endif




ER sde_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(FLG_ID_SDE, flg);
}
//---------------------------------------------------------------

ER sde_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	return wai_flg(p_flgptn, FLG_ID_SDE, flg, TWF_CLR | TWF_ORW);
}
//---------------------------------------------------------------

ER sde_platform_flg_set(FLGPTN flg)
{
	return iset_flg(FLG_ID_SDE, flg);
}
//---------------------------------------------------------------

VOID sde_platform_int_enable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_SDE);
#endif
}


VOID sde_platform_int_disable(VOID)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_SDE);
#endif
}



VOID sde_platform_disable_sram_shutdown(VOID)
{

	if (nvt_get_chip_id() != CHIP_NA51055) {
		nvt_disable_sram_shutdown(SDE_SD);
	}

#if 0
#if defined (__KERNEL__)
	//SDE does not exist in NT98520.

	//nvt_disable_sram_shutdown(SDE_SD);
#if defined (_BSP_NA51055_)
	nvt_disable_sram_shutdown(SDE_SD);

	//*(volatile UINT32 *)(0xF0011004) &= 0xFFFFFFFD; 

#endif
#elif defined (__FREERTOS)

#if defined (_BSP_NA51055_)
	nvt_disable_sram_shutdown(SDE_SD);

	//*(volatile UINT32 *)(0xF0011004) &= 0xFFFFFFFD; 

#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_disableSramShutDown(SDE_RSTN);
#else
#endif

#endif



}
//---------------------------------------------------------------

VOID sde_platform_enable_sram_shutdown(VOID)
{
	if (nvt_get_chip_id() != CHIP_NA51055) {
		nvt_enable_sram_shutdown(SDE_SD);
	}
#if 0
#if defined (__KERNEL__)
	//SDE does not exist in NT98520.

	//nvt_enable_sram_shutdown(SDE_SD);

#elif defined (__FREERTOS)

#if defined (_BSP_NA51055_)
	//*(volatile UINT32 *)(0xF0011004) &= 0xFFFFFFFF; 
	nvt_enable_sram_shutdown(SDE_SD);
#endif

#elif defined (__UITRON) || defined (__ECOS)
	pinmux_enableSramShutDown(SDE_RSTN);
#else
#endif
#endif

}


VOID sde_platform_enable_clk(VOID)
{
	if (g_sde_platform_clk_en == FALSE) {
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_enableClock(SDE_CLK);
		//*(volatile UINT32 *)(0xf0020070) |= 0x00100000; //alex, clock enable.
#else
		//linux
		if (IS_ERR(sde_set_clk[0])) {
			DBG_WRN("SDE: get clk fail 0x%p\r\n", sde_set_clk[0]);
		} else {
			clk_enable(sde_set_clk[0]);
		}
#endif
		g_sde_platform_clk_en = TRUE;
	}
}
//---------------------------------------------------------------

VOID sde_platform_disable_clk(VOID)
{
	if (g_sde_platform_clk_en == TRUE) {
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		pll_disableClock(SDE_CLK);
		//*(volatile UINT32 *)(0xf0020070) &= 0xFFEFFFFF; //alex, clock disable.
#else
		//LINUX
		if (IS_ERR(sde_set_clk[0])) {
			DBG_WRN("SDE: get clk fail 0x%p\r\n", sde_set_clk[0]);
		} else {
			clk_disable(sde_set_clk[0]);
		}
#endif
		g_sde_platform_clk_en = FALSE;
	}

}

//---------------------------------------------------------------


UINT32 sde_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}


UINT32 sde_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)
	fmem_dcache_sync_vb((void *)addr, size, DMA_BIDIRECTIONAL);

	ret = 0;
#elif defined (__FREERTOS)
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------


UINT32 sde_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	UINT32 ret;

	#if defined (__UITRON) || defined (__ECOS)
		ret = dma_flushWriteCacheWithoutCheck(addr, size);
		wmb();
		ret = 0;
	#else
		vos_cpu_dcache_sync(addr, size, /*VOS_DMA_BIDIRECTIONAL */VOS_DMA_TO_DEVICE);
		ret = 0;
	#endif

#if 0
#if defined (__KERNEL__)
	fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);

	ret = 0;

#elif defined (__FREERTOS)
	vos_cpu_dcache_sync(addr, size, /*VOS_DMA_BIDIRECTIONAL */VOS_DMA_TO_DEVICE);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)

	ret = dma_flushWriteCacheWithoutCheck(addr, size);
	wmb();
#else
	ret = 0;
#endif


#endif
	return ret;
}
//---------------------------------------------------------------


UINT32 sde_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
	UINT32 ret;
#if defined (__UITRON) || defined (__ECOS)

#else 
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;
#endif

#if 0
#if defined (__KERNEL__)

	fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	ret = 0;

#elif defined (__FREERTOS)
	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	ret = (UINT32)dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
#else
	ret = 0;
#endif
#endif

	return ret;
}
//---------------------------------------------------------------

UINT32 sde_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__) || defined (__FREERTOS)

	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	//ret = 0;

	vos_cpu_dcache_sync(addr, size, /*VOS_DMA_BIDIRECTIONAL*/ VOS_DMA_FROM_DEVICE);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	ret = (UINT32)dma_flushReadCacheWithoutCheck(addr, size);
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------

UINT32 sde_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__)

	//fmem_dcache_sync_vb((void *)addr, size, DMA_BIDIRECTIONAL);
	ret = 0;

#elif defined (__FREERTOS)
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	ret = 0;
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------


UINT32 sde_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
	UINT32 ret;

#if defined (__KERNEL__) || defined (__FREERTOS)

	//fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
	//ret = 0;

	vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
	ret = 0;
#elif defined (__UITRON) || defined (__ECOS)
	ret = (UINT32)dma_flushReadCacheWithoutCheck(addr, size);
#else
	ret = 0;
#endif

	return ret;
}
//---------------------------------------------------------------

UINT32 sde_platform_va2pa(UINT32 addr)
{
#if defined (__KERNEL__) || defined (__FREERTOS)
	//return fmem_lookup_pa(addr);

	return vos_cpu_get_phy_addr(addr);
#elif defined (__UITRON) || defined (__ECOS)
	return dma_getPhyAddr(addr);
#else
	return addr;
#endif
}


ER sde_platform_set_clk_rate(UINT32 source_clk)
{
	if (source_clk != sde_eng_clock_rate) {

#if defined (__KERNEL__)

		struct clk *parent_clk = NULL;

		if (IS_ERR(sde_set_clk[0])) {
			DBG_ERR("SDE: get clk fail...0x%p\r\n", sde_set_clk[0]);
			return E_SYS;
		}

		if (source_clk > 480) {
			parent_clk = clk_get(NULL, "fix480m");
			sde_eng_clock_rate = 480;
			DBG_ERR("No such clock rate, too big (set to 480Mhz)\r\n");
		} else if (source_clk == 480) {
			parent_clk = clk_get(NULL, "fix480m");
			sde_eng_clock_rate = 480;
		} else if (source_clk > 420) {
			parent_clk = clk_get(NULL, "pll13");
			sde_eng_clock_rate = 420;
			DBG_ERR("No such clock rate (set to 420Mhz)\r\n");

		} else if (source_clk == 420) {
			parent_clk = clk_get(NULL, "pll13");
			sde_eng_clock_rate = 420;
		} else if (source_clk > 360) {
			parent_clk = clk_get(NULL, "pll6");
			sde_eng_clock_rate = 360;
			DBG_ERR("No such clock rate (set to 360Mhz)\r\n");

		} else if (source_clk == 360) {
			parent_clk = clk_get(NULL, "pll6");
			sde_eng_clock_rate = 360;
		} else if (source_clk > 240) {
			parent_clk = clk_get(NULL, "fix240m");
			sde_eng_clock_rate = 240;
			DBG_ERR("No such clock rate (set to 240Mhz)\r\n");
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			sde_eng_clock_rate = 240;
		} else {
			parent_clk = clk_get(NULL, "fix240m");
			sde_eng_clock_rate = 240;
			DBG_ERR("Clock rate is too small (set to 240Mhz)\r\n");
		}

		if (IS_ERR(parent_clk)) {
			nvt_dbg(ERR, "ide get clk source err\n");
			return E_SYS;
		}

		clk_set_parent(sde_clk[0], parent_clk);
		clk_put(parent_clk);

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS )
		UINT32 uiSelectedClock;
         
        
//PLL_CLKSEL_SDE_240        (0x00 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as 240MHz
//PLL_CLKSEL_SDE_320        (0x01 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as 320MHz
//PLL_CLKSEL_SDE_480        (0x02 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as 480MHz
//PLL_CLKSEL_SDE_PLL13      (0x03 << (PLL_CLKSEL_SDE - PLL_CLKSEL_R1_OFFSET)) //< Select SDE clock as PLL13


		if (source_clk > 480) {
		    uiSelectedClock = PLL_CLKSEL_SDE_480;//< Select SDE clock as 480MHz, 2
		    sde_eng_clock_rate = 480; 
		    DBG_ERR("No such clock rate, too big (set to 480Mhz)\r\n");
		} else if (source_clk == 480) {
		    uiSelectedClock = PLL_CLKSEL_SDE_480;
		    sde_eng_clock_rate = 480;
		} else if (source_clk > 420) {
		    uiSelectedClock = PLL_CLKSEL_SDE_PLL13;//< Select SDE clock as PLL13
		    sde_eng_clock_rate = 420;
		    DBG_ERR("No such clock rate (set to 420Mhz)\r\n");
		} else if (source_clk == 420) {
		    uiSelectedClock = PLL_CLKSEL_SDE_PLL13;
		    sde_eng_clock_rate = 420;
		} else if (source_clk > 320) {
		    uiSelectedClock = PLL_CLKSEL_SDE_320;
		    sde_eng_clock_rate = 320;
		    DBG_ERR("No such clock rate (set to 320Mhz)\r\n");
		} else if (source_clk == 320) {
		    uiSelectedClock = PLL_CLKSEL_SDE_320;
		    sde_eng_clock_rate = 320;
		} else if (source_clk > 240) {
		    uiSelectedClock = PLL_CLKSEL_SDE_240;//< Select SDE clock as 240MHz
		    sde_eng_clock_rate = 240;
		    DBG_ERR("No such clock rate (set to 240Mhz)\r\n");
		} else if (source_clk == 240) {
		    uiSelectedClock = PLL_CLKSEL_SDE_240;//< Select SDE clock as 240MHz
		    sde_eng_clock_rate = 240;
		} else {
		    uiSelectedClock = PLL_CLKSEL_SDE_240;//< Select SDE clock as 240MHz
		    sde_eng_clock_rate = 240;
		    DBG_ERR("Clock rate is too small (set to 240Mhz)\r\n");
		}
		
		//alex  Note : change after sa driver ready.
		DBG_ERR("driver set Clock rate %d\r\n", uiSelectedClock);
		pll_setClockRate(PLL_CLKSEL_SDE, uiSelectedClock);
		
		//pll_set_clock_rate
#endif

		sde_eng_clock_rate = source_clk;

	}
	return E_OK;
}

UINT32 sde_platform_get_clk_rate(VOID)
{
	return sde_eng_clock_rate;
}

ER sde_platform_sem_wait(VOID)
{
	g_SdeLockStatus = TASK_LOCKED;
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_SDE);
#else
	return SEM_WAIT(SEMID_SDE);
#endif
}
//---------------------------------------------------------------

ER sde_platform_sem_signal(VOID)
{
	g_SdeLockStatus = NO_TASK_LOCKED;
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_SDE);
#else
	SEM_SIGNAL(SEMID_SDE);
	return E_OK;
#endif
}


VOID sde_platform_spin_lock(VOID)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#elif defined(__FREERTOS)
	vk_spin_lock_irqsave(&sde_spin_lock, sde_spin_flags);
#else
	vk_spin_lock_irqsave(&sde_spin_lock, sde_spin_flags);
#endif
}
//---------------------------------------------------------------

VOID sde_platform_spin_unlock(VOID)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&sde_spin_lock, sde_spin_flags);
#else
	vk_spin_unlock_irqrestore(&sde_spin_lock, sde_spin_flags);
#endif
}














