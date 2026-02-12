/*
    H.26x module driver.

    @file       h26x.c
    @ingroup    mIDrvCodec_H26X
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#if (defined __LINUX || defined __FREERTOS)
#ifdef __LINUX
#include <mach/rcw_macro.h>
#include <plat-na51055/nvt-sramctl.h>
#include <plat/efuse_protected.h>
#include <plat/top.h>
#include <linux/clk.h>
#else
#include <rtos_na51055/nvt-sramctl.h>
#include <rtos_na51055/top.h>
#include <string.h>
#include "pll_protected.h"
#include "efuse_protected.h"
#endif

#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "nvt_vdocdc_dbg.h"

#define FLGPTN_H26X	FLGPTN_BIT(0)	// interrupt flag

// semaphore & interrupt flag id
static SEM_HANDLE	SEMID_H26X;
#if defined(__LINUX)
static FLGPTN		FLG_ID_H26X;

struct clk *g_eng_clk = NULL;

#elif defined(__FREERTOS)
static ID		FLG_ID_H26X;
#endif

#else
// UITRON //
#include <string.h>

#include "DrvCommon.h"
#include "nvtDrvProtected.h"
#include "interrupt.h"
#endif

#include "h26x_reg.h"
#include "h26x.h"

#define H26X_INT_ENABLE     ENABLE
//#define H26X_INT_ENABLE     DISABLE

static ID     guih26xLockStatus = NO_TASK_LOCKED;
static BOOL   gbh26xOpened = FALSE;
static UINT32 guih26xClock = 0;
static UINT32 guih26xIntStatus = 0;
static BOOL   h26x_tick_en = FALSE;
static VOS_TICK h26x_tick_begin, h26x_tick_end, h26x_tick_duration;

static volatile H26X_HW_REG *pH26X_HW_REG = NULL;

UINT32 g_rst_addr;

static void h26x_tick_sum(void)
{
	h26x_tick_duration += vos_perf_duration(h26x_tick_begin, h26x_tick_end);
}

void h26x_enableClk(void)
{
	#if defined(__FREERTOS)
	pll_enableClock(H265_CLK);
	#else

	if (IS_ERR(g_eng_clk)) {
		printk("get clk failed\r\n");
	} else {
		clk_enable(g_eng_clk);
	}

	#endif
}

void h26x_disableClk(void)
{

#if defined(__FREERTOS)
	// disable clock //
	pll_disableClock(H265_CLK);
	// Turn off power //
	//pmc_turnoffPower(PMC_MODULE_H26X);
#else
	if (IS_ERR(g_eng_clk)) {
		DBG_ERR("get clk failed\r\n");
	} else {
		clk_disable(g_eng_clk);
	}
#endif
}

static void h26x_setGating(int enable)
{

#if defined(__FREERTOS)
	if (enable == 0)
		pll_clearClkAutoGating(H265_M_GCLK);
	else
		pll_setClkAutoGating(H265_M_GCLK);
#else
	clk_set_phase(g_eng_clk, enable);
#endif
}

static void h26x_setSRAMCfg(void)
{
	pH26X_HW_REG->SRAM_PW_CFG &= 0x1fffffff;
	#if 0
	pH26X_HW_REG->SRAM_PW_CFG |= (0x1 << 29);
	pH26X_HW_REG->SRAM_PW_CFG |= (0x1 << 30);
	#else
	pH26X_HW_REG->SRAM_PW_CFG &= ~(0x1 << 29);
	pH26X_HW_REG->SRAM_PW_CFG &= ~(0x1 << 30);
	#endif
}

void h26x_setSRAMMode(UINT32 uiCycle,UINT32 uiLightSleepEn, UINT32 uiSleepDownEn)
{
	pH26X_HW_REG->SRAM_PW_CFG &= ~(0xFFFF << 0);
	pH26X_HW_REG->SRAM_PW_CFG |= (uiCycle << 0);
	pH26X_HW_REG->SRAM_PW_CFG &= ~(0x1 << 29);
	pH26X_HW_REG->SRAM_PW_CFG |= ((uiLightSleepEn & 0x1) << 29);
	pH26X_HW_REG->SRAM_PW_CFG &= ~(0x1 << 30);
	pH26X_HW_REG->SRAM_PW_CFG |= ((uiSleepDownEn & 0x1) << 30);

	//DBG_DUMP("SRAM_PW_CFG : 0x%08x\r\n", (int)pH26X_HW_REG->SRAM_PW_CFG);
}

static void h26x_setWakeUpSRAM(void)
{
	pH26X_HW_REG->SRAM_PW_CFG |= (0x1 << 31);
}

//! Attach driver
/*!
    Configures clock rate and enable clock for h26x

    @return void
*/
void h26x_setClk(UINT32 clock)
{
    if (clock == 0) {
        DBG_WRN("input clock is 0, force to set to 1\r\n");
        clock = 1;
    }
	guih26xClock = clock;
#if defined(__LINUX)
	g_eng_clk = clk_get(NULL, "f0a10000.h26x");
#elif defined(__FREERTOS)
	if (guih26xClock == 240000000) {
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
	}
	else if (guih26xClock == 320000000) {
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE)
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_320);
	}
	else {
		if (guih26xClock > 320000000)
			DBG_WRN("h26x clk(%d) > 320, set to PLL15 base on system PLL15 setting.\r\n", (int)(guih26xClock));
		if (pll_getPLLEn(PLL_ID_15) == FALSE)
			pll_setPLLEn(PLL_ID_15, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL15);
	}
