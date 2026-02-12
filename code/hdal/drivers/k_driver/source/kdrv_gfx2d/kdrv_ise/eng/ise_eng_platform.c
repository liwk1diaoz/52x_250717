/*
    ISE module driver

    NT96520 ISE module driver.

    @file       ise_eng_platform.c
    @ingroup    mIIPPISE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

//---------------------------------------------------------------
#include "ise_eng_int_platform.h"

#define ISE_ENG_CLOCK_240        240
#define ISE_ENG_CLOCK_320        320
#define ISE_ENG_CLOCK_360        360
#define ISE_ENG_CLOCK_480        480

VOID ise_eng_platform_prepare_clk(ISE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("ISE: get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_prepare(p_eng->pclk);
	}
#else
	pll_enableSystemReset(ISE_RSTN);
	pll_disableSystemReset(ISE_RSTN);
#endif
}

VOID ise_eng_platform_unprepare_clk(ISE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("ISE: get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_unprepare(p_eng->pclk);
	}
#endif
}

VOID ise_eng_platform_enable_clk(ISE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("ISE: get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_enable(p_eng->pclk);
	}
#else
	pll_enableClock(ISE_CLK);
#endif
}

VOID ise_eng_platform_disable_clk(ISE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("ISE: get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_disable(p_eng->pclk);
	}
#else
	pll_disableClock(ISE_CLK);
#endif
}

INT32 ise_eng_platform_set_clk_rate(ISE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
		struct clk *parent_clk = NULL;

		if (IS_ERR(p_eng->pclk)) {
			DBG_ERR("ISE: get clk fail...0x%p\r\n", p_eng->pclk);
			return E_SYS;
		}

		switch (p_eng->clock_rate) {
		case ISE_ENG_CLOCK_480:
			parent_clk = clk_get(NULL, "fix480m");
			break;

		case ISE_ENG_CLOCK_360:
			parent_clk = clk_get(NULL, "pll13");
			break;

		case ISE_ENG_CLOCK_320:
			parent_clk = clk_get(NULL, "pllf320");
			break;

		case ISE_ENG_CLOCK_240:
			parent_clk = clk_get(NULL, "fix240m");
			break;
		default:
			DBG_ERR("ISE: unknown clk rate !!\r\n");
			parent_clk = clk_get(NULL, "fix240m");
			break;
		}

		clk_set_parent(p_eng->pclk, parent_clk);
		clk_put(parent_clk);


#elif defined (__FREERTOS )

		UINT32 uiSelectedClock;

		// Turn on power

		// select clock
		if (p_eng->clock_rate >= ISE_ENG_CLOCK_480) {
			uiSelectedClock = PLL_CLKSEL_ISE_480;
			if (p_eng->clock_rate > ISE_ENG_CLOCK_480) {
				DBG_WRN("ISE: user-clock - %d, engine-clock: 480MHz\r\n", (int)p_eng->clock_rate);
			}
		} else if (p_eng->clock_rate > ISE_ENG_CLOCK_360) {
			uiSelectedClock = PLL_CLKSEL_ISE_PLL13;
			DBG_WRN("ISE: user-clock - %d, engine-clock: 360MHz\r\n", (int)p_eng->clock_rate);
		} else if (p_eng->clock_rate == ISE_ENG_CLOCK_360) {
			uiSelectedClock = PLL_CLKSEL_ISE_PLL13;
		} else if (p_eng->clock_rate > ISE_ENG_CLOCK_320) {
			uiSelectedClock = PLL_CLKSEL_ISE_PLL13;
			DBG_WRN("ISE: user-clock - %d, engine-clock: 360MHz\r\n", (int)p_eng->clock_rate);
		} else if (p_eng->clock_rate == ISE_ENG_CLOCK_320) {
			uiSelectedClock = PLL_CLKSEL_ISE_320;
		} else if (p_eng->clock_rate > ISE_ENG_CLOCK_240) {
			uiSelectedClock = PLL_CLKSEL_ISE_320;
			DBG_WRN("ISE: user-clock - %d, engine-clock: 320MHz\r\n", (int)p_eng->clock_rate);
		} else if (p_eng->clock_rate == ISE_ENG_CLOCK_240) {
			uiSelectedClock = PLL_CLKSEL_ISE_240;
		} else {
			uiSelectedClock = PLL_CLKSEL_ISE_240;
			DBG_WRN("ISE: user-clock - %d, engine-clock: 240MHz\r\n", (int)p_eng->clock_rate);
		}


		if (uiSelectedClock == PLL_CLKSEL_ISE_320) {
			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (uiSelectedClock == PLL_CLKSEL_ISE_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}


		pll_setClockRate(PLL_CLKSEL_ISE, uiSelectedClock);
#endif

	return E_OK;
}

VOID ise_eng_platform_disable_sram_shutdown(ISE_ENG_HANDLE *p_eng)
{
	nvt_disable_sram_shutdown(p_eng->sram_id);
}

VOID ise_eng_platform_enable_sram_shutdown(ISE_ENG_HANDLE *p_eng)
{
	nvt_enable_sram_shutdown(p_eng->sram_id);
}