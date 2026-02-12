#include "remote_int.h"


UINT64 guiRemoteCycleTime = RM_CYCLE_TIME_RTC;
UINT32 guiRemoteStatus = REMOTE_STATUS_CLOSED;

#if defined __UITRON || defined __ECOS
#else
UINT32 _REGIOBASE;

static SEM_HANDLE remote_sem_id;
static vk_spinlock_t remote_lock;
volatile DRV_CB pf_ist_cbs_remote;

#if defined __LINUX
static struct clk* remote_clk;
static BOOL remote_clk_en;

#elif defined __FREERTOS
unsigned int remote_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
#endif
#endif


#if defined __UITRON || defined __ECOS
#else
#if defined __LINUX
#elif defined __FREERTOS

static irqreturn_t remote_platform_isr(int irq, void *devid)
{
	remote_isr();
	return IRQ_HANDLED;
}

#endif
#endif


ER remote_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_REMOTE);
#else
	return SEM_WAIT(remote_sem_id);
#endif
}

ER remote_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_REMOTE);
#else
	SEM_SIGNAL(remote_sem_id);
	return E_OK;
#endif
}

UINT32 remote_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long flags;
	vk_spin_lock_irqsave(&remote_lock, flags);
	return flags;
#endif
}

void remote_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&remote_lock, flags);
#endif
}

void remote_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_REMOTE);
#else
#endif
}

void remote_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_REMOTE);
#else
#endif
}

void remote_platform_set_ist_event(UINT32 events)
{
#if defined __UITRON || defined __ECOS
	drv_setIstEvent(DRV_IST_MODULE_REMOTE, events);
#else
#endif
}

ER remote_platform_create_resource(void *pmodule_info)
{
#if defined __UITRON || defined __ECOS
#else
#if defined __LINUX
	REMOTE_MODULE_INFO *_pmodule_info = (REMOTE_MODULE_INFO *)pmodule_info;
#elif defined __FREERTOS
#endif
#endif

	DBG_IND("\n");

	SEM_CREATE(remote_sem_id, 1);
	//coverity[side_effect_free]
	vk_spin_lock_init(&remote_lock);

#if defined __UITRON || defined __ECOS
#else
#if defined __LINUX
	_REGIOBASE = (UINT32)_pmodule_info->io_addr[0];
	if (!IS_ERR(_pmodule_info->pclk)) {
		remote_clk = _pmodule_info->pclk[0];
		remote_clk_en = DISABLE;
		clk_prepare(remote_clk);
	} else {
		DBG_ERR("remote clock error\r\n");
		remote_clk = NULL;
		return E_SYS;
	}

#elif defined __FREERTOS
	_REGIOBASE = IOADDR_REMOTE_REG_BASE;
	request_irq(INT_ID_REMOTE, remote_platform_isr ,IRQF_TRIGGER_HIGH, "remote", 0);
#endif
#endif

	return E_OK;
}

void remote_platform_release_resource(void)
{
	DBG_IND("\n");

	SEM_DESTROY(remote_sem_id);

#if defined __UITRON || defined __ECOS
#else
#if defined __LINUX
	clk_unprepare(remote_clk);
	remote_clk = NULL;

#elif defined __FREERTOS
	free_irq(INT_ID_REMOTE, 0);
#endif
#endif
}

void remote_platform_clock_enable(void)
{
	DBG_IND("\n");

#if defined __UITRON || defined __ECOS
	pll_enableClock(REMOTE_CLK);

#else
#if defined __LINUX
	if (!remote_clk_en) {
		if (remote_clk) {
			clk_enable(remote_clk);
			remote_clk_en = ENABLE;
		}
	} else {
		DBG_WRN("remote clock already enable\r\n");
	}

#elif defined __FREERTOS
	pll_enable_clock(REMOTE_CLK);
#endif
#endif
}