#endif
}

void h26x_setAPBAddr(UINT32 addr)
{
	pH26X_HW_REG = (H26X_HW_REG *)addr;
}

void h26x_setRstAddr(UINT32 addr)
{
	g_rst_addr = addr;
}

//! Lock h26x module
/*!
    Lock h26x driver resource.

    @return
        - @b E_OK: lock success
        - @b Others: lock fail
*/
H26xStatus h26x_lock(void)
{
	if(guih26xLockStatus == TASK_LOCKED)
	{
		DBG_ERR("H26x lock error at at line:(%d).\r\n", (int)(__LINE__));
		return H26X_STATUS_ERROR_LOCK;
	}

    if (vos_sem_wait(SEMID_H26X) != E_OK)
    {
    	DBG_ERR("H26x lock error at at line:(%d).\r\n", (int)(__LINE__));
		return H26X_STATUS_ERROR_LOCK;
    }
	else
		guih26xLockStatus = TASK_LOCKED;

    return H26X_STATUS_OK;
}

//! Unlock h26x module
/*!
    Unlock h26x driver resource.

    @return
        - @b E_OK: unlock success
        - @b Others: unlock fail
*/
H26xStatus h26x_unlock(void)
{
	if(guih26xLockStatus == NO_TASK_LOCKED)
	{
		DBG_ERR("H26x unlock error at line:(%d).\r\n", (int)(__LINE__));
		return H26X_STATUS_ERROR_LOCK;
	}

	vos_sem_sig(SEMID_H26X);
	guih26xLockStatus = NO_TASK_LOCKED;

    return H26X_STATUS_OK;
}

void h26x_powerOn(void)
{
	h26x_setDramBurstLen(0);

#if defined(__FREERTOS)
	// Turn on power //
	//pmc_turnonPower(PMC_MODULE_H26X);
	// enable clock //
    	//pll_enableClock(H265_CLK);
	h26x_setDramBurstLen(dma_getDramType()==DDR_TYPE_DDR3);
#endif

	// need to move //
	h26x_setSRAMCfg();
	h26x_setSRAMMode(0x32, 1, 1);
}

void h26x_powerOff(void)
{
#if defined(__FREERTOS)
    // disable clock //
    //pll_disableClock(H265_CLK);
    // Turn off power //
    //pmc_turnoffPower(PMC_MODULE_H26X);
#endif
}

/* ////////////////////////////////////////////////////////// */
/* Open/close driver to operate                               */
/* ---------------------------------------------------------- */
/* OS driver level that may need to be modified               */
/* when porting for different OS                              */
/* ////////////////////////////////////////////////////////// */

//! Initialize h26x module
/*!
    Initialize h26x module, setup interrupt service routine, and h26x H/W.

    @param[in] pH26xOpen - config h26x operation mode

    @return
        - @b E_OK: Operation successful
        - @b Others: Operation failed
*/
H26xStatus h26x_open(void)
{
    H26xStatus ret = h26x_lock();

	if (ret == H26X_STATUS_OK) {
		if (guih26xClock == 0) {
			DBG_ERR("h26x clock not set already\r\n");
			h26x_unlock();
			return H26X_STATUS_ERROR_OPEN;
		}

		if (pH26X_HW_REG == NULL) {
			DBG_ERR("h26x apb not set already\r\n");
			h26x_unlock();
			return H26X_STATUS_ERROR_OPEN;
		}

		if(gbh26xOpened == FALSE)
		{
			// initilize hw module //
			#if (defined __LINUX || defined __FREERTOS)
			nvt_disable_sram_shutdown(H264_SD);
			#else
			pinmux_disableSramShutDown(H26X_RSTN);
			#if (H26X_INT_ENABLE == ENABLE)
			drv_enableInt(DRV_INT_H26X);
			#endif
			#endif
			// if auto-clock gatting enable, use power-on while h26x_open and power-off h26x_close	//
			// else power-on while engine trigger start and power-off while engine finish all jobs		//
			h26x_powerOn();
			gbh26xOpened = TRUE;
		}
		//h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SRC_DECMP_ERR_INT | H26X_SWRST_FINISH_INT);
		h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
		h26x_setLLDummyWriteNum(0x10);
		h26x_setHWTimeoutCnt(0x1388);
		//h26x_reset();
		DBG_INFO("HW_VERSION = 0x%08x\r\n", (int)pH26X_HW_REG->HW_VERSION);

		ret = h26x_unlock();
	}

	return ret;
}

