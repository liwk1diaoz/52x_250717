/*
    H.26x module driver.

    @file       h26x.c
    @ingroup    mIDrvCodec_H26X
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#if (defined __LINUX || defined __FREERTOS)
#ifdef __LINUX
#include <linux/mm.h> //
#include <mach/rcw_macro.h>
#include <plat-na51055/nvt-sramctl.h>
#include <plat/efuse_protected.h>
#else
#include <rtos_na51055/nvt-sramctl.h>
#include <string.h>
#include "pll_protected.h"
#include "efuse_protected.h"
#endif
#include <stdlib.h> //rand

#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kdrv_vdocdc_dbg.h"
#ifdef VDOCDC_LL
#include "kdrv_vdocdc_thread.h"
#endif

#include <arm_neon.h>
#include "comm/hwclock.h"  // hwclock_get_counter

#define FLGPTN_H26X	FLGPTN_BIT(0)	// interrupt flag

// semaphore & interrupt flag id
static SEM_HANDLE	SEMID_H26X;
#if defined(__LINUX)
static FLGPTN		FLG_ID_H26X;
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

#if H26X_INT_ENABLE
#include "interrupt.h" //request_irq
#endif

static ID     guih26xLockStatus = NO_TASK_LOCKED;
static BOOL   gbh26xOpened = FALSE;
static UINT32 guih26xClock = 0;
static UINT32 guih26xIntStatus = 0;

static volatile H26X_HW_REG *pH26X_HW_REG = NULL;

UINT32 g_rst_addr;


UINT32 h26x_getHwVersion(void)
{
    return pH26X_HW_REG->HW_VERSION;
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

	DBG_DUMP("SRAM_PW_CFG : 0x%08x\r\n", (int)pH26X_HW_REG->SRAM_PW_CFG);
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
	guih26xClock = clock;

#if defined(__FREERTOS)
	if (clock == 240) {
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
	}
	else if (clock == 320)
	{
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE)
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_320);
	}
	else
	{
		if (clock > 320)
            DBG_DUMP("h26x clk(%d) > 320, set to PLL15 base on system PLL15 setting.\r\n", (int)(clock));
		if (pll_getPLLEn(PLL_ID_15) == FALSE)
			pll_setPLLEn(PLL_ID_15, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL15);
	}
#endif
}

void h26xEmuClkSel(UINT32 uiVal)
{
	UINT32 uiTmp;
	pll_disableClock(H265_CLK);
	switch (uiVal) {
	case 0:  	//FPGA Y23 //
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
		DBG_DUMP("Set t0 240\r\n");
		break;
	case 1: 	//FPGA Y1 //
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE)
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_320);
		DBG_DUMP("Set t0 320\r\n");
		break;
	case 2:		//FPGA Y23 //
		//uiTmp = 280;
		uiTmp = (200 + rand() % 120); // 200 ~ 320 //
		DBG_DUMP("Set t0 PLL15 (%d)\r\n", (int)uiTmp);
		uiTmp = uiTmp * 131072 / 12;
		pll_setPLLEn(PLL_ID_15, FALSE);
		//pll_setPLL(PLL_ID_15, uiTmp); // 250MHz
		pll_setDrvPLL(PLL_ID_15, uiTmp); // 360MHz
		pll_setPLLEn(PLL_ID_15, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL15);
		break;
	case 3: 	//FPGA Y27 //
		//uiTmp = 280;
		uiTmp = (200 + rand() % 120); // 200 ~ 320 //
		DBG_DUMP("Set t0 PLL13 (%d)\r\n", (int)uiTmp);
		uiTmp = uiTmp * 131072 / 12;
		pll_setPLLEn(PLL_ID_13, FALSE);
		//pll_setPLL(PLL_ID_13, uiTmp); // 250MHz
		pll_setDrvPLL(PLL_ID_13, uiTmp); // 360MHz
		pll_setPLLEn(PLL_ID_13, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL13);
		break;
	default:
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
		break;
	}

	pll_enableClock(H265_CLK);

}

// 0 : 240
// 1 : 320
// 2 : pll13 (200 + rand() % 120); // 200 ~ 320 //


void h26xEmuClkSel2(UINT32 uiVal, UINT32 speed)
{
#if 0
	//extern int ddrburst;
	UINT32 uiTmp;
    int pl;
	pll_disableClock(H265_CLK);
	switch (uiVal) {
	case 0:  	//FPGA Y23 //
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
		DBG_DUMP("!1Set t0 240, ddr = %d\r\n",ddrburst);
		break;
	case 1: 	//FPGA Y1 //
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE)
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_320);
		DBG_DUMP("!1Set t0 320, ddr = %d\r\n",ddrburst);
		break;
	case 2:		//FPGA Y23 //
		//uiTmp = 280;
		//uiTmp = (200 + rand() % 120); // 200 ~ 320 //
		uiTmp = speed;
		DBG_DUMP("Set t0 PLL15 (%d), ddr = %d\r\n", (int)uiTmp,ddrburst);
		uiTmp = uiTmp * 131072 / 12;
		pll_setPLLEn(PLL_ID_15, FALSE);
#if 1
		//pll_setPLL(PLL_ID_15, uiTmp); // 250MHz
		pll_setDrvPLL(PLL_ID_15, uiTmp); // 360MHz
#else
        uiTmp = speed;
        uiTmp = uiTmp * 1000000;
        pll_set_pll_freq(PLL_ID_15, uiTmp);
#endif
		pll_setPLLEn(PLL_ID_15, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL15);
        pl = pll_get_pll_freq(PLL_ID_15);
        DBG_DUMP("PLL_ID_13 = %d\r\n",pl);
		break;
	case 3: 	//FPGA Y27 //
		//uiTmp = 280;
		//uiTmp = (200 + rand() % 120); // 200 ~ 320 //
		uiTmp = speed;
		DBG_DUMP("!1Set t0 PLL13 (%d), ddr = %d\r\n", (int)uiTmp,ddrburst);
		uiTmp = uiTmp * 131072 / 12;
		pll_setPLLEn(PLL_ID_13, FALSE);
#if 0
		pll_setDrvPLL(PLL_ID_13, uiTmp); // 360MHz
#else
		uiTmp = speed;
        uiTmp = uiTmp * 1000000;
        pll_set_pll_freq(PLL_ID_13, uiTmp);
#endif
		pll_setPLLEn(PLL_ID_13, TRUE);
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_PLL13);
        pl = pll_get_pll_freq(PLL_ID_13);
        DBG_DUMP("PLL_ID_15 = %d\r\n",pl);
		break;
	default:
		pll_setClockRate(PLL_CLKSEL_H265, PLL_CLKSEL_H265_240);
		break;
	}
	pll_enableClock(H265_CLK);
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
static H26xStatus h26x_lock(void)
{
	if(guih26xLockStatus == TASK_LOCKED)
	{
		DBG_DUMP("H26x lock error at at line:(%d).\r\n", (int)(__LINE__));
		return H26X_STATUS_ERROR_LOCK;
	}

    if (vos_sem_wait(SEMID_H26X) != E_OK)
    {
    	DBG_DUMP("H26x lock error at at line:(%d).\r\n", (int)(__LINE__));
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
static H26xStatus h26x_unlock(void)
{
	if(guih26xLockStatus == NO_TASK_LOCKED)
	{
		DBG_DUMP("H26x unlock error at line:(%d).\r\n", (int)(__LINE__));
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
    pll_enableClock(H265_CLK);
	h26x_setDramBurstLen(dma_getDramType()==DDR_TYPE_DDR3);
#endif

	// need to move //
	h26x_setSRAMCfg();
	h26x_setSRAMMode(0x32, 1, 1);

	//h26x_setSRAMMode(0x32, 0, 0);//hk tmp
}

void h26x_powerOff(void)
{
#if defined(__FREERTOS)
    // disable clock //
    pll_disableClock(H265_CLK);
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
			DBG_DUMP("h26x clock not set already\r\n");
			h26x_unlock();
			return H26X_STATUS_ERROR_OPEN;
		}

		if (pH26X_HW_REG == NULL) {
			DBG_DUMP("h26x apb not set already\r\n");
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
		h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SRC_DECMP_ERR_INT | H26X_SWRST_FINISH_INT | H26X_SRC_D2D_OVERWRITE_EN);

		DBG_DUMP("h26x_setLLDummyWriteNum = 128\r\n");

		h26x_setLLDummyWriteNum(0x10);//uitron
		//h26x_setLLDummyWriteNum(0xFF);//hk:freertos
		//h26x_setLLDummyWriteNum(0);

		h26x_setHWTimeoutCnt(0x1388);
		h26x_setChkSumEn(TRUE);

#if 1 //gating // link list mode
        h26x_setCodecClock(FALSE);
		h26x_setCodecPClock(FALSE);
#else //Direct mode
        h26x_setCodecClock(TRUE);
		h26x_setCodecPClock(TRUE);
#endif

		//h26x_reset();
        DBG_DUMP("HW_VERSION = 0x%08x\r\n", (int)pH26X_HW_REG->HW_VERSION);

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
			DBG_DUMP("H26x close error at at line:(%d).\r\n", (int)(__LINE__));
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

	//hk:disable_codec_flow h26x_unlock(); return 1;

#if H26X_INT_ENABLE
    FLGPTN  uiFlag;

    wai_flg(&uiFlag, FLG_ID_H26X, FLGPTN_H26X, TWF_ORW | TWF_CLR);

	uiIntStatus = guih26xIntStatus;
	guih26xIntStatus &= (~uiIntStatus);
#else
    UINT32 tmp = 0;
    while((uiIntStatus & h26x_getIntEn()) ==0) {
        if((tmp != 1) && (tmp % 2000000 == 1)) {
            DBG_DUMP("h26x pooling mode not get interrupt status\r\n");
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
	//DBG_DUMP("int : %08x\r\n", (unsigned int)uiIntStatus);

	//if ((uiIntStatus & (H26X_FINISH_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_FBC_ERR_INT | H26X_SRC_DECMP_ERR_INT | H26X_SWRST_FINISH_INT )) != 0) {
	//if ((uiIntStatus & (H26X_FINISH_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT)) != 0) {
	if ((uiIntStatus & (pH26X_HW_REG->INT_EN & ~(H26X_BSDMA_INT | H26X_BSOUT_INT))) != 0) { //hk tmp add bsout
		h26x_unlock();
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
    if(guih26xLockStatus == TASK_LOCKED)
    {
        h26x_unlock();
    }
}

void h26x_module_reset(void)
{
	*(UINT32 *)g_rst_addr &= ~(1<<24);
	*(UINT32 *)g_rst_addr |= (1<<24);
	h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SRC_DECMP_ERR_INT | H26X_SWRST_FINISH_INT | H26X_SRC_D2D_OVERWRITE_EN);
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

    if (uiIntStatus)
    {
        h26x_clearIntStatus(uiIntStatus);
        guih26xIntStatus |= uiIntStatus;

        Ret = iset_flg(FLG_ID_H26X, FLGPTN_H26X);
        if(Ret != E_OK)
			DBG_DUMP("H26x ISR iset_flag error : Ret = %d\r\n", (int)(Ret));

#ifdef VDOCDC_LL
		if ((uiIntStatus & (H26X_FINISH_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT )) != 0) {
			h26x_unlock();
		}
		kdrv_vdocdc_get_interrupt(guih26xIntStatus);
#endif

    }
	else
		DBG_DUMP("H26x ISR interrupt error : uiIntStatus = %d\r\n", (int)(uiIntStatus));
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
}
int hevc_start_point = 0;

void h26x_start(void)
{
	hevc_start_point = 0;
	//DBG_DUMP("[%s][%d]\r\n", __FUNCTION__, __LINE__);
    //h26x_prtReg();
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

void h26x_setCodecClock(BOOL enable)
{
	if (enable == TRUE)
		pH26X_HW_REG->CTRL |= (1 << H26X_CLOCK_EN_BIT);
	else
		pH26X_HW_REG->CTRL &= ~(1 << H26X_CLOCK_EN_BIT);
	//DBG_DUMP("[%s][%d] CTRL = 0x%08X\r\n", __FUNCTION__, __LINE__,(int)pH26X_HW_REG->CTRL);
}
void h26x_setCodecPClock(BOOL enable)
{
	if (enable == TRUE)
		pH26X_HW_REG->CTRL |= (1 << H26X_PCLOCK_EN_BIT);
	else
		pH26X_HW_REG->CTRL &= ~(1 << H26X_PCLOCK_EN_BIT);
	//DBG_DUMP("[%s][%d] CTRL = 0x%08X\r\n", __FUNCTION__, __LINE__,(int)pH26X_HW_REG->CTRL);
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
	//pH26X_HW_REG->CTRL |= (1 << H26X_APB_LOAD_BIT);
}
void h26x_setNextBsSize(UINT32 uiSize){
	pH26X_HW_REG->BSOUT_BUF_SIZE = uiSize;
}


void h26x_setNextBsBuf(UINT32 uiAddr, UINT32 uiSize)
{
	//DBG_INFO("[%s][%d] 0x%08x 0x%08x\r\n",__FUNCTION__,__LINE__,(unsigned int)pH26X_HW_REG->BSOUT_BUF_ADDR,(unsigned int)pH26X_HW_REG->BSOUT_BUF_SIZE);

	pH26X_HW_REG->BSOUT_BUF_ADDR = uiAddr;
	pH26X_HW_REG->BSOUT_BUF_SIZE = uiSize;

	//DBG_INFO("[%s][%d] 0x%08x 0x%08x\r\n",__FUNCTION__,__LINE__,(unsigned int)pH26X_HW_REG->BSOUT_BUF_ADDR,(unsigned int)pH26X_HW_REG->BSOUT_BUF_SIZE);

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
	return guih26xClock;
}

UINT32 h26x_getDramBurstLen(void)
{
    return (pH26X_HW_REG->CTRL & (1 << H26X_DRM_SEL_BIT)) ? 128 : 256;
}

UINT32 h26x_getCtrl(void)
{
    return pH26X_HW_REG->CTRL;
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

void h26x_reset2(void)
{
	UINT32 int_en = pH26X_HW_REG->INT_EN;
	pH26X_HW_REG->INT_EN = 0;
	pH26X_HW_REG->CTRL |= (1 << H26X_SWRST_BIT);
	while(pH26X_HW_REG->INT_FLAG != H26X_SWRST_FINISH_INT);
	pH26X_HW_REG->INT_FLAG = H26X_SWRST_FINISH_INT;
	pH26X_HW_REG->INT_EN = int_en;
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
    h26x_lock();
    //h26x_reset();
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
	//disable mb timeout int for d2d mode
	if (pH26X_HW_REG->FUNC_CFG[0] & (1<<20)) {
        h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
    }
    else {
        h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
    }
	h26x_setAPBLoadEn();

    //h26x_reset();
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
	h26x_lock();
    //h26x_reset();
    //h26x_reset2();
    // Reset Interrupt //
    h26x_clearIntStatus(H26X_INIT_INT_STATUS);
    h26x_resetIntStatus();
    h26x_resetINT();

	// prepare encode register //
	h26x_setWakeUpSRAM();
	h26x_setLLCRegSet(uiJobNum, uiJob0APBAddr);

	h26x_setBsDmaEn();
	h26x_setBsOutEn();
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
void h26x_getEncReport2(UINT32 uiEncReport[H26X_REPORT_2_NUMBER])
{
	#if 0
	memcpy(uiEncReport, (void *)((UINT32)(pH26X_HW_REG) + H26X_REG_REPORT_2_START_OFFSET), sizeof(UINT32)*H26X_REPORT_2_NUMBER);
	#else
	UINT32 i;
	for (i = 0; i < H26X_REPORT_2_NUMBER; i++) {
		uiEncReport[i] = pH26X_HW_REG->CHK_REPORT_2[i];
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

	pad = 4;
	//pad = 8;


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
		if(pad==8){
		DBG_DUMP("0x%08x: %08x %08x %08x %08x %08x %08x %08x %08x\r\n", (int)(uiMemAddr+(i*4)),
			(int)(*(puiMemAddr + i)), (int)(*(puiMemAddr + i + 1)), (int)(*(puiMemAddr + i+ 2)), (int)(*(puiMemAddr + i + 3)),
			(int)(*(puiMemAddr + i + 4)), (int)(*(puiMemAddr + i + 5)), (int)(*(puiMemAddr + i+ 6)), (int)(*(puiMemAddr + i + 7)));
		}
		else if (pad==4){
		DBG_DUMP("0x%08x: %08x %08x %08x %08x\r\n", (int)(uiMemAddr+(i*4)),
			(int)(*(puiMemAddr + i)), (int)(*(puiMemAddr + i + 1)), (int)(*(puiMemAddr + i+ 2)), (int)(*(puiMemAddr + i + 3)));

		}
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

UINT32 h26x_getVirAddr(UINT32 uiAddr)
{
	//return vos_cpu_get_phy_addr(uiAddr);
	return uiAddr;
}

void h26x_flushCache(UINT32 uiAddr, UINT32 uiSize)
{
#if 0
	if(uiAddr != SIZE_32X(uiAddr)){
		DBG_INFO("[%s][%d] Error : uiAddr = 0x%08x \n ",__FUNCTION__,__LINE__,(unsigned int) uiAddr);
	}


	if(uiSize != SIZE_32X(uiSize)){
		DBG_INFO("[%s][%d] Error : uiSize = 0x%08x \n ",__FUNCTION__,__LINE__,(unsigned int) uiSize);
	}
#endif
	vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL);
}
void h26x_flushCacheRead(UINT32 uiAddr, UINT32 uiSize)
{
#if 0
	if(uiAddr != SIZE_32X(uiAddr)){
		DBG_INFO("[%s][%d] Error : uiAddr = 0x%08x \n ",__FUNCTION__,__LINE__,(unsigned int) uiAddr);
	}
	if(uiSize != SIZE_32X(uiSize)){
		DBG_INFO("[%s][%d] Error : uiSize = 0x%08x \n ",__FUNCTION__,__LINE__,(unsigned int) uiSize);
	}
#endif
	vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_FROM_DEVICE);
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

	h26x_flushCache(uiOriuiBSDMAAddr, SIZE_32X((uiHwHeaderNum*2 + 1)*4));

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
UINT32 h26x_getBsOutSize(void)
{
	return pH26X_HW_REG->BSOUT_BUF_SIZE;
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

UINT32 h26x_getStableLen(void){
    UINT32 sb;
    sb = pH26X_HW_REG->CHK_REPORT[40];
    return sb;
}

UINT32 h26x_getStableSliceNum(void){
    return pH26X_HW_REG->CHK_REPORT[41];
}

void h26x_setUnitChecksum(UINT32 uiVal){
    pH26X_HW_REG->RES_24C = uiVal;
    DBG_DUMP("h26x_setUnitChecksum %08x\r\n", (int)pH26X_HW_REG->RES_24C);
}

UINT32 h26x_getUnitChecksum(void){
    return pH26X_HW_REG->RES_24C;
}

UINT32 h26x_getNalLen(UINT32 uiSel)
{
	pH26X_HW_REG->NAL_REPORT[0] = 0x80000000 | uiSel;
	return pH26X_HW_REG->NAL_REPORT[1];
}



UINT32 h26x_getBslen(void){
	return pH26X_HW_REG->CHK_REPORT[H26X_BS_LEN];
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
		return TRUE;
	}
	else {
		DBG_DUMP("cannot support this chip config(%d, %d).\r\n", (int)cdc_ability.uiWidth, (int)cdc_ability.uiHeight);
		return FALSE;
	}

	return TRUE;
}

#if defined (__FREERTOS)
#if (H26X_INT_ENABLE == ENABLE)

irqreturn_t videocodc_platform_isr(int irq, void *devid)
{
    h26x_isr();
    return IRQ_HANDLED;
}
#endif
#endif

void  h26x_request_irq(void){
#if (H26X_INT_ENABLE == ENABLE)

	request_irq(INT_ID_H26X, videocodc_platform_isr, IRQF_TRIGGER_HIGH, "H26X_INT", 0);
#else

#endif
}


#if (defined __FREERTOS)
UINT32 h26x_calDramChkSum(UINT32 adr, UINT32 len)
{
    UINT16 tmp=0;
    UINT32 i;
    UINT16 *dst=(UINT16*)adr;

    for (i = 0; i < len/2; i++) {
        tmp += dst[i];
    }

    return tmp;
}
UINT32 h26x_calDram32ChkSum(UINT32 adr, UINT32 len)
{
    UINT16 tmp=0;
    UINT32 i;
    UINT32 *dst=(UINT32*)adr;

    for (i = 0; i < len/4; i++) {
		tmp += (dst[i]>>16) & 0xffff;
        tmp += (dst[i]) & 0xffff;
    }

    return tmp;
}
#if 0
UINT32 add_float_neon(UINT32 adr, UINT32 len)
{
	UINT16 dst[8];
	uint16x8_t sum;
	UINT16 tmp;
	// uint16x8_t one = 1;
	//sum = veorq_u16 (sum, sum); //sum = 0
	UINT16 *src=(UINT16*)adr;
	UINT32 i;
	
	DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());
	for (i = 0; i < len; i += 16)
	{
		uint16x8_t in;	
		in = vld1q_u16(src);
		src += 8;
		sum = vaddq_u16(sum, in);
	}
	
	DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());
	DBG_INFO("add_float_neon  \r\n" );
	// sum = vmlaq_u16 (sum, one)
	vst1q_u16(dst, sum);
	tmp = dst[0] +  dst[1] + dst[2] + dst[3] + dst[4] + dst[5] + dst[6] + dst[7];
	return tmp;
}
#else
UINT32 add_float_neon(UINT32 adr, UINT32 len)
{
	UINT16 dst[8];
	uint16x8_t sum;
	UINT16 tmp;
	// uint16x8_t one = 1;
	//sum = veorq_u16 (sum, sum); //sum = 0
	UINT16 *src=(UINT16*)adr;
	
	uint16x8_t in1, in2;	

	for (; len>0; len -= 16*2)
	{

		in1 = vld1q_u16(src);
		src += 8;
		in2 = vld1q_u16(src);
		src += 8;
		
		sum = vaddq_u16(sum, in1);
		sum = vaddq_u16(sum, in2);


	}
	
	// sum = vmlaq_u16 (sum, one)
	vst1q_u16(dst, sum);
	tmp = dst[0] +  dst[1] + dst[2] + dst[3] + dst[4] + dst[5] + dst[6] + dst[7];
	return tmp;
}

#endif
UINT32 h26x_getDramChkSum(UINT32 adr, UINT32 len)
{
    //dma_getHeavyLoadChkSum(DMA_ID id, DMA_HEAVY_LOAD_CH uiCh, UINT32 uiBurstLen, UINT32 uiAddr, UINT32 uiLength);

    return dma_getHeavyLoadChkSum(0, 0, 128, adr, len);;
	//return dma_getHeavyLoadChkSum(0, 2, 127, adr, len);
}
#endif

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