void remote_platform_clock_disable(void)
{
	DBG_IND("\n");

#if defined __UITRON || defined __ECOS
	pll_disableClock(REMOTE_CLK);

#else
#if defined __LINUX
	if (remote_clk_en) {
		if (remote_clk) {
			clk_disable(remote_clk);
			remote_clk_en = DISABLE;
		}
	} else {
		DBG_WRN("remote clock must be enable before disable\r\n");
	}

#elif defined __FREERTOS
	pll_disable_clock(REMOTE_CLK);
#endif
#endif
}

ER remote_platform_select_clock_source(REMOTE_CLK_SRC_SEL src_type)
{
#if defined(_BSP_NA51055_)
	ER rt = E_OK;
#if defined __UITRON || defined __ECOS
#else
#if defined __LINUX
	struct clk *src_clk = NULL;
#elif defined __FREERTOS
#endif
#endif

	DBG_IND("type %s\n", (src_type == REMOTE_CLK_SRC_RTC) ? "RTC" : "OSC");

#if defined __UITRON || defined __ECOS
	switch (src_type) {
	case REMOTE_CLK_SRC_RTC:
		pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_RTC);
		break;
	case REMOTE_CLK_SRC_OSC:
		pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_OSC);
		break;
	case REMOTE_CLK_SRC_EXT:
		//pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_EXT);
		break;
	case REMOTE_CLK_SRC_3M:
		//pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_3M);
		break;
	default:
		DBG_ERR("error type %d\n", (int)(src_type));
		rt = E_PAR;
		goto err;
	}

#else
#if defined __LINUX
	if (remote_clk_en) {
		DBG_ERR("remote clock must be stopped before change clock source!\r\n");
		rt = E_SYS;
		goto err;
	}

	switch (src_type) {
	case REMOTE_CLK_SRC_RTC:
		src_clk = clk_get(NULL, "fix16M");
		break;
	case REMOTE_CLK_SRC_OSC:
		src_clk = clk_get(NULL, "osc_in");
		break;
	case REMOTE_CLK_SRC_EXT:
		//src_clk = clk_get(NULL, "fix24m");
		break;
	case REMOTE_CLK_SRC_3M:
		//src_clk = clk_get(NULL, "fix3m");
		break;
	default:
		DBG_ERR("error type %d\n", (int)(src_type));
		rt = E_PAR;
		goto err;
	}

	clk_set_parent(remote_clk, src_clk);
	clk_put(src_clk);

#elif defined __FREERTOS
	switch (src_type) {
	case REMOTE_CLK_SRC_RTC:
		pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_RTC);
		break;
	case REMOTE_CLK_SRC_OSC:
		pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_OSC);
		break;
	case REMOTE_CLK_SRC_EXT:
		//pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_EXT);
		break;
	case REMOTE_CLK_SRC_3M:
		//pll_set_clock_rate(PLL_CLKSEL_REMOTE, PLL_CLKSEL_REMOTE_3M);
		break;
	default:
		DBG_ERR("error type %d\n", (int)(src_type));
		rt = E_PAR;
		goto err;
	}

#endif
#endif

	// Change cycle time
	switch (src_type) {
	case REMOTE_CLK_SRC_RTC:
	default:
		guiRemoteCycleTime = RM_CYCLE_TIME_RTC;
		break;
	case REMOTE_CLK_SRC_OSC:
		guiRemoteCycleTime = RM_CYCLE_TIME_OSC;
		break;
	case REMOTE_CLK_SRC_EXT:
		break;
	case REMOTE_CLK_SRC_3M:
		guiRemoteCycleTime = RM_CYCLE_TIME_3M;
		break;
	}

	return E_OK;

err:
	return rt;


#elif defined(_BSP_NA51000_)
	if (guiRemoteStatus != REMOTE_STATUS_CLOSED && src_type != RM_CYCLE_TIME_RTC) {
		DBG_WRN("680 remote has only one clock source. Don't support change clock source!\r\n");
		return E_SYS;
	}

	return E_OK;

#endif
}