//! destory h26x module
/*!
    destory h26x module, reset and disable interrupt for h26x H/W.

    @return
        - @b E_OK: Operation successful
        - @b Others: Operation failed
*/
H26xStatus h26x_close(void)
{
    H26xStatus ret = h26x_lock();

	if (ret == H26X_STATUS_OK)
	{
		if (gbh26xOpened == FALSE)
		{
			DBG_ERR("H26x close error at at line:(%d).\r\n", (int)(__LINE__));
			h26x_unlock();
			ret = H26X_STATUS_ERROR_CLOSE;
		}
		else
		{
			// if auto-clock gatting enable, use power-on while h26x_open and power-off h26x_close	//
			// else power-on while engine trigger start and power-off while engine finish all jobs 		//
			h26x_powerOff();
			#if (defined __LINUX || defined __FREERTOS)
			nvt_enable_sram_shutdown(H264_SD);
			#else
			#if (H26X_INT_ENABLE == ENABLE)
			drv_disableInt(DRV_INT_H26X);   // disable interrupt //
			#endif
			pinmux_enableSramShutDown(H26X_RSTN);
			#endif
			gbh26xOpened = FALSE;
			ret = h26x_unlock();
		}
	}
	return ret;
}

//! Reset interrupt
/*!
    reset interrupt is asserted by h26x

    @return void

*/
void h26x_resetINT(void)
{
    clr_flg(FLG_ID_H26X, FLGPTN_H26X);
}

//! Wait for interrupt asserted by h26x decoder
/*!
    Wait until some interrupt is asserted by h26x encoder

    @return
        - @b H26x_PIC_FSH_INT: operation finishs correctly
        - @b Others: status code
*/
UINT32 h26x_waitINT(void)
{
    UINT32 uiIntStatus = 0;

#if H26X_INT_ENABLE
    FLGPTN  uiFlag;

    wai_flg(&uiFlag, FLG_ID_H26X, FLGPTN_H26X, TWF_ORW | TWF_CLR);

	uiIntStatus = guih26xIntStatus;
	guih26xIntStatus &= (~uiIntStatus);
#else
    UINT32 tmp = 0;
    while((uiIntStatus & h26x_getIntEn()) ==0) {
        if((tmp != 1) && (tmp % 2000000 == 1)) {
            DBG_ERR("h26x pooling mode not get interrupt status\r\n");
            h26x_prtReg();
            h26x_getDebug();
            if(tmp > 8000000) {
                break;
            }
        }
        tmp++;
		uiIntStatus = h26x_getIntStatus();
    }
    h26x_clearIntStatus(uiIntStatus);
#endif  // #if H26x_INT_ENABLE
	//DBG_DUMP("int : %08x\r\n", uiIntStatus);

	if (h26x_tick_en == TRUE) {
		vos_perf_mark(&h26x_tick_end);
		h26x_tick_sum();
	}

	return uiIntStatus;
}

//! Check interrupt
/*!
    Check interrupt is asserted by h26x

    @return void

*/
UINT32 h26x_checkINT(void)
{
    UINT32 uiIntStatus = 0;

#if H26X_INT_ENABLE
	if (kchk_flg(FLG_ID_H26X, FLGPTN_H26X)) {
		#if (defined __LINUX || defined __FREERTOS)
		uiIntStatus = guih26xIntStatus;
		guih26xIntStatus &= (~uiIntStatus);
		#else
   		uiIntStatus = guih26xIntStatus;
		drv_disableInt(DRV_INT_H26X);
		guih26xIntStatus &= (~uiIntStatus);
		drv_enableInt(DRV_INT_H26X);
		#endif
		clr_flg(FLG_ID_H26X, FLGPTN_H26X);
    }
#else
	uiIntStatus = h26x_getIntStatus();
	h26x_clearIntStatus(uiIntStatus);
#endif

	//if(uiIntStatus != H26X_BSOUT_INT)
	{
		h26x_unlock();
	}

    return uiIntStatus;
}

//! reset hw
/*!
    reset hw

    @return void

*/
void h26x_resetHW(void)
{
	h26x_reset();
    h26x_clearIntStatus(H26X_INIT_INT_STATUS);
    guih26xIntStatus = 0;
	/*
    if(guih26xLockStatus == TASK_LOCKED)
    {
        h26x_unlock();
    }
    */
}

void h26x_module_reset(void)
{
	*(UINT32 *)g_rst_addr &= ~(1<<24);
	*(UINT32 *)g_rst_addr |= (1<<24);

	h26x_setSRAMCfg();
	h26x_setSRAMMode(0x32, 1, 1);
	h26x_setLLDummyWriteNum(0x10);
}

// Interrupt service routine for h26x decoder
/*
    receive and take care of interrupt events for h26x decoder

    @return void
*/
void h26x_isr(void)
{
    UINT32 uiIntStatus;
    ER Ret;
    //h26x_prtReg();
    uiIntStatus = h26x_getIntStatus();

    if (uiIntStatus) {
        h26x_clearIntStatus(uiIntStatus);
        guih26xIntStatus |= uiIntStatus;

        Ret = iset_flg(FLG_ID_H26X, FLGPTN_H26X);
        if(Ret != E_OK)
			DBG_ERR("H26x ISR iset_flag error : Ret = %d\r\n", (int)(Ret));
    }
	else
		DBG_ERR("H26x ISR interrupt error : uiIntStatus = %d\r\n", (int)(uiIntStatus));
}

UINT32 h26x_exit_isr(void)
{
    guih26xIntStatus |= 0x80000000;
    iset_flg(FLG_ID_H26X, FLGPTN_H26X);

    return guih26xIntStatus;
}

//! Set Interrupt Status
/*!
    Set Interrupt Status for h.264 codec

    @return void
*/
void h26x_resetIntStatus(void)
{
    guih26xIntStatus = 0;
}

// set hw register function //
void h26x_reset(void)
{
	pH26X_HW_REG->INT_EN |= H26X_SWRST_FINISH_INT;
	pH26X_HW_REG->CTRL |= (1 << H26X_SWRST_BIT);
	h26x_waitINT();
}

void h26x_start(void)
{
	if (h26x_tick_en == TRUE) vos_perf_mark(&h26x_tick_begin);
	pH26X_HW_REG->CTRL |= (1 << H26X_START_BIT);
}

void h26x_setIntEn(UINT32 uiVal)
{
	pH26X_HW_REG->INT_EN = uiVal;
}

void h26x_setLLDummyWriteNum(UINT32 uiVal)
{
	pH26X_HW_REG->LLC_CFG_1 &= 0xFFFF00FF;
	pH26X_HW_REG->LLC_CFG_1 |= ((uiVal << 8) & 0x0000FF00);
}

void h26x_setHWTimeoutCnt(UINT32 uiVal)
{
	pH26X_HW_REG->LLC_CFG_1 &= 0x0000FFFF;
	pH26X_HW_REG->LLC_CFG_1 |= ((uiVal << 16) & 0xFFFF0000);
}

void h26x_setDramBurstLen(UINT32 uiSel)
{
	if (uiSel == 1)
		pH26X_HW_REG->CTRL |= (1 << H26X_DRM_SEL_BIT);
	else
		pH26X_HW_REG->CTRL &= ~(1 << H26X_DRM_SEL_BIT);
}

void h26x_setBsDmaEn(void)
{
	pH26X_HW_REG->CTRL |= (1 << H26X_BSDMA_CMD_EN_BIT);
}

void h26x_setBsOutEn(void)
{
	pH26X_HW_REG->CTRL |= (1 << H26X_BSOUT_EN_BIT);
}

void h26x_setChkSumEn(BOOL enable)
{
	if (enable == TRUE)
		pH26X_HW_REG->CTRL |= (1 << H26X_CHKSUM_EN_BIT);
	else
		pH26X_HW_REG->CTRL &= ~(1 << H26X_CHKSUM_EN_BIT);
}

void h26x_setAPBReadLoadSet(UINT32 uiSel)
{
#if 0 //520 removed
	pH26X_HW_REG->CTRL &= ~(1 << H26X_APB_READ_LOAD_BIT);
	pH26X_HW_REG->CTRL |= (uiSel << H26X_APB_READ_LOAD_BIT);
#endif
}

void h26x_setAPBLoadEn(void)
{
	pH26X_HW_REG->CTRL |= (1 << H26X_APB_LOAD_BIT);
}

void h26x_setNextBsBuf(UINT32 uiAddr, UINT32 uiSize)
{
	pH26X_HW_REG->BSOUT_BUF_ADDR = uiAddr;
	pH26X_HW_REG->BSOUT_BUF_SIZE = uiSize;

	h26x_setBsOutEn();
}
void h26x_setNextBsDmaBuf(UINT32 uiAddr)
{

	pH26X_HW_REG->BSDMA_CMD_BUF_ADDR = uiAddr;

	h26x_setBsDmaEn();
}

void h26x_setBsLen(UINT32 uiSize)
{
	pH26X_HW_REG->NAL_HDR_LEN_TOTAL_LEN = uiSize;
}

void h26x_setLock(void)
{
	h26x_lock();
}

void h26x_setUnLock(void)
{
	// only use for emualtion for unlock while swrest + bsout_re-trigger enable at the same time 	//
	// for unlock bsout interrupt but not send next bitstream buffer 							//
	h26x_unlock();
}

void h26x_clearIntStatus(UINT32 uiVal)
{
    pH26X_HW_REG->INT_FLAG = uiVal;
    while(pH26X_HW_REG->INT_FLAG & uiVal);
}

// get hw register function //
UINT32 h26x_getClk(void)
{
	return (guih26xClock/1000000);
}

UINT32 h26x_getDramBurstLen(void)
{
    return (pH26X_HW_REG->CTRL & (1 << H26X_DRM_SEL_BIT)) ? 128 : 256;
}

UINT32 h26x_getIntEn(void)
{
	return pH26X_HW_REG->INT_EN;
}

UINT32 h26x_getIntStatus(void)
{
	return pH26X_HW_REG->INT_FLAG;
}

UINT32 h26x_getHwRegSize(void)
{
	return sizeof(H26XRegSet);
}

static void h26x_setRegSet(UINT32 uiAPBAddr)
{
	#if 0
	memcpy((void *)((UINT32)(pH26X_HW_REG) + H26X_REG_BASIC_START_OFFSET), (void *)uiAPBAddr, sizeof(H26XRegSet));
	#else
	UINT32 i;
	UINT32 uiRegAddr = (UINT32)pH26X_HW_REG + H26X_REG_BASIC_START_OFFSET;
	for (i = 0; i < sizeof(H26XRegSet); i+=4) {
		*(UINT32 *)(uiRegAddr + i) = *(UINT32 *)(uiAPBAddr + i);
	}
	#endif
}


static void h26x_reset2(void)
{
	if (h26x_getChipId() == 98520) {
		UINT32 int_en = pH26X_HW_REG->INT_EN;
		pH26X_HW_REG->INT_EN = 0;
		pH26X_HW_REG->CTRL |= (1 << H26X_SWRST_BIT);
		while(pH26X_HW_REG->INT_FLAG != H26X_SWRST_FINISH_INT);
		pH26X_HW_REG->INT_FLAG = H26X_SWRST_FINISH_INT;
		pH26X_HW_REG->INT_EN = int_en;
	}
}

//! Set registers to encode for current picture
/*!
    Set registers to encode for current picture

    @param[in]  uiRegObjIdx Register object index

    @return
        void
*/
void h26x_setEncDirectRegSet(UINT32 uiAPBAddr)
{
	h26x_reset2();
	// Reset Interrupt //
	h26x_clearIntStatus(H26X_INIT_INT_STATUS);
	h26x_resetIntStatus();
	h26x_resetINT();
	// prepare encode register //
	h26x_setWakeUpSRAM();
	h26x_setRegSet(uiAPBAddr);
	h26x_setBsDmaEn();
	h26x_setBsOutEn();
	//pH26X_HW_REG->LLC_CFG_1 = (0x10<<8);
	if ((h26x_getChipId() == 98528) && (pH26X_HW_REG->FUNC_CFG[0] & (1<<20))) {
		h26x_setGating(0);
	}
	h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
	h26x_setAPBLoadEn();


    //h26x_reset();
}

void h26x_low_latency_patch(UINT32 uiAPBAddr)
{
	UINT8 times=0;

	//h26x_setGating(0);
	pH26X_HW_REG->DBG_CFG = 22;

	while(pH26X_HW_REG->DBG_REPORT[0] & 0x80000000) {
		if (pH26X_HW_REG->DBG_REPORT[0] & 0x80000000) {
			//avoid hang in while loop
			if (times > 20) {
				DBG_ERR("low latency is not ready over triggering 20 times!\r\n");
				break;
			}
			times++;

			h26x_resetHW();
			h26x_module_reset();
			// Reset Interrupt //
			h26x_clearIntStatus(H26X_INIT_INT_STATUS);
			h26x_resetIntStatus();
			h26x_resetINT();
			// prepare encode register //
			h26x_setWakeUpSRAM();
			h26x_setRegSet(uiAPBAddr);
			h26x_setBsDmaEn();
			h26x_setBsOutEn();
			h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
			h26x_setAPBLoadEn();
			h26x_start();
		}
	}
	h26x_setGating(1);
}

static void h26x_setLLCRegSet(UINT32 uiJobNum, UINT32 uiJob0APBAddr)
{
	pH26X_HW_REG->LLC_CFG_0 = uiJob0APBAddr;
	//pH26X_HW_REG->LLC_CFG_1 = ((0x10)<<8 | uiJobNum);
	pH26X_HW_REG->LLC_CFG_1 &= 0xFFFFFF00;
	pH26X_HW_REG->LLC_CFG_1 |= (uiJobNum & 0xFF);
}

void h26x_setEncLLRegSet(UINT32 uiJobNum, UINT32 uiJob0APBAddr)
{
	//h26x_reset();
	// Reset Interrupt //
	h26x_clearIntStatus(H26X_INIT_INT_STATUS);
	h26x_resetIntStatus();
	h26x_resetINT();

	// prepare encode register //
	h26x_setWakeUpSRAM();
	h26x_setLLCRegSet(uiJobNum, uiJob0APBAddr);

	h26x_setBsDmaEn();
	h26x_setBsOutEn();
	h26x_setAPBLoadEn();
	//h26x_reset();
}

BOOL h26x_getBusyStatus(void)
{
	return ((pH26X_HW_REG->CTRL & 0x1000) != 0);
}

// get result //
void h26x_getEncReport(UINT32 uiEncReport[H26X_REPORT_NUMBER])
{
	#if 0
	memcpy(uiEncReport, (void *)((UINT32)(pH26X_HW_REG) + H26X_REG_REPORT_START_OFFSET), sizeof(UINT32)*H26X_REPORT_NUMBER);
	#else
	UINT32 i;
	for (i = 0; i < H26X_REPORT_NUMBER; i++) {
		uiEncReport[i] = pH26X_HW_REG->CHK_REPORT[i];
	}
	#endif
}

UINT32 h26x_getCurJobNum(void)
{
	return pH26X_HW_REG->LLC_CFG_2;
}

// debug tools //
void h26x_prtMem(UINT32 uiMemAddr,UINT32 uiMemLen){
    UINT32 i, pad;
    UINT32 *puiMemAddr;

    uiMemLen = ((uiMemLen+3)>>2)<<2;

    puiMemAddr = (UINT32 *)uiMemAddr;

    DBG_DUMP("H26x PrtMem[0x%08x][0x%08x]\r\n", (int)uiMemAddr, (int)uiMemLen);

	//pad = 4;
	pad = 8;

	for (i=0; i<(uiMemLen/4); i+=pad)
	{
		#if 0
		UINT32 j;
		//DBG_DUMP("0x%08x [0x%04x]:", (uiMemAddr+(i*4)),i*4);
		DBG_DUMP("0x%08x:", (uiMemAddr+(i*4)));
		for (j = 0; j < pad; j++)
			DBG_DUMP("%08x ", *(puiMemAddr + i + j));
		DBG_DUMP("\r\n");
		#else
		DBG_DUMP("0x%08x: %08x %08x %08x %08x %08x %08x %08x %08x\r\n", (int)(uiMemAddr+(i*4)),
			(int)(*(puiMemAddr + i)), (int)(*(puiMemAddr + i + 1)), (int)(*(puiMemAddr + i+ 2)), (int)(*(puiMemAddr + i + 3)),
			(int)(*(puiMemAddr + i + 4)), (int)(*(puiMemAddr + i + 5)), (int)(*(puiMemAddr + i+ 6)), (int)(*(puiMemAddr + i + 7)));
		#endif
    }
}

void h26x_prtReg(void){
    //return;
    h26x_setAPBReadLoadSet(0);
    h26x_prtMem((UINT32)pH26X_HW_REG, sizeof(H26X_HW_REG));
    DBG_DUMP("guih26xIntStatus = 0x%x\r\n", (unsigned int)(guih26xIntStatus));
    DBG_DUMP("\r\n------------\r\n");
	h26x_setAPBReadLoadSet(1);
}

UINT32 h26x_getDbg1(UINT32 uiSel)
{
	pH26X_HW_REG->DBG_CFG &= 0xffffff00;
	pH26X_HW_REG->DBG_CFG |= (uiSel);

	return pH26X_HW_REG->DBG_REPORT[0];
}

UINT32 h26x_getDbg2(UINT32 uiSel)
{
	pH26X_HW_REG->DBG_CFG &= 0xffff00ff;
	pH26X_HW_REG->DBG_CFG |= (uiSel<<8);

	return pH26X_HW_REG->DBG_REPORT[1];
}

UINT32 h26x_getDbg3(UINT32 uiSel)
{
	pH26X_HW_REG->DBG_CFG &= 0xff00ffff;
	pH26X_HW_REG->DBG_CFG |= (uiSel<<16);

	return pH26X_HW_REG->DBG_REPORT[2];
}

void h26x_getDebug(void){
    UINT32 result[3];
    UINT32 i;
    DBG_DUMP("              0x240       0x244       0x248\r\n");
    for(i=0;i<100;i++)
    {
        result[0] = h26x_getDbg1(i);
        result[1] = h26x_getDbg2(i);
        result[2] = h26x_getDbg3(i);
        DBG_DUMP("[%02d] 0x%08x, 0x%08x, 0x%08x\r\n", (int)i, (int)result[0], (int)result[1], (int)result[2]);
    }
}

UINT32 h26x_getPhyAddr(UINT32 uiAddr)
{
	return vos_cpu_get_phy_addr(uiAddr);
}

void h26x_flushCache(UINT32 uiAddr, UINT32 uiSize)
{
	BOOL bAlign64 = (((uiAddr & 0x3F)==0) && ((uiSize & 0x3F)==0))? 1: 0;

	if (bAlign64)
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL);
	else
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL_NON_ALIGN);
}

void h26x_cache_clean(UINT32 uiAddr, UINT32 uiSize)
{
	BOOL bAlign64 = (((uiAddr & 0x3F)==0) && ((uiSize & 0x3F)==0))? 1: 0;

	if (bAlign64)
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_TO_DEVICE);
	else
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_TO_DEVICE_NON_ALIGN);
}

void h26x_cache_invalidate(UINT32 uiAddr, UINT32 uiSize)
{
	BOOL bAlign64 = (((uiAddr & 0x3F)==0) && ((uiSize & 0x3F)==0))? 1: 0;

	if (bAlign64)
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_FROM_DEVICE);
	else
		vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_FROM_DEVICE_NON_ALIGN);
}

void h26x_setBSDMA(UINT32 uiBSDMAAddr, UINT32 uiHwHeaderNum, UINT32 uiHwHeaderAddr, UINT32 *uiHwHeaderSize){
    UINT32 i;
    UINT32 uiOriuiBSDMAAddr = uiBSDMAAddr;
    //DBG_ERR("uiHwHeaderNum = %d, uiBSDMAAddr = 0x%08x\r\n",uiHwHeaderNum,uiBSDMAAddr);
    *(volatile UINT32 *)(uiBSDMAAddr) = uiHwHeaderNum; uiBSDMAAddr = uiBSDMAAddr + 4;

    for (i=0;i<uiHwHeaderNum;i++){
        *(volatile UINT32 *)(uiBSDMAAddr) = uiHwHeaderAddr; uiBSDMAAddr = uiBSDMAAddr + 4;
        *(volatile UINT32 *)(uiBSDMAAddr) = *uiHwHeaderSize; uiBSDMAAddr = uiBSDMAAddr + 4;
        //DBG_ERR("uiBSDMAAddr = 0x%08x,0x%08x\r\n",uiHwHeaderAddr,*uiHwHeaderSize);
        uiHwHeaderAddr += (*uiHwHeaderSize & 0xfffffff);
        //uiHwHeaderAddr += (((((*uiHwHeaderSize) + 3)/4)*4) & 0xfffffff);
        uiHwHeaderSize++;
    }

	h26x_cache_clean(uiOriuiBSDMAAddr, (uiHwHeaderNum*2 + 1)*4);

}
void h26x_setBsOutAddr(UINT32 uiAddr, UINT32 uiSize)
{
	pH26X_HW_REG->BSOUT_BUF_ADDR = h26x_getPhyAddr(uiAddr);
	pH26X_HW_REG->BSOUT_BUF_SIZE = uiSize;
	pH26X_HW_REG->CTRL |= (1 << H26X_BSOUT_EN_BIT);
}

UINT32 h26x_getBsOutAddr(void)
{
	return pH26X_HW_REG->BSOUT_BUF_ADDR;
}
UINT32 h26x_getTmnrSumY(void){
	h26x_setDebugSel(0,68,0);
	return h26x_getCheckSumSelResult();
}
UINT32 h26x_getTmnrSumC(void){
	h26x_setDebugSel(0,69,0);
	return h26x_getCheckSumSelResult();
}
UINT32 h26x_getMaskSumY(void){
	h26x_setDebugSel(0,70,0);
	return h26x_getCheckSumSelResult();
}
UINT32 h26x_getMaskSumC(void){
	h26x_setDebugSel(0,71,0);
	return h26x_getCheckSumSelResult();
}
UINT32 h26x_getOsgSumY(void){
	h26x_setDebugSel(0,72,0);
	return h26x_getCheckSumSelResult();
}
UINT32 h26x_getOsgSumC(void){
	h26x_setDebugSel(0,73,0);
	return h26x_getCheckSumSelResult();
}
void h26x_setDebugSel(UINT32 uiVal370,UINT32 uiVal374,UINT32 uiVal378){
    pH26X_HW_REG->DBG_CFG = (uiVal370 & 0xFF) << 0;
    pH26X_HW_REG->DBG_CFG |= (uiVal374 & 0xFF) << 8;
    pH26X_HW_REG->DBG_CFG |= (uiVal378 & 0xFF) << 16;
}
UINT32 h26x_getCheckSumSelResult(void){
    return pH26X_HW_REG->DBG_REPORT[1];
}


void h26x_setUnitChecksum(UINT32 uiVal){
    pH26X_HW_REG->RES_24C = uiVal;
    DBG_DUMP("h26x_setUnitChecksum %08x\r\n", (int)pH26X_HW_REG->RES_24C);
}

UINT32 h26x_getUnitChecksum(void){
    return pH26X_HW_REG->RES_24C;
}

UINT32 h26x_getBslen(void){
	return pH26X_HW_REG->CHK_REPORT[H26X_BS_LEN];
}

UINT32 h26x_getChipId(void)
{
	if (nvt_get_chip_id() == CHIP_NA51055)
		return 98520;
	else
		return 98528;

	return 0;
}

void h26x_tick_open(void)
{
	h26x_tick_en = TRUE;
	h26x_tick_duration = 0;
}

void h26x_tick_close(void)
{
	h26x_tick_en = FALSE;
}

VOS_TICK h26x_tick_result(void)
{
	return h26x_tick_duration;
}

void h26x_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_H26X);
	SEM_CREATE(SEMID_H26X, 1);
}

void h26x_release_resource(void)
{
	rel_flg(FLG_ID_H26X);
	SEM_DESTROY(SEMID_H26X);
}

BOOL h26x_efuse_check(UINT32 uiWidth, UINT32 uiHeight)
{
	CDC_ABILITY cdc_ability;

	cdc_ability.uiWidth = uiWidth;
	cdc_ability.uiHeight = uiHeight;

	if (efuse_check_available_extend(EFUSE_ABILITY_CDC_RESOLUTION, (UINT32)&cdc_ability)) {
		if (efuse_check_available_extend(EFUSE_ABILITY_CDC_PLL_FREQ, guih26xClock)) {
			return TRUE;
		}
		else {
			DBG_ERR("cannot support clock(%d)\r\n", (int)guih26xClock);
			return FALSE;
		}
	}
	else {
		DBG_ERR("cannot support this chip config(%d, %d).\r\n", (int)cdc_ability.uiWidth, (int)cdc_ability.uiHeight);
		return FALSE;
	}

	return TRUE;
}

#if 0
void h26x_prtDebug(void)
{
    UINT32 x,y;
    UINT32 d1,d2,d3,d4,d5;

    h26x_getXY(&x, &y);
    h26x_get350Debug(&d1,&d2,&d3,&d4,&d5);
    DBG_DUMP("\r\n");
    DBG_DUMP("X = %d, Y = %d\r\n", (int)(x), (int)(y));
    DBG_DUMP("SrcCsY         = 0x%08x\r\n",h26x_getSrcCheckSum_Y());
    DBG_DUMP("SrcCsC         = 0x%08x\r\n",h26x_getSrcCheckSum_C());
    DBG_DUMP("cycle          = 0x%08x\r\n",h26x_getCycle());
    DBG_DUMP("Debug          = 0x%08x\r\n",h26x_getDebugCtr());
    DBG_DUMP("DMA_DEBUG      = 0x%08x\r\n",d1);
    DBG_DUMP("SRC_DEBUG      = 0x%08x\r\n",d2);
    DBG_DUMP("ENTROPY_DEBUG  = 0x%08x\r\n",d3);
    DBG_DUMP("ENTROPY_DEBUG1  = 0x%08x\r\n",d4);
    DBG_DUMP("ENTROPY_DEBUG2  = 0x%08x\r\n",d5);
    h26x_get370Debug();
    h26x_getSWRSTDebug();
}
#endif

#if 0
void h26x_doAgain(UINT32 uiIsEnc, UINT32 EC_KTab[9], UINT32 ED_KTab[9])
{
    //DBG_ERR("h26x_doAgain\r\n");
    // S/W Reset //

    if(uiIsEnc)
    {
        h26x_setECKTablW( EC_KTab);
        h26x_setEDKTabl(ED_KTab);
    }

    h26x_reset();

	h26x_setWakeUpSRAM();
    // clear interrupt status //
    h26x_clearIntStatus(H26X_INIT_INT_STATUS);

    // Reset Interrupt
    h26x_resetINT();

    // Reset Status Interrupt
    h26x_resetIntStatus();

    // Set BSDMA CMD Buffer addr //
    h26x_setBsdmaCmdBufAddrEn();

    h26x_setBsOutBufEn();
    h26x_load();
    h26x_reset();

    // S/W Start //
    h26x_start();
}
#endif




