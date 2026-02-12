/*
    SIF module driver

    SIF module driver.

    @file       sif.c
    @ingroup    mIDrvIO_SIF
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#if defined(__FREERTOS)
#endif
#if defined(_NVT_FPGA_) && defined(_NVT_EMULATION_)
#endif
#if defined(_BSP_NA51055_)
#define __MODULE__ nvt_sif
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/type.h"

#include "rtos_na51055/sif_api.h"
#include "rtos_na51055/pll.h"

#include "plat/dma.h"
#include "sif_reg.h"
#include "sif_int.h"
#include "interrupt.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "io_address.h"


static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static SEM_HANDLE SEMID_SIF;
static ID FLG_ID_SIF;
unsigned int nvt_sif_debug_level = NVT_DBG_WRN;

#endif

#if defined(__FREERTOS)
irqreturn_t sif_isr(int irq, void *devid);
#else
void sif_isr(void);
#endif

// Set register base address
REGDEF_SETIOBASE(IOADDR_SIF_REG_BASE);

// Static function declaration
static void     sif_setMode(SIF_CH Ch, SIF_MODE Mode);
static void     sif_setBusClock(SIF_CH Ch, UINT32 uiBusClock);
static void     sif_setSEND(SIF_CH Ch, UINT32 uiSEND);
static void     sif_setSENS(SIF_CH Ch, UINT32 uiSENS);
static void     sif_setSENH(SIF_CH Ch, UINT32 uiSENH);
static void     sif_setTranLength(SIF_CH Ch, UINT32 uiTranLength);
static void     sif_setShiftDIR(SIF_CH Ch, SIF_DIR Direction);
static void     sif_setDelay(SIF_CH Ch, UINT32 uiDelay);
static void     sif_setBurstDelay(SIF_CH Ch, UINT32 uiBurstDelay);
static void     sif_setBurstIntval1(SIF_CH Ch, UINT32 uiIntVal);
static void     sif_setBurstIntval2(SIF_CH Ch, UINT32 uiIntVal);
static void     sif_setBurstSRC(SIF_CH Ch, SIF_VD_SOURCE uiIntVal);

static UINT32   sif_getMode(SIF_CH Ch);
static UINT32   sif_getBusClockRegister(SIF_CH Ch);
static UINT32   sif_getBusClock(SIF_CH Ch);
static UINT32   sif_getSENDCount(SIF_CH Ch);
static UINT32   sif_getSEND(SIF_CH Ch);
static UINT32   sif_getSENSRegister(SIF_CH Ch);
static UINT32   sif_getSENS(SIF_CH Ch);
static UINT32   sif_getSENHRegister(SIF_CH Ch);
static UINT32   sif_getSENH(SIF_CH Ch);
static UINT32   sif_getTranLength(SIF_CH Ch);
static UINT32   sif_getShiftDIR(SIF_CH Ch);
static UINT32   sif_getExtraDelay(SIF_CH Ch);
static UINT32   sif_getDelay(SIF_CH Ch);
static UINT32   sif_getBurstDelay(SIF_CH Ch);
static UINT32   sif_getBurstIntval1(SIF_CH Ch);
static UINT32   sif_getBurstIntval2(SIF_CH Ch);
static UINT32   sif_getBurstSRC(SIF_CH Ch);

// Open status (bit-wise)
static UINT32   uiSIFOpenStatus     = 0;

// "Data empty" flag pattern
static const FLGPTN     SIFFlagPtnEmpty[SIF_TOTAL_CHANNEL] = {
	FLGPTN_SIFEMPTY0,
	FLGPTN_SIFEMPTY1,
	FLGPTN_SIFEMPTY2,
	FLGPTN_SIFEMPTY3,
};

// "Transmission end" flag pattern
static const FLGPTN     SIFFlagPtnEnd[SIF_TOTAL_CHANNEL] = {
	FLGPTN_SIF0,
	FLGPTN_SIF1,
	FLGPTN_SIF2,
	FLGPTN_SIF3,

};

// "DMA Transmission end" flag pattern
static const FLGPTN     SIFDMAFlagPtnEnd[SIF_TOTAL_CHANNEL] = {
	FLGPTN_SIFDMA0,
	FLGPTN_SIFDMA1,
};

// Data port selection
static const UINT32     uiSIFDPSel[SIF_TOTAL_CHANNEL] = {
	SIF_DPSEL_CH0,
	SIF_DPSEL_CH1,
	SIF_DPSEL_CH2,
	SIF_DPSEL_CH3

};

#ifndef __KERNEL__
#if !defined(__FREERTOS)
// Pinmux function group
static const PINMUX_FUNC_ID_DRV SIFPinmuxID[SIF_TOTAL_CHANNEL] = {
	PINMUX_FUNC_ID_SIF_0,
	PINMUX_FUNC_ID_SIF_1,
	PINMUX_FUNC_ID_SIF_2,
	PINMUX_FUNC_ID_SIF_3
};
#endif
#endif

// (DMA) Continuous mode
static SIF_CONTMODE     SIFContMode[SIF_TOTAL_CHANNEL] = {
	SIF_CONTMODE_OFF,
	SIF_CONTMODE_OFF
};

// (DMA) Continuous mode address bits
static UINT32           uiSIFContAddrBits[SIF_TOTAL_CHANNEL] = {
	SIF_CONTADDRBITS_DEFAULT,
	SIF_CONTADDRBITS_DEFAULT
};

// (PIO) Remaining bits
static UINT32           uiSIFRemainingBits[SIF_TOTAL_CHANNEL] = {
	0,
	0,
	0,
	0
};

// Auto pinmux
static BOOL             bSIFAutoPinmux[SIF_TOTAL_CHANNEL] = {
	FALSE,
	FALSE,
	FALSE,
	FALSE
};

// (DMA) Delay mode
static SIF_DMA_DELAY     SIFDelayMode[SIF_TOTAL_CHANNEL] = {
	SIF_DMA_DELAY_OFF,
	SIF_DMA_DELAY_OFF
};

#if defined(_NVT_EMULATION_)
// Store interrupt status for emulation checking
static UINT32           uiSIFIntStatus;
#endif

/*
    Configure SIF mode

    Configure SIF mode.

    @param[in] Ch           SIF channel that will be configured
    @param[in] Mode         SIF mode
    @return void
*/
static void sif_setMode(SIF_CH Ch, SIF_MODE Mode)
{
	unsigned long      flags;
	RCW_DEF(SIF_MODE_REG);

	// Enter critical section
	loc_cpu(flags);

	// Set register
	RCW_LD(SIF_MODE_REG);
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_OF(SIF_MODE_REG).Mode_Ch0 = Mode;
		break;

	case SIF_CH1:
		RCW_OF(SIF_MODE_REG).Mode_Ch1 = Mode;
		break;

	case SIF_CH2:
		RCW_OF(SIF_MODE_REG).Mode_Ch2 = Mode;
		break;

	case SIF_CH3:
		RCW_OF(SIF_MODE_REG).Mode_Ch3 = Mode;
		break;

	}
	RCW_ST(SIF_MODE_REG);

	// Leave critical section
	unl_cpu(flags);

	// When mode is SIF_MODE_08 ~ SIF_MODE_15, SIF delay between successive
	// transmission will be related to SENS. SENS is not valid in these modes,
	// so we can set SENS to zero to remove the relationship.
	// Is this a BUG? Ask DE to fix this issue in next IC if possible
	if (Mode >= SIF_MODE_08) {
		sif_setSENS(Ch, 0);
	}
}

/*
    Configure SIF bus clock

    Configure SIF bus clock.

    @param[in] Ch           SIF channel that will be configured
    @param[in] Mode         Desired bus clock (Unit: Hz)
    @return void
*/
static void sif_setBusClock(SIF_CH Ch, UINT32 uiBusClock)
{
	unsigned long      flags;
	RCW_DEF(SIF_CLKCTRL0_REG);
	UINT32  uiDivider;

	// Calculate divider
	uiDivider = SIF_DIVIDER_MIN;
	while (1) {
		if (((SIF_SOURCE_CLOCK / ((uiDivider + 1) << 1)) <= uiBusClock) || (uiDivider == SIF_DIVIDER_MAX)) {
			break;
		}

		uiDivider++;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_CLKCTRL0_REG);
		RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch0 = uiDivider;
		RCW_ST(SIF_CLKCTRL0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_CLKCTRL0_REG);
		RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch1 = uiDivider;
		RCW_ST(SIF_CLKCTRL0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_CLKCTRL0_REG);
		RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch2 = uiDivider;
		RCW_ST(SIF_CLKCTRL0_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_CLKCTRL0_REG);
		RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch3 = uiDivider;
		RCW_ST(SIF_CLKCTRL0_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF SEN duration

    Configure SIF SEN duration.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiSEND       Desired SEN duration (Unit: ns)
    @return void
*/
static void sif_setSEND(SIF_CH Ch, UINT32 uiSEND)
{
	unsigned long      flags;
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);
	UINT32  uiValue;
#if defined __UITRON || defined __ECOS
	FLOAT   fPeriod;

	// Get half bus clock period in ns
	fPeriod = ((FLOAT)1000000 / sif_getBusClock(Ch)) * 500;
#else
    UINT32 fPeriod;    // Get half bus clock period in ns
    fPeriod = (1000000000 / sif_getBusClock(Ch)) / 2;
#endif

	// Calculate duration setting
	uiValue = SIF_SEND_MIN;
	while (1) {
		if (((UINT32)(uiValue * fPeriod) >= uiSEND) || (uiValue == SIF_SEND_MAX)) {
			break;
		}

		uiValue++;
	}

	// Convert to real register setting
	// 16       --> Register 0
	// 1 ~ 15   --> Register 1 ~ 15
	if (uiValue == SIF_SEND_MAX) {
		uiValue = 0;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SEND_Ch0 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SEND_Ch1 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SEND_Ch2 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SEND_Ch3 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF SEN setup time

    Configure SIF SEN setup time.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiSENS       Desired SEN setup time (Unit: ns)
    @return void
*/
static void sif_setSENS(SIF_CH Ch, UINT32 uiSENS)
{
	unsigned long      flags;
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);
	UINT32  uiValue;
#if defined __UITRON || defined __ECOS
	FLOAT   fPeriod;

	// Get half bus clock period in ns
	fPeriod = ((FLOAT)1000000 / sif_getBusClock(Ch)) * 500;
#else
    UINT32 fPeriod;    // Get half bus clock period in ns
    fPeriod = (1000000000 / sif_getBusClock(Ch)) / 2;
#endif

	// Calculate setup time setting
	uiValue = SIF_SENS_MIN;
	while (1) {
		if (((UINT32)(uiValue * fPeriod) >= uiSENS) || (uiValue == SIF_SENS_MAX)) {
			break;
		}

		uiValue++;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SENS_Ch0 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SENS_Ch1 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SENS_Ch2 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SENS_Ch3 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF SEN hold time

    Configure SIF SEN hold time.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiSENH       Desired SEN hold time (Unit: ns)
    @return void
*/
static void sif_setSENH(SIF_CH Ch, UINT32 uiSENH)
{
	unsigned long      flags;
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);
	UINT32  uiValue;
#if defined __UITRON || defined __ECOS
	FLOAT   fPeriod;

	// Get half bus clock period in ns
	fPeriod = ((FLOAT)1000000 / sif_getBusClock(Ch)) * 500;
#else
    UINT32 fPeriod;    // Get half bus clock period in ns
    fPeriod = (1000000000 / sif_getBusClock(Ch)) / 2;
#endif

	// Calculate setup time setting
	uiValue = SIF_SENH_MIN;
	while (1) {
		if (((UINT32)(uiValue * fPeriod) >= uiSENH) || (uiValue == SIF_SENH_MAX)) {
			break;
		}

		uiValue++;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SENH_Ch0 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		RCW_OF(SIF_TIMING0_REG).SENH_Ch1 = uiValue;
		RCW_ST(SIF_TIMING0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SENH_Ch2 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		RCW_OF(SIF_TIMING1_REG).SENH_Ch3 = uiValue;
		RCW_ST(SIF_TIMING1_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF transmission length

    Configure SIF transmission length.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiTranLength Transfer length
    @return void
*/
static void sif_setTranLength(SIF_CH Ch, UINT32 uiTranLength)
{
	unsigned long      flags;
	RCW_DEF(SIF_TXSIZE0_REG);

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TXSIZE0_REG);
		RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch0 = uiTranLength;
		RCW_ST(SIF_TXSIZE0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_TXSIZE0_REG);
		RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch1 = uiTranLength;
		RCW_ST(SIF_TXSIZE0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_TXSIZE0_REG);
		RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch2 = uiTranLength;
		RCW_ST(SIF_TXSIZE0_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_TXSIZE0_REG);
		RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch3 = uiTranLength;
		RCW_ST(SIF_TXSIZE0_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF bit shift out direction

    Configure SIF bit shift out direction.

    @param[in] Ch           SIF channel that will be configured
    @param[in] Direction    Shift out direction
    @return void
*/
static void sif_setShiftDIR(SIF_CH Ch, SIF_DIR Direction)
{
	unsigned long      flags;
	RCW_DEF(SIF_CONF1_REG);

	// Enter critical section
	loc_cpu(flags);

	// Set register
	RCW_LD(SIF_CONF1_REG);
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_OF(SIF_CONF1_REG).Dir_Ch0 = Direction;
		break;

	case SIF_CH1:
		RCW_OF(SIF_CONF1_REG).Dir_Ch1 = Direction;
		break;

	case SIF_CH2:
		RCW_OF(SIF_CONF1_REG).Dir_Ch2 = Direction;
		break;

	case SIF_CH3:
		RCW_OF(SIF_CONF1_REG).Dir_Ch3 = Direction;
		break;

	}
	RCW_ST(SIF_CONF1_REG);

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF delay between successive transmission

    Configure SIF delay between successive transmission.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiDelay      Delay between successive transmission (Unit: ns)
    @return void
*/
static void sif_setDelay(SIF_CH Ch, UINT32 uiDelay)
{
	unsigned long      flags;
	RCW_DEF(SIF_DELAY0_REG);
	UINT32  uiValue, uiExtra;
#if defined __UITRON || defined __ECOS
	FLOAT   fPeriod;

	// Get source clock period in ns
	fPeriod = (FLOAT)1000000000 / SIF_SOURCE_CLOCK;
#else
    UINT32 fPeriod;    // Get half bus clock period in ns
    fPeriod = 1000000000 / SIF_SOURCE_CLOCK;
#endif

	// PIO mode will send data one SIF clock faster than DMA mode.
	// The formula to calculate delay time is based on DMA mode.
	// To make sure the delay time is enought for both mode,
	// we will add one SIF clock time (about 10 ns)to the delay value
	uiDelay += 10;

	// Get extra delay
	uiExtra = sif_getExtraDelay(Ch);

	// Calculate delay setting
	uiValue = SIF_DELAY_MIN;
	while (1) {
		if (((UINT32)((uiExtra + (uiValue << 5)) * fPeriod) >= uiDelay) || (uiValue == SIF_DELAY_MAX)) {
			break;
		}

		uiValue++;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DELAY0_REG);
		RCW_OF(SIF_DELAY0_REG).Delay_Ch0 = uiValue;
		RCW_ST(SIF_DELAY0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DELAY0_REG);
		RCW_OF(SIF_DELAY0_REG).Delay_Ch1 = uiValue;
		RCW_ST(SIF_DELAY0_REG);
		break;

	case SIF_CH2:
		RCW_LD(SIF_DELAY0_REG);
		RCW_OF(SIF_DELAY0_REG).Delay_Ch2 = uiValue;
		RCW_ST(SIF_DELAY0_REG);
		break;

	case SIF_CH3:
		RCW_LD(SIF_DELAY0_REG);
		RCW_OF(SIF_DELAY0_REG).Delay_Ch3 = uiValue;
		RCW_ST(SIF_DELAY0_REG);
		break;

	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF burst delay after SIE VD signal

    Configure SIF burst delay after SIE VD signal.

    @param[in] Ch           SIF channel that will be configured
    @param[in] uiBurstDelay Delay between VD signal and burst transmission (Unit: ns)
    @return void
*/
static void sif_setBurstDelay(SIF_CH Ch, UINT32 uiBurstDelay)
{
	unsigned long      flags;
	RCW_DEF(SIF_DMA0_CTRL3_REG);
	RCW_DEF(SIF_DMA1_CTRL3_REG);
	UINT32  uiValue = SIF_DELAY_MIN;
	UINT32  fPeriod;

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", Ch);
		return;
	}

	// Get source clock period in ns
	fPeriod = 1000000000 / SIF_SOURCE_CLOCK;

	// Calculate the minimum unit for different clk sources.
	// The delay will be rounded up if less than 1 unit.
	if (uiBurstDelay != 0) {
		if (uiBurstDelay < fPeriod) {
			uiBurstDelay = fPeriod;
		}

		while (1) {
			if ((uiBurstDelay < fPeriod) || (uiValue == SIF_BURST_DELAY_MAX)) {
				break;
			}

			uiValue++;
			uiBurstDelay -= fPeriod;
		}
	} else {
		uiValue = 0;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL3_REG);
		RCW_OF(SIF_DMA0_CTRL3_REG).DMA_Burst_Delay = uiValue;
		RCW_ST(SIF_DMA0_CTRL3_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL3_REG);
		RCW_OF(SIF_DMA1_CTRL3_REG).DMA_Burst_Delay = uiValue;
		RCW_ST(SIF_DMA1_CTRL3_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);


}

/*
    Configure SIF burst interval between 1st and 2nd transmission

    Configure SIF burst interval between 1st and 2nd transmission

    @param[in] Ch           SIF channel that will be configured
    @param[in] Interval VD numbers between transimissions.
    @return void
*/
static void sif_setBurstIntval1(SIF_CH Ch, UINT32 uiIntVal)
{
	unsigned long      flags;
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Burst_IntVal1 = uiIntVal;
		RCW_ST(SIF_DMA0_CTRL0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Burst_IntVal1 = uiIntVal;
		RCW_ST(SIF_DMA1_CTRL0_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF burst interval between 2nd and 3rd transmission

    Configure SIF burst interval between 2nd and 3rd transmission.

    @param[in] Ch           SIF channel that will be configured
    @param[in] Interval VD numbers between transimissions.
    @return void
*/
static void sif_setBurstIntval2(SIF_CH Ch, UINT32 uiIntVal)
{
	unsigned long      flags;
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Burst_IntVal2 = uiIntVal;
		RCW_ST(SIF_DMA0_CTRL0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Burst_IntVal2 = uiIntVal;
		RCW_ST(SIF_DMA1_CTRL0_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    Configure SIF VD signal source

    Configure SIF VD signal source.

    @param[in] Ch           SIF channel that will be configured
    @param[in] Input source of SIE VD channel.
    @return void
*/
static void sif_setBurstSRC(SIF_CH Ch, SIF_VD_SOURCE uiIntVal)
{
	unsigned long      flags;
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Sync_Src = uiIntVal;
		RCW_ST(SIF_DMA0_CTRL0_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Sync_Src = uiIntVal;
		RCW_ST(SIF_DMA1_CTRL0_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}


/*
    Configure SIF DMA Delay Tag

    Configure SIF DMA Delay Tag

    @param[in] Ch           SIF channel that will be configured
    @param[in] Enable / Disable DMA delay
    @return void
*/
static void sif_setDMADelayTag(SIF_CH Ch, UINT16 uiTag)
{
	unsigned long      flags;
	RCW_DEF(SIF_DMA0_DLY_REG);
	RCW_DEF(SIF_DMA1_DLY_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Enter critical section
	loc_cpu(flags);

	// Set register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_DLY_REG);
		RCW_OF(SIF_DMA0_DLY_REG).SIF_DLY_TAG = uiTag;
		RCW_ST(SIF_DMA0_DLY_REG);
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_DLY_REG);
		RCW_OF(SIF_DMA1_DLY_REG).SIF_DLY_TAG = uiTag;
		RCW_ST(SIF_DMA1_DLY_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);

}

/*
    Get SIF mode

    Get SIF mode.

    @param[in] Ch           SIF channel that will be probed

    @return SIF mode
*/
static UINT32 sif_getMode(SIF_CH Ch)
{
	RCW_DEF(SIF_MODE_REG);

	// Get mode from register
	RCW_LD(SIF_MODE_REG);
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		return RCW_OF(SIF_MODE_REG).Mode_Ch0;

	case SIF_CH1:
		return RCW_OF(SIF_MODE_REG).Mode_Ch1;

	case SIF_CH2:
		return RCW_OF(SIF_MODE_REG).Mode_Ch2;

	case SIF_CH3:
		return RCW_OF(SIF_MODE_REG).Mode_Ch3;

	}
}

/*
    Get SIF bus clock divider register setting

    Get SIF bus clock divider register setting.

    @param[in] Ch           SIF channel that will be probed

    @return Bus clock divider register
*/
static UINT32 sif_getBusClockRegister(SIF_CH Ch)
{
	RCW_DEF(SIF_CLKCTRL0_REG);

	// Get divider from register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_CLKCTRL0_REG);
		return (RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch0);

	case SIF_CH1:
		RCW_LD(SIF_CLKCTRL0_REG);
		return (RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch1);

	case SIF_CH2:
		RCW_LD(SIF_CLKCTRL0_REG);
		return (RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch2);

	case SIF_CH3:
		RCW_LD(SIF_CLKCTRL0_REG);
		return (RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch3);
	}
}

/*
    Get SIF bus clock

    Get SIF bus clock.

    @param[in] Ch           SIF channel that will be probed

    @return Bus clock (Unit: Hz)
*/
static UINT32 sif_getBusClock(SIF_CH Ch)
{
	return (SIF_SOURCE_CLOCK / ((sif_getBusClockRegister(Ch) + 1) << 1));
}

/*
    Get SIF SEN duration count

    Get SIF SEN duration count.

    @param[in] Ch           SIF channel that will be probed

    @return SEN duration count
*/
static UINT32 sif_getSENDCount(SIF_CH Ch)
{
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);
	UINT32  uiValue;

	// Get SEND from register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		uiValue = RCW_OF(SIF_TIMING0_REG).SEND_Ch0;
		break;

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		uiValue = RCW_OF(SIF_TIMING0_REG).SEND_Ch1;
		break;

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		uiValue = RCW_OF(SIF_TIMING1_REG).SEND_Ch2;
		break;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		uiValue = RCW_OF(SIF_TIMING1_REG).SEND_Ch3;
		break;
	}

	// Convert from register to real setting
	// Register 0       --> 16
	// Register 1 ~ 15  --> 1 ~ 15
	if (uiValue == 0) {
		uiValue = SIF_SEND_MAX;
	}

	return uiValue;
}

/*
    Get SIF SEN duration

    Get SIF SEN duration.

    @param[in] Ch           SIF channel that will be probed

    @return SEN duration (Unit: ns)
*/
static UINT32 sif_getSEND(SIF_CH Ch)
{
#if defined __UITRON || defined __ECOS
	return (UINT32)(((FLOAT)500000000 / sif_getBusClock(Ch)) * sif_getSENDCount(Ch));
#else
    return (UINT32)((500000000 / sif_getBusClock(Ch)) * sif_getSENDCount(Ch));
#endif
}

/*
    Get SIF SEN setup time register setting

    Get SIF SEN setup time register setting.

    @param[in] Ch           SIF channel that will be probed

    @return SEN setup time (Unit: ns)
*/
static UINT32 sif_getSENSRegister(SIF_CH Ch)
{
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);

	// Get SENS from register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		return RCW_OF(SIF_TIMING0_REG).SENS_Ch0;

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		return RCW_OF(SIF_TIMING0_REG).SENS_Ch1;

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		return RCW_OF(SIF_TIMING1_REG).SENS_Ch2;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		return RCW_OF(SIF_TIMING1_REG).SENS_Ch3;
	}
}

/*
    Get SIF SEN setup time

    Get SIF SEN setup time.

    @param[in] Ch           SIF channel that will be probed

    @return SEN setup time (Unit: ns)
*/
static UINT32 sif_getSENS(SIF_CH Ch)
{
#if defined __UITRON || defined __ECOS
	return (UINT32)(((FLOAT)500000000 / sif_getBusClock(Ch)) * sif_getSENSRegister(Ch));
#else
    return (UINT32)((500000000 / sif_getBusClock(Ch)) * sif_getSENSRegister(Ch));
#endif
}

/*
    Get SIF SEN hold time register setting

    Get SIF SEN hold time register setting.

    @param[in] Ch           SIF channel that will be probed

    @return SEN hold time register
*/
static UINT32 sif_getSENHRegister(SIF_CH Ch)
{
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);

	// Get SENH from register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TIMING0_REG);
		return (RCW_OF(SIF_TIMING0_REG).SENH_Ch0);

	case SIF_CH1:
		RCW_LD(SIF_TIMING0_REG);
		return (RCW_OF(SIF_TIMING0_REG).SENH_Ch1);

	case SIF_CH2:
		RCW_LD(SIF_TIMING1_REG);
		return RCW_OF(SIF_TIMING1_REG).SENH_Ch2;

	case SIF_CH3:
		RCW_LD(SIF_TIMING1_REG);
		return RCW_OF(SIF_TIMING1_REG).SENH_Ch3;

	}
}

/*
    Get SIF SEN hold time

    Get SIF SEN hold time.

    @param[in] Ch           SIF channel that will be probed

    @return SEN hold time (Unit: ns)
*/
static UINT32 sif_getSENH(SIF_CH Ch)
{
#if defined __UITRON || defined __ECOS
	return (UINT32)(((FLOAT)500000000 / sif_getBusClock(Ch)) * sif_getSENHRegister(Ch));
#else
    return (UINT32)((500000000 / sif_getBusClock(Ch)) * sif_getSENHRegister(Ch));
#endif
}

/*
    Get SIF transmission length

    Get SIF transmission length.

    @param[in] Ch           SIF channel that will be probed

    @return Transmission length
*/
static UINT32 sif_getTranLength(SIF_CH Ch)
{
	RCW_DEF(SIF_TXSIZE0_REG);

	// Get transmission length from register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_TXSIZE0_REG);
		return RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch0;

	case SIF_CH1:
		RCW_LD(SIF_TXSIZE0_REG);
		return RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch1;

	case SIF_CH2:
		RCW_LD(SIF_TXSIZE0_REG);
		return RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch2;
		break;

	case SIF_CH3:
		RCW_LD(SIF_TXSIZE0_REG);
		return RCW_OF(SIF_TXSIZE0_REG).TxSize_Ch3;
		break;

	}
}

/*
    Get SIF bit shift out direction

    Get SIF bit shift out direction.

    @param[in] Ch           SIF channel that will be probed

    @return Shift out direction
        - @b SIF_DIR_LSB    : LSb is shifted out first
        - @b SIF_DIR_MSB    : MSb is shifted out first
*/
static UINT32 sif_getShiftDIR(SIF_CH Ch)
{
	RCW_DEF(SIF_CONF1_REG);

	// Get shift direction from register
	RCW_LD(SIF_CONF1_REG);
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		return RCW_OF(SIF_CONF1_REG).Dir_Ch0;

	case SIF_CH1:
		return RCW_OF(SIF_CONF1_REG).Dir_Ch1;

	case SIF_CH2:
		return RCW_OF(SIF_CONF1_REG).Dir_Ch2;

	case SIF_CH3:
		return RCW_OF(SIF_CONF1_REG).Dir_Ch3;

	}
}

/*
    Get SIF extra delay by controller's design

    Get SIF extra delay by controller's design.

    @param[in] Ch           SIF channel that will be probed

    @return Extra delay
*/
static UINT32 sif_getExtraDelay(SIF_CH Ch)
{
	UINT32  uiCheck, uiMore;

	// Formula of delay time is:
	// When SIF mode is 00 ~ 07
	//   if SENH is odd
	//     (clock divider + 1) + 4 + delay * 32
	//   else
	//     (clock divider + 1) * 2 + 4 + delay * 32
	// When SIF mode is 08 ~ 15
	//   if (SENH + SEND) is odd
	//     (clock divider + 1) + 4 + delay * 32 + (clock divider + 1) * SENS
	//   else
	//     (clock divider + 1) * 2 + 4 + delay * 32 + (clock divider + 1) * SENS
	//
	// We will set SENS to zero when upper layer set mode to 08 ~ 15

	uiCheck = sif_getSENHRegister(Ch);

	// Mode 08 ~ Mode 15
	if (sif_getMode(Ch) >= SIF_MODE_08) {
		uiCheck += sif_getSENDCount(Ch);
		uiMore = (sif_getBusClockRegister(Ch) + 1) * sif_getSENSRegister(Ch);
	} else {
		uiMore = 0;
	}

	// Odd
	if (uiCheck & 0x1) {
		return (((sif_getBusClockRegister(Ch) + 1) + 4) + uiMore);
	}
	// Even
	else {
		return ((((sif_getBusClockRegister(Ch) + 1) << 1) + 4) + uiMore);
	}
}

/*
    Get SIF delay between successive transmission

    Get SIF delay between successive transmission.

    @param[in] Ch           SIF channel that will be probed

    @return Delay between successive transmission (Unit: ns)
*/
static UINT32 sif_getDelay(SIF_CH Ch)
{
	RCW_DEF(SIF_DELAY0_REG);
	UINT32  uiValue;

	// Get register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DELAY0_REG);
		uiValue = RCW_OF(SIF_DELAY0_REG).Delay_Ch0;
		break;

	case SIF_CH1:
		RCW_LD(SIF_DELAY0_REG);
		uiValue = RCW_OF(SIF_DELAY0_REG).Delay_Ch1;
		break;

	case SIF_CH2:
		RCW_LD(SIF_DELAY0_REG);
		uiValue = RCW_OF(SIF_DELAY0_REG).Delay_Ch2;
		break;

	case SIF_CH3:
		RCW_LD(SIF_DELAY0_REG);
		uiValue = RCW_OF(SIF_DELAY0_REG).Delay_Ch3;
		break;

	}

#if defined __UITRON || defined __ECOS
	return (UINT32)(((FLOAT)1000000000 / SIF_SOURCE_CLOCK) * ((uiValue << 5) + sif_getExtraDelay(Ch)));
#else
    return (UINT32)((1000000000 / SIF_SOURCE_CLOCK) * ((uiValue << 5) + sif_getExtraDelay(Ch)));
#endif

}

/*
    Get SIF burst delay after SIE VD signal

    Get SIF burst delay after SIE VD signal.

    @param[in] Ch           SIF channel that will be configured
    @return uiBurstDelay Delay between VD signal and burst transmission (Unit: ns)
*/
static UINT32 sif_getBurstDelay(SIF_CH Ch)
{
	RCW_DEF(SIF_DMA0_CTRL3_REG);
	RCW_DEF(SIF_DMA1_CTRL3_REG);
	UINT32  uiValue;
#if defined __UITRON || defined __ECOS
	FLOAT period;
#endif

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return 0;
	}

	// Get register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL3_REG);
		uiValue = RCW_OF(SIF_DMA0_CTRL3_REG).DMA_Burst_Delay;
		break;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL3_REG);
		uiValue = RCW_OF(SIF_DMA1_CTRL3_REG).DMA_Burst_Delay;
		break;
	}

#if defined __UITRON || defined __ECOS
	period = (FLOAT)1000000000 / SIF_SOURCE_CLOCK;

	return (UINT32)(period * uiValue);
#else
	return (UINT32)(1000000000 / SIF_SOURCE_CLOCK * uiValue);
#endif
}

/*
    Get SIF burst interval between 1st and 2nd transmission

    Get SIF burst interval between 1st and 2nd transmission

    @param[in] Ch           SIF channel that will be configured
    @return burst interval
*/
static UINT32 sif_getBurstIntval1(SIF_CH Ch)
{
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return 0;
	}

	// Get register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		return RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Burst_IntVal1;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		return RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Burst_IntVal1;
	}
}

/*
    Get SIF burst interval between 2nd and 3rd transmission

    Get SIF burst interval between 2nd and 3rd transmission

    @param[in] Ch           SIF channel that will be configured
    @return burst interval
*/
static UINT32 sif_getBurstIntval2(SIF_CH Ch)
{
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return 0;
	}

	// Get register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		return RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Burst_IntVal2;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		return RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Burst_IntVal2;
	}
}

/*
    Get SIF VD signal source

    Get SIF VD signal source

    @param[in] Ch           SIF channel that will be configured
    @return SIE VD channel
*/
static UINT32 sif_getBurstSRC(SIF_CH Ch)
{
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return 0;
	}

	// Get register
	switch (Ch) {
	// The channel validation is done in the function that will
	// call this API. We can skip the checking here.
	default:
//    case SIF_CH0:
		RCW_LD(SIF_DMA0_CTRL0_REG);
		return RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Sync_Src;

	case SIF_CH1:
		RCW_LD(SIF_DMA1_CTRL0_REG);
		return RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Sync_Src;
	}
}

/*
    SIF interrupt service routine

    SIF interrupt service routine.

    @return void
*/
#if defined(__FREERTOS)
irqreturn_t sif_isr(int irq, void *devid)
#else
void sif_isr(void)
#endif
{
	RCW_DEF(SIF_INTSTS0_REG);
	FLGPTN  FlagPtn = 0;
	UINT32  i;

	// Read interrupt status register
	RCW_LD(SIF_INTSTS0_REG);

	if (RCW_VAL(SIF_INTSTS0_REG) != 0) {
#if defined(_NVT_EMULATION_)
		// Store interrupt status for emulation checking
		uiSIFIntStatus = RCW_VAL(SIF_INTSTS0_REG);
#endif

		// Clear interrupt status
		RCW_ST(SIF_INTSTS0_REG);

		// DMA0 end
		if (RCW_OF(SIF_INTSTS0_REG).DMAEnd_DMA0 != 0) {
			// Set flag pattern
			FlagPtn |= (SIFDMAFlagPtnEnd[0] | SIFFlagPtnEnd[0]);
		}

		// DMA1 end
		if (RCW_OF(SIF_INTSTS0_REG).DMAEnd_DMA1 != 0) {
			// Set flag pattern
			FlagPtn |= (SIFDMAFlagPtnEnd[1] | SIFFlagPtnEnd[1]);
		}

		// Data empty or transfer end
		for (i = 0; i < SIF_TOTAL_CHANNEL; i++) {
			// Data empty
			if (RCW_VAL(SIF_INTSTS0_REG) & SIF_INT_EMPTY(i)) {
				FlagPtn |= SIFFlagPtnEmpty[i];
			}
			// Transfer end
			if (RCW_VAL(SIF_INTSTS0_REG) & SIF_INT_END(i)) {
				// Set flag pattern
				FlagPtn |= SIFFlagPtnEnd[i];
			}
		}
		iset_flg(FLG_ID_SIF, FlagPtn);
	}

#if defined(__FREERTOS)
		return IRQ_HANDLED;
#endif

}

#if defined(__FREERTOS)
irqreturn_t sif_isr_core2(int irq, void *devid)
#else
void sif_isr_core2(void)
#endif
{
	RCW_DEF(SIF_INTSTS1_REG);
	FLGPTN  FlagPtn = 0;
	UINT32  i;

	//DBG_ERR("in core2 sif isr core2\r\n");

	// Read interrupt status register
	RCW_LD(SIF_INTSTS1_REG);

	if (RCW_VAL(SIF_INTSTS1_REG) != 0) {
#if defined(_NVT_EMULATION_)
		// Store interrupt status for emulation checking
		uiSIFIntStatus = RCW_VAL(SIF_INTSTS1_REG);
#endif

		// Clear interrupt status
		RCW_ST(SIF_INTSTS1_REG);

		// DMA0 end
		if (RCW_OF(SIF_INTSTS1_REG).DMAEnd_DMA0_core2 != 0) {
			// Set flag pattern
			FlagPtn |= (SIFDMAFlagPtnEnd[0] | SIFFlagPtnEnd[0]);
			//DBG_ERR("DMA0 end\r\n");
		}

		// DMA1 end
		if (RCW_OF(SIF_INTSTS1_REG).DMAEnd_DMA1_core2 != 0) {
			// Set flag pattern
			FlagPtn |= (SIFDMAFlagPtnEnd[1] | SIFFlagPtnEnd[1]);
			//DBG_ERR("DMA1 end\r\n");
		}

		// Data empty or transfer end
		for (i = 0; i < SIF_TOTAL_CHANNEL; i++) {
			// Data empty
			if (RCW_VAL(SIF_INTSTS1_REG) & SIF_INT_EMPTY(i)) {
				FlagPtn |= SIFFlagPtnEmpty[i];
				//DBG_ERR("Data empty\r\n");
			}
			// Transfer end
			if (RCW_VAL(SIF_INTSTS1_REG) & SIF_INT_END(i)) {
				// Set flag pattern
				FlagPtn |= SIFFlagPtnEnd[i];
				//DBG_ERR("Transfer end\r\n");
			}
		}
		iset_flg(FLG_ID_SIF, FlagPtn);
	}

#if defined(__FREERTOS)
		return IRQ_HANDLED;
#endif

}


#if defined __KERNEL__ || defined(__FREERTOS)
void sif_init(UINT32 pmodule_info)
{
    OS_CONFIG_FLAG(FLG_ID_SIF);
    SEM_CREATE(SEMID_SIF,1);
}

void sif_exit(void)
{
    rel_flg(FLG_ID_SIF);
    SEM_DESTROY(SEMID_SIF);
}
#endif


/**
    @addtogroup mIDrvIO_SIF
*/
//@{

/**
    Open SIF channel

    This function should be called before calling any other functions.

    @param[in] Ch       SIF channel number
    @return Open status
        - @b E_SYS      The channel is already opened
        - @b E_OK       Everything is oK
*/
ER sif_open(SIF_CH Ch)
{
	RCW_DEF(SIF_INTEN0_REG);
	RCW_DEF(SIF_INTEN1_REG);
	RCW_DEF(SIF_INT_DES_REG);
	UINT32 i;

#if defined(__FREERTOS)
	static BOOL rtos_init = 0;

	if (!rtos_init) {
		rtos_init = 1;
		cre_flg(&FLG_ID_SIF, NULL, "FLG_ID_SIF");
		vos_sem_create(&SEMID_SIF, 1, "SEMID_SIF");
	}
#endif

	// The channel is already opened
	if (uiSIFOpenStatus & (1 << Ch)) {
		DBG_ERR("SIF_CH%d is already opened!\r\n", (int)Ch);
		return E_SYS;
	}

	// Wait semaphore
	SEM_WAIT(SEMID_SIF);


	// The channel is opened now
	uiSIFOpenStatus |= ((UINT32)1 << Ch);

	// All SIF channels are closed
	if (uiSIFOpenStatus == ((UINT32)1 << Ch)) {
		// Reset remain bits
		for (i = 0; i < SIF_TOTAL_CHANNEL; i++) {
			uiSIFRemainingBits[i] = 0;
		}

		// Enable interrupt 0 for data empty, data end and DMA end
		RCW_VAL(SIF_INTEN0_REG) = 0;
		RCW_OF(SIF_INTEN0_REG).Data_Empty_Ch0_IntEn  = ENABLE;
		RCW_OF(SIF_INTEN0_REG).Data_Empty_Ch1_IntEn  = ENABLE;
		RCW_OF(SIF_INTEN0_REG).Data_Empty_Ch2_IntEn  = ENABLE;
		RCW_OF(SIF_INTEN0_REG).Data_Empty_Ch3_IntEn  = ENABLE;
		RCW_OF(SIF_INTEN0_REG).TxEnd_Ch0_IntEn       = ENABLE;
		RCW_OF(SIF_INTEN0_REG).TxEnd_Ch1_IntEn       = ENABLE;
		RCW_OF(SIF_INTEN0_REG).TxEnd_Ch2_IntEn       = ENABLE;
		RCW_OF(SIF_INTEN0_REG).TxEnd_Ch3_IntEn       = ENABLE;
		RCW_OF(SIF_INTEN0_REG).DMAEnd_DMA0_IntEn     = ENABLE;
		RCW_OF(SIF_INTEN0_REG).DMAEnd_DMA1_IntEn     = ENABLE;
		RCW_ST(SIF_INTEN0_REG);

		// Enable interrupt 1 for data empty, data end and DMA end
		RCW_VAL(SIF_INTEN1_REG) = 0;
		RCW_OF(SIF_INTEN1_REG).Data_Empty_Ch0_IntEn_core2  = ENABLE;
		RCW_OF(SIF_INTEN1_REG).Data_Empty_Ch1_IntEn_core2  = ENABLE;
		RCW_OF(SIF_INTEN1_REG).Data_Empty_Ch2_IntEn_core2  = ENABLE;
		RCW_OF(SIF_INTEN1_REG).Data_Empty_Ch3_IntEn_core2  = ENABLE;
		RCW_OF(SIF_INTEN1_REG).TxEnd_Ch0_IntEn_core2       = ENABLE;
		RCW_OF(SIF_INTEN1_REG).TxEnd_Ch1_IntEn_core2       = ENABLE;
		RCW_OF(SIF_INTEN1_REG).TxEnd_Ch2_IntEn_core2       = ENABLE;
		RCW_OF(SIF_INTEN1_REG).TxEnd_Ch3_IntEn_core2       = ENABLE;
		RCW_OF(SIF_INTEN1_REG).DMAEnd_DMA0_IntEn_core2     = ENABLE;
		RCW_OF(SIF_INTEN1_REG).DMAEnd_DMA1_IntEn_core2     = ENABLE;
		RCW_ST(SIF_INTEN1_REG);		

		//
		RCW_VAL(SIF_INT_DES_REG) = 0;
		RCW_OF(SIF_INT_DES_REG).INT_TO_CPU1  = 0xF;
		RCW_OF(SIF_INT_DES_REG).INT_TO_CPU2  = 0xF;
		RCW_ST(SIF_INT_DES_REG);
#ifndef __KERNEL__
		// Enable SIF clock
		pll_enable_clock(SIF_CLK);

		// Enable interrupt
		request_irq(INT_ID_SIF, sif_isr ,IRQF_TRIGGER_HIGH, "sif", 0);
		//request_irq(INT_ID_SIF2, sif_isr_core2 ,IRQF_TRIGGER_HIGH, "sif", 0);

#endif
		// Pre-set the interrupt flag
		set_flg(FLG_ID_SIF, SIF_FLGPTN_END_ALL);
	}
#ifndef __KERNEL__
#if !defined(__FREERTOS)
	// Change pinmux to SIF
	if (bSIFAutoPinmux[Ch] == TRUE) {
		pinmux_setPinmux((PINMUX_FUNC_ID)SIFPinmuxID[Ch], PINMUX_SIF_SEL_ACTIVE);
	}
#endif
#endif
	// Signal semaphore
	SEM_SIGNAL(SEMID_SIF);

	return E_OK;
}


/**
    Close SIF channel

    Release SIF channel and let other application use SIF.

    @param[in] Ch       SIF channel number
    @return Close status
        - @b E_SYS      The channel is already closed
        - @b E_OK       Everything is oK
*/
ER sif_close(SIF_CH Ch)
{
	FLGPTN  FlagPtn;

	// The channel is already closed
	if (uiSIFOpenStatus == 0) {
		DBG_ERR("SIF_CH%d is already closed!\r\n", (int)Ch);
		return E_SYS;
	}

	// Wait semaphore
	SEM_WAIT(SEMID_SIF);

	// The channel is now closed
	uiSIFOpenStatus &= ~(1 << Ch);

#ifndef __KERNEL__
#if !defined(__FREERTOS)
	// Change pinmux to GPIO
	if (bSIFAutoPinmux[Ch] == TRUE) {
		pinmux_setPinmux((PINMUX_FUNC_ID)SIFPinmuxID[Ch], PINMUX_SIF_SEL_INACTIVE);
	}
#endif
#endif

	// There is no more SIF channel is opened
	if (uiSIFOpenStatus == 0) {
		// Wait for transfer done
		wai_flg(&FlagPtn, FLG_ID_SIF, SIF_FLGPTN_END_ALL, TWF_ANDW);

#ifndef __KERNEL__
		// Release interrupt
		//drv_disableInt(DRV_INT_SIF);

		// Disable clock
		pll_disableClock(SIF_CLK);
#endif
	}

#ifndef __KERNEL__
	// Disable interrupt
	free_irq(INT_ID_SIF, 0);
#endif

	// Signal semaphore
	SEM_SIGNAL(SEMID_SIF);

	return E_OK;
}


/**
    Check if SIF channel is opened or not

    Check if SIF channel is opened or not.

    @param[in] Ch       SIF channel number
    @return SIF channel open status
        - @b TRUE   : SIF is opened
        - @b FALSE  : SIF is closed
*/
BOOL sif_isOpened(SIF_CH Ch)
{
	return (((uiSIFOpenStatus & (1 << Ch)) != 0) ? TRUE : FALSE);
}


/**
    Configure SIF

    Configuration for specific SIF channel.

    @param[in] Ch           SIF channel that will be configured
    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value
    @return void
*/
void sif_setConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	RCW_DEF(SIF_STS_REG);

	// Check channel number
	if (Ch >= SIF_TOTAL_CHANNEL) {
		DBG_ERR("Invalid channel!\r\n");
		return;
	}

	// Allow to config SIF's auto pinmux configuration when SIF is not open
	if ((ConfigID != SIF_CONFIG_ID_AUTOPINMUX) && ((uiSIFOpenStatus & ((UINT32)1 << Ch)) == 0)) {
		DBG_ERR("SIF_CH%d is not opened!\r\n", Ch);
		return;
	}

	// Check channel is busy or not
	RCW_LD(SIF_STS_REG);
	if (RCW_VAL(SIF_STS_REG) & SIF_STATUS_CH(Ch)) {
		DBG_ERR("SIF_CH%d is busy!\r\n", Ch);
		return;
	}

	switch (ConfigID) {
	case SIF_CONFIG_ID_BUSMODE:
		// Check parameter
		if (uiConfig > SIF_MODE_15) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = SIF_MODE_15;
		}

		sif_setMode(Ch, (SIF_MODE)uiConfig);
		break;

	case SIF_CONFIG_ID_BUSCLOCK:
		sif_setBusClock(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_SEND:
		sif_setSEND(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_SENS:
		sif_setSENS(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_SENH:
		sif_setSENH(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_LENGTH:
		// Check parameter
		if ((uiConfig < SIF_TRANLEN_MIN) || (uiConfig > SIF_TRANLEN_MAX)) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = SIF_TRANLEN_DEFAULT;
		}

		sif_setTranLength(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_DIR:
		// Check parameter
		if (uiConfig > SIF_DIR_MSB) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = SIF_DIR_LSB;
		}

		sif_setShiftDIR(Ch, (SIF_DIR)uiConfig);
		break;

	case SIF_CONFIG_ID_CONTMODE:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}

		if ((SIF_CONTMODE)uiConfig > SIF_CONTMODE_ON) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = (UINT32)SIF_CONTMODE_OFF;
		}

		SIFContMode[Ch] = (SIF_CONTMODE)uiConfig;
		break;

	case SIF_CONFIG_ID_CONTADDRBITS:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if ((uiConfig < SIF_CONTADDRBITS_MIN) || (uiConfig > SIF_CONTADDRBITS_MAX)) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = SIF_CONTADDRBITS_DEFAULT;
		}

		uiSIFContAddrBits[Ch] = uiConfig;
		break;

	case SIF_CONFIG_ID_DELAY:
		sif_setDelay(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_AUTOPINMUX:
		// Check parameter
		if (uiConfig > (UINT32)TRUE) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = (UINT32)TRUE;
		}

		// Project want to enable auto-pinmux function, and SIF channel is not opened yet!
		// We have to switch pinmux to GPIO to make sure pinmux is corrent before calling
		// sif_open() and sif_close().

		// Wait semaphore
		SEM_WAIT(SEMID_SIF);

#ifndef __KERNEL__
#if !defined(__FREERTOS)
		if ((uiConfig == (UINT32)TRUE) && ((uiSIFOpenStatus & ((UINT32)1 << Ch)) == 0)) {
			pinmux_setPinmux((PINMUX_FUNC_ID)SIFPinmuxID[Ch], PINMUX_SIF_SEL_INACTIVE);
		}
#endif
#endif

		bSIFAutoPinmux[Ch] = (BOOL)uiConfig;

		// Signal semaphore
		SEM_SIGNAL(SEMID_SIF);

		break;

	case SIF_CONFIG_ID_BURST_DELAY:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if (uiConfig > SIF_UI_BURST_DELAY_MAX) {
			DBG_ERR("Invalid delay time!\r\n");
			break;
		}
		sif_setBurstDelay(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_BURST_INTVAL1:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if (uiConfig > 7) {
			DBG_ERR("Invalid burst interval!\r\n");
			break;
		}
		sif_setBurstIntval1(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_BURST_INTVAL2:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if (uiConfig > 7) {
			DBG_ERR("Invalid burst interval!\r\n");
			break;
		}
		sif_setBurstIntval2(Ch, uiConfig);
		break;

	case SIF_CONFIG_ID_BURST_SRC:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if (uiConfig > SIF_VD_SIE5) {
			DBG_ERR("Invalid burst source!\r\n");
			break;
		}
		sif_setBurstSRC(Ch, (SIF_VD_SOURCE) uiConfig);
		break;

    case SIF_CONFIG_ID_DMA_DELAY:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}

		if ((SIF_DMA_DELAY)uiConfig > SIF_DMA_DELAY_ON) {
			DBG_WRN("Invalid value (%d) for ID %d!\r\n", (int)uiConfig, (int)ConfigID);
			uiConfig = (UINT32)SIF_DMA_DELAY_OFF;
		}

		SIFDelayMode[Ch] = (SIF_DMA_DELAY)uiConfig;
		break;

    case SIF_CONFIG_ID_DMA_DELAY_TAG:
		// Check parameter
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return;
		}
		if (uiConfig > 0xFFFF) {
			DBG_ERR("Invalid delay tag!\r\n");
			break;
		}

		sif_setDMADelayTag(Ch, uiConfig);
		break;
	default:
		DBG_ERR("Not supported ID (%d)!\r\n", (int)ConfigID);
		break;
	}
}


/**
    Get SIF configuration

    Get SIF configuration for specific channel.

    @param[in] Ch           SIF channel that will be probed
    @param[in] ConfigID     Configuration ID
    @return Configuration value.
*/
UINT32 sif_getConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID)
{
	// Check channel number
	if (Ch >= SIF_TOTAL_CHANNEL) {
		DBG_ERR("Invalid channel!\r\n");
		return 0;
	}

	// Allow to get SIF's auto pinmux configuration when SIF is not open
	if ((ConfigID != SIF_CONFIG_ID_AUTOPINMUX) && ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("SIF_CH%d is not opened!\r\n", (int)Ch);
		return 0;
	}

	switch (ConfigID) {
	case SIF_CONFIG_ID_BUSMODE:
		return sif_getMode(Ch);

	case SIF_CONFIG_ID_BUSCLOCK:
		return sif_getBusClock(Ch);

	case SIF_CONFIG_ID_SEND:
		return sif_getSEND(Ch);

	case SIF_CONFIG_ID_SENS:
		return sif_getSENS(Ch);

	case SIF_CONFIG_ID_SENH:
		return sif_getSENH(Ch);

	case SIF_CONFIG_ID_LENGTH:
		return sif_getTranLength(Ch);

	case SIF_CONFIG_ID_DIR:
		return sif_getShiftDIR(Ch);

	case SIF_CONFIG_ID_CONTMODE:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return (UINT32)SIFContMode[Ch];

	case SIF_CONFIG_ID_CONTADDRBITS:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return (UINT32)uiSIFContAddrBits[Ch];

	case SIF_CONFIG_ID_DELAY:
		return sif_getDelay(Ch);

	case SIF_CONFIG_ID_AUTOPINMUX:
		return (UINT32)bSIFAutoPinmux[Ch];

	case SIF_CONFIG_ID_BURST_DELAY:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return sif_getBurstDelay(Ch);

	case SIF_CONFIG_ID_BURST_INTVAL1:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return sif_getBurstIntval1(Ch);

	case SIF_CONFIG_ID_BURST_INTVAL2:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return sif_getBurstIntval2(Ch);

	case SIF_CONFIG_ID_BURST_SRC:
		if (Ch > SIF_CH1) {
			DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
			return 0;
		}
		return sif_getBurstSRC(Ch);

	default:
		DBG_ERR("Not supported ID (%d)!\r\n", (int)ConfigID);
		return 0;
	}
}


/**
    Send data

    Send data out of specific channel.
    If the channel is LSb out first, shift out sequence will be uiData0 [b0 b1..b31],
    uiData1 [b0 b1..b31], uiData2 [b0 b1..b31].
    If the channel is MSb out first, Shift out sequence will be uiData0 [b31 b30..b0],
    uiData1 [b31 b30..b0], uiData2 [b31 b30..b0].
    You have to do data alignment before calling this function.

    @note Don't call this function in ISR.

    @param[in] Ch       SIF channel
    @param[in] uiData0  SIF data 0
    @param[in] uiData1  SIF data 1
    @param[in] uiData2  SIF data 2
    @return void
*/
void sif_send(SIF_CH Ch, UINT32 uiData0, UINT32 uiData1, UINT32 uiData2)
{
	unsigned long      flags;
	RCW_DEF(SIF_DATA0_REG);
	RCW_DEF(SIF_DATA1_REG);
	RCW_DEF(SIF_DATA2_REG);
	RCW_DEF(SIF_DATASEL_REG);
	RCW_DEF(SIF_CONF0_REG);
	FLGPTN  FlagPtn;
	BOOL    bNewTran;

	// Check channel number and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// New transmission
	if (uiSIFRemainingBits[Ch] == 0) {
		// Wait for previous transmission done
		wai_flg(&FlagPtn, FLG_ID_SIF, SIFFlagPtnEnd[Ch], TWF_ORW | TWF_CLR);
		bNewTran = TRUE;
	} else {
		// Wait for data empty
		wai_flg(&FlagPtn, FLG_ID_SIF, SIFFlagPtnEmpty[Ch], TWF_ORW | TWF_CLR);
		bNewTran = FALSE;
	}

	// Enter critical section
	loc_cpu(flags);

	// Update remaining bits
	if (bNewTran == TRUE) {
		uiSIFRemainingBits[Ch] = sif_getTranLength(Ch);
	}

	if (uiSIFRemainingBits[Ch] > SIF_MAX_DATAREG_BITS) {
		uiSIFRemainingBits[Ch] -= SIF_MAX_DATAREG_BITS;
	} else {
		uiSIFRemainingBits[Ch] = 0;
	}

	// Select data port for specific channel
	RCW_VAL(SIF_DATASEL_REG)        = 0;
	RCW_OF(SIF_DATASEL_REG).DP_Sel  = uiSIFDPSel[Ch];
	RCW_ST(SIF_DATASEL_REG);

	// Set data register
	RCW_VAL(SIF_DATA0_REG) = uiData0;
	RCW_ST(SIF_DATA0_REG);

	RCW_VAL(SIF_DATA1_REG) = uiData1;
	RCW_ST(SIF_DATA1_REG);

	RCW_VAL(SIF_DATA2_REG) = uiData2;
	RCW_ST(SIF_DATA2_REG);

	// Start or restart transmission
	if (bNewTran == TRUE) {
		// Start
		RCW_VAL(SIF_CONF0_REG) = SIF_CONF_START_CH(Ch);
	} else {
		// Restart
		RCW_VAL(SIF_CONF0_REG) = SIF_CONF_RESTART_CH(Ch);
	}
	RCW_ST(SIF_CONF0_REG);

	// Leave critical section
	unl_cpu(flags);
}


/**
    Wait for SIF PIO transmission end

    Call this API will wait for SIF specific channel's PIO transmission end.

    @note Don't call this function in ISR.

    @param[in] Ch   Which channel that you want to wait for

    @return void
*/
void sif_waitTransferEnd(SIF_CH Ch)
{
	FLGPTN  FlagPtn;

	// Check channel number and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// Wait for previous transfer done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFFlagPtnEnd[Ch], TWF_ORW);

#if defined(_NVT_EMULATION_)
	// Output interrupt status for emulation checking
//    DBG_DUMP("SIF Int Status: 0x%.8X\r\n", uiSIFIntStatus);
#endif
}



/**
    Send data via DMA

    Send data via DMA.

    @note Don't call this function in ISR.

    @param[in] Ch           SIF Channel
    @param[in] uiAddr       SIF DMA starting address (Word alignment)
    @param[in] uiDMALength  SIF DMA transfer length in byte unit (Word alignment, include dummy bits).
                            DMA transfer length must be >= 4 and <= 0xFFFC.
    @return void
*/
void sif_sendDMA(SIF_CH Ch, UINT32 uiAddr, UINT32 uiDMALength)
{
	unsigned long      flags;
	RCW_DEF(SIF_CONF1_REG);
	RCW_DEF(SIF_CONF2_REG);
	RCW_DEF(SIF_DMA0_STARTADDR_REG);
	RCW_DEF(SIF_DMA1_STARTADDR_REG);
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);
	RCW_DEF(SIF_DMA0_CTRL1_REG);
	RCW_DEF(SIF_DMA1_CTRL1_REG);
	FLGPTN  FlagPtn;

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Check channel number and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// Check parameter
	if ((uiAddr & 0x3) ||
		(uiDMALength & 0x3) ||
		(uiDMALength > SIF_DMA_SIZE_MAX)) {
		DBG_ERR("Invalid parameter!\r\n");
		return;
	}

	// Flush cache
#ifndef __KERNEL__
	dma_flushWriteCache(uiAddr, uiDMALength);
#else
	//fmem_dcache_sync((void *)uiAddr, uiDMALength, DMA_BIDIRECTIONAL);
#endif

	if(SIFDelayMode[Ch])
	{
		RCW_DEF(SIF_DMA0_DLY_REG);
		RCW_DEF(SIF_DMA1_DLY_REG);
		UINT32 i, *pbuffer= (UINT32 *)uiAddr;
		UINT32 tag, remap;

		if(Ch == SIF_CH0) {
			RCW_LD(SIF_DMA0_DLY_REG);
			tag = RCW_OF(SIF_DMA0_DLY_REG).SIF_DLY_TAG;
		} else if(Ch == SIF_CH1) {
			RCW_LD(SIF_DMA1_DLY_REG);
			tag = RCW_OF(SIF_DMA1_DLY_REG).SIF_DLY_TAG;
		}

		remap = uiDMALength;

		for(i=0;i<(uiDMALength>>2);i++) {
			if((*pbuffer >> 16) == tag) {
				remap -= 4;
			}
			pbuffer = pbuffer+1;
		}
		uiDMALength = remap;
		//DBG_DUMP("remap size = %d\r\n",uiDMALength);
	}

	// Wait for previous DMA and PIO transmission done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFDMAFlagPtnEnd[Ch] | SIFFlagPtnEnd[Ch], TWF_ANDW | TWF_CLR);

	// Enter critical section
	loc_cpu(flags);

	switch (Ch) {
	default:
	case SIF_CH0:
		// Configure DMA mode and address bits
		RCW_LD(SIF_CONF1_REG);
		RCW_OF(SIF_CONF1_REG).Cont_Ch0 = SIFContMode[Ch];
		RCW_ST(SIF_CONF1_REG);

		RCW_LD(SIF_CONF2_REG);
		RCW_OF(SIF_CONF2_REG).Ch0_Addr_Bits = uiSIFContAddrBits[Ch];
		RCW_ST(SIF_CONF2_REG);

		// Configure DMA starting address and length
		RCW_VAL(SIF_DMA0_STARTADDR_REG)              = 0;
		RCW_OF(SIF_DMA0_STARTADDR_REG).DMA_StartAddr = dma_getPhyAddr(uiAddr);
		RCW_ST(SIF_DMA0_STARTADDR_REG);
		RCW_VAL(SIF_DMA0_CTRL1_REG)                  = 0;
		RCW_OF(SIF_DMA0_CTRL1_REG).DMA0_BufSize1     = uiDMALength;
		RCW_ST(SIF_DMA0_CTRL1_REG);

		// Configure DMA channel and enable DMA
		RCW_VAL(SIF_DMA0_CTRL0_REG)                  = 0;
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Sync_En      = DISABLE;
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_En           = ENABLE;
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Delay        = SIFDelayMode[Ch];
		RCW_ST(SIF_DMA0_CTRL0_REG);
		break;

	case SIF_CH1:
		// Configure DMA mode and address bits
		RCW_LD(SIF_CONF1_REG);
		RCW_OF(SIF_CONF1_REG).Cont_Ch1 = SIFContMode[Ch];
		RCW_ST(SIF_CONF1_REG);

		RCW_LD(SIF_CONF2_REG);
		RCW_OF(SIF_CONF2_REG).Ch1_Addr_Bits = uiSIFContAddrBits[Ch];
		RCW_ST(SIF_CONF2_REG);

		// Configure DMA starting address and length
		RCW_VAL(SIF_DMA1_STARTADDR_REG)              = 0;
		RCW_OF(SIF_DMA1_STARTADDR_REG).DMA_StartAddr = dma_getPhyAddr(uiAddr);
		RCW_ST(SIF_DMA1_STARTADDR_REG);
		RCW_VAL(SIF_DMA1_CTRL1_REG)                  = 0;
		RCW_OF(SIF_DMA1_CTRL1_REG).DMA1_BufSize1     = uiDMALength;
		RCW_ST(SIF_DMA1_CTRL1_REG);

		// Configure DMA channel and enable DMA
		RCW_VAL(SIF_DMA1_CTRL0_REG)                  = 0;
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Sync_En      = DISABLE;
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_En           = ENABLE;
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Delay        = SIFDelayMode[Ch];
		RCW_ST(SIF_DMA1_CTRL0_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}

/**
    Send burst data via DMA

    Send burst data via DMA.

    @note Don't call this function in ISR.

    @param[in] Ch            SIF Channel
    @param[in] uiAddr        SIF DMA starting address (Word alignment)
    @param[in] uiBurstNumber SIF DMA burst transmission number, range 1~3.
    @param[in] uiDMALength1  SIF DMA transfer length1 in byte unit (Word alignment, include dummy bits).
                             DMA transfer length must be >= 4 and <= 0xFFFC.
    @param[in] uiDMALength2  SIF DMA transfer length2 in byte unit (Word alignment, include dummy bits).
                             DMA transfer length must be >= 4 and <= 0xFFFC.
    @param[in] uiDMALength3  SIF DMA transfer length3 in byte unit (Word alignment, include dummy bits).
                             DMA transfer length must be >= 4 and <= 0xFFFC.
    @return void
*/

void sif_send_burst_DMA(SIF_CH Ch, UINT32 uiAddr, UINT8 uiBurstNumber, UINT32 uiDMALength1, UINT32 uiDMALength2, UINT32 uiDMALength3)
{
	unsigned long      flags;
	RCW_DEF(SIF_CONF1_REG);
	RCW_DEF(SIF_CONF2_REG);
	RCW_DEF(SIF_DMA0_STARTADDR_REG);
	RCW_DEF(SIF_DMA1_STARTADDR_REG);
	RCW_DEF(SIF_DMA0_CTRL0_REG);
	RCW_DEF(SIF_DMA1_CTRL0_REG);
	RCW_DEF(SIF_DMA0_CTRL1_REG);
	RCW_DEF(SIF_DMA1_CTRL1_REG);
	RCW_DEF(SIF_DMA0_CTRL2_REG);
	RCW_DEF(SIF_DMA1_CTRL2_REG);
	FLGPTN  FlagPtn;

	if (Ch > SIF_CH1) {
		DBG_ERR("SIF_CH%d does NOT support DMA!\r\n", (int)Ch);
		return;
	}

	// Check channel number, and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// Check parameter
	if ((uiAddr & 0x3) || (uiDMALength1 & 0x3) || (uiDMALength1 > SIF_DMA_SIZE_MAX) ||
		(uiDMALength2 & 0x3) || (uiDMALength2 > SIF_DMA_SIZE_MAX) ||
		(uiDMALength3 & 0x3) || (uiDMALength3 > SIF_DMA_SIZE_MAX) ||
		(uiBurstNumber > 0x3) || (uiBurstNumber < 0x1)) {
		DBG_ERR("Invalid parameter!\r\n");
		return;
	}

	// Check DMAbuffer size with burst number condition
	if ((uiBurstNumber == 0x3) &&
		((uiDMALength1 == 0x0) || (uiDMALength2 == 0x0) || (uiDMALength3 == 0x0))) {
		DBG_ERR("Invalid DMA SIZE 1!\r\n");
		return;
	} else if ((uiBurstNumber == 0x2) &&
			   ((uiDMALength1 == 0x0) || (uiDMALength2 == 0x0))) {
		DBG_ERR("Invalid DMA SIZE 2!\r\n");
		return;
	} else {
		if ((uiDMALength1 == 0x0)) {
			DBG_ERR("Invalid DMA SIZE 3!\r\n");
			return;
		}
	}

	// Flush cache
#ifndef __KERNEL__
	dma_flushWriteCache(uiAddr, uiDMALength1 + uiDMALength2 + uiDMALength3);
#else
	//fmem_dcache_sync((void *)uiAddr, uiDMALength1+uiDMALength2+uiDMALength3, DMA_BIDIRECTIONAL);
#endif

	// Wait for previous DMA and PIO transmission done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFDMAFlagPtnEnd[Ch] | SIFFlagPtnEnd[Ch], TWF_ANDW | TWF_CLR);

	// Enter critical section
	loc_cpu(flags);

	switch (Ch) {
	default:
	case SIF_CH0:
		// Configure DMA mode and address bits
		RCW_LD(SIF_CONF1_REG);
		RCW_OF(SIF_CONF1_REG).Cont_Ch0 = SIFContMode[Ch];
		RCW_ST(SIF_CONF1_REG);

		RCW_LD(SIF_CONF2_REG);
		RCW_OF(SIF_CONF2_REG).Ch0_Addr_Bits = uiSIFContAddrBits[Ch];
		RCW_ST(SIF_CONF2_REG);

		// Configure DMA starting address and length
		RCW_VAL(SIF_DMA0_STARTADDR_REG)              = 0;
		RCW_OF(SIF_DMA0_STARTADDR_REG).DMA_StartAddr = dma_getPhyAddr(uiAddr);
		RCW_ST(SIF_DMA0_STARTADDR_REG);
		RCW_VAL(SIF_DMA0_CTRL1_REG)                  = 0;
		RCW_OF(SIF_DMA0_CTRL1_REG).DMA0_BufSize1     = uiDMALength1;
		RCW_OF(SIF_DMA0_CTRL1_REG).DMA0_BufSize2     = uiDMALength2;
		RCW_ST(SIF_DMA0_CTRL1_REG);
		RCW_VAL(SIF_DMA0_CTRL2_REG)                  = 0;
		RCW_OF(SIF_DMA0_CTRL2_REG).DMA0_BufSize3     = uiDMALength3;
		RCW_ST(SIF_DMA0_CTRL2_REG);

		// Configure DMA channel and enable DMA
		RCW_LD(SIF_DMA0_CTRL0_REG);
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Burst_N      = uiBurstNumber;
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_Sync_En      = ENABLE;
		RCW_OF(SIF_DMA0_CTRL0_REG).DMA0_En           = ENABLE;
		RCW_ST(SIF_DMA0_CTRL0_REG);
		break;

	case SIF_CH1:
		// Configure DMA mode and address bits
		RCW_LD(SIF_CONF1_REG);
		RCW_OF(SIF_CONF1_REG).Cont_Ch1 = SIFContMode[Ch];
		RCW_ST(SIF_CONF1_REG);

		RCW_LD(SIF_CONF2_REG);
		RCW_OF(SIF_CONF2_REG).Ch1_Addr_Bits = uiSIFContAddrBits[Ch];
		RCW_ST(SIF_CONF2_REG);

		// Configure DMA starting address and length
		RCW_VAL(SIF_DMA1_STARTADDR_REG)              = 0;
		RCW_OF(SIF_DMA1_STARTADDR_REG).DMA_StartAddr = dma_getPhyAddr(uiAddr);
		RCW_ST(SIF_DMA1_STARTADDR_REG);
		RCW_VAL(SIF_DMA1_CTRL1_REG)                  = 0;
		RCW_OF(SIF_DMA1_CTRL1_REG).DMA1_BufSize1     = uiDMALength1;
		RCW_OF(SIF_DMA1_CTRL1_REG).DMA1_BufSize2     = uiDMALength2;
		RCW_ST(SIF_DMA1_CTRL1_REG);
		RCW_VAL(SIF_DMA1_CTRL2_REG)                  = 0;
		RCW_OF(SIF_DMA1_CTRL2_REG).DMA1_BufSize3     = uiDMALength3;
		RCW_ST(SIF_DMA1_CTRL2_REG);

		// Configure DMA channel and enable DMA
		RCW_LD(SIF_DMA1_CTRL0_REG);
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Burst_N      = uiBurstNumber;
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_Sync_En      = ENABLE;
		RCW_OF(SIF_DMA1_CTRL0_REG).DMA1_En           = ENABLE;
		RCW_ST(SIF_DMA1_CTRL0_REG);
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}

/**
    Wait for SIF DMA transmission end

    Call this API will wait for SIF DMA transmission end.

    @note Don't call this function in ISR.

    @return void
*/
void sif_waitDMATransferEnd(SIF_CH Ch)
{
	FLGPTN  FlagPtn;

	// Check if SIF driver is opened
	if (uiSIFOpenStatus == 0) {
		DBG_ERR("SIF isn't opened!\r\n");
		return;
	}

	// Wait for previous transfer done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFDMAFlagPtnEnd[Ch], TWF_ORW);

#if defined(_NVT_EMULATION_)
	// Output interrupt status for emulation checking
    //DBG_DUMP("SIF Int Status: 0x%.8X\r\n", uiSIFIntStatus);
#endif
}

/**

*/
BOOL sif_dmapack_start(UINT32 *current_index)
{
	if (current_index != NULL)
		*current_index = 0;
	else
		DBG_ERR("index NULL\r\n");

	return FALSE;
}

/**

*/
BOOL sif_dmapack_data(SIF_CH Ch, UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT32 data_bit_len, UINT32* data)
{
	UINT32 *pDesc = (UINT32 *)(desc_buf_addr+*current_index);
	UINT32 i, wordcnt, tag;
	RCW_DEF(SIF_DMA0_DLY_REG);
	RCW_DEF(SIF_DMA1_DLY_REG);

	wordcnt = (data_bit_len+31)>>5;
	if((*current_index+(wordcnt<<2)) > max_buf_size) {
		DBG_ERR("DMA Buf Size not enough %d %d\r\n", (int)*current_index, (int)max_buf_size);
		return TRUE;
	}

	if(Ch == SIF_CH0) {
		RCW_LD(SIF_DMA0_DLY_REG);
		tag = RCW_OF(SIF_DMA0_DLY_REG).SIF_DLY_TAG;
	} else if(Ch == SIF_CH1) {
		RCW_LD(SIF_DMA1_DLY_REG);
		tag = RCW_OF(SIF_DMA1_DLY_REG).SIF_DLY_TAG;
	} else {
		DBG_ERR("SIF CH ERR %d\r\n", (int)Ch);
		return TRUE;
	}

	for(i = 0; i < wordcnt; i++) {
		pDesc[i] = data[i];
		if ((data[i] >> 16) == tag)
			DBG_WRN("DATA(0x%08X) Hits DelayTAG\r\n",(unsigned int)data[i]);
	}

	*current_index += (wordcnt<<2);
	return FALSE;
}

/**

*/
BOOL 	sif_dmapack_delay_us(SIF_CH Ch, UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT32 delay_us)
{
	UINT32 *pDesc = (UINT32 *)(desc_buf_addr+*current_index);
	UINT64 tickcount;
	UINT32 i, wordcnt,tag;
	RCW_DEF(SIF_DMA0_DLY_REG);
	RCW_DEF(SIF_DMA1_DLY_REG);

#ifndef __KERNEL__
	tickcount = ((UINT64)delay_us*SIF_SOURCE_CLOCK/1000000)+1;
	wordcnt = (UINT32)((UINT64)(tickcount+65534)/65535);
#else
	tickcount = delay_us*SIF_SOURCE_CLOCK;
	do_div(tickcount,1000000);
	tickcount+=65535;
	do_div(tickcount,65535);
	wordcnt = (UINT32)tickcount;
#endif

	if((*current_index+(wordcnt<<2)) > max_buf_size) {
		DBG_ERR("DMA Buf Size not enough %d %d\r\n", (int)*current_index, (int)max_buf_size);
		return TRUE;
	}

	if(Ch == SIF_CH0) {
		RCW_LD(SIF_DMA0_DLY_REG);
		tag = (RCW_OF(SIF_DMA0_DLY_REG).SIF_DLY_TAG) << 16;
	} else if(Ch == SIF_CH1) {
		RCW_LD(SIF_DMA1_DLY_REG);
		tag = (RCW_OF(SIF_DMA1_DLY_REG).SIF_DLY_TAG) << 16;
	} else {
		DBG_ERR("SIF CH ERR %d\r\n", (int)Ch);
		return TRUE;
	}

	for(i = 0; i < wordcnt; i++) {
		if(tickcount > 65535) {
			pDesc[i] = 65535+tag;
			tickcount -= 65535;
		} else {
			pDesc[i] = tickcount+tag;
		}
		//DBG_DUMP("DLY(0x%08X)\r\n",pDesc[i]);
	}

	*current_index += (wordcnt<<2);
	return FALSE;
}

//@}

// Emulation only code
#if defined(_NVT_EMULATION_)

void    sifTest_sendDelay(SIF_CH Ch);
void    sifTest_setConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID, UINT32 uiConfig);
void    sifTest_waitDMATransferEnd(SIF_CH Ch);
BOOL    sifTest_compareRegDefaultValue(void);

/*
    (Verification code) Send 2 data transmissions in PIO mode for delay testing

    (Verification code) Send 2 data transmissions in PIO mode for delay testing.
    Please don't set length >= 32 when calling this API.

    @param[in] Ch           SIF channel that will be tested
    @return void
*/
void sifTest_sendDelay(SIF_CH Ch)
{
	unsigned long      flags;
	RCW_DEF(SIF_DATA0_REG);
	RCW_DEF(SIF_DATASEL_REG);
	RCW_DEF(SIF_INTEN0_REG);
	RCW_DEF(SIF_INTSTS0_REG);
	RCW_DEF(SIF_CONF0_REG);
	FLGPTN  FlagPtn;
	UINT32 oldIntEn = 0;

	// Check channel number and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// Check length
	if (sif_getTranLength(Ch) >= 32) {
		DBG_ERR("Invalid transfer length, must <= 32\r\n");
		return;
	}

	// Wait for previous transmission done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFFlagPtnEnd[Ch], TWF_ORW | TWF_CLR);

	// Enter critical section
	loc_cpu(flags);

	// Disable interrupt 0
	oldIntEn = RCW_LD(SIF_INTEN0_REG);
	RCW_VAL(SIF_INTEN0_REG) = 0;
	RCW_ST(SIF_INTEN0_REG);

	// Select data port for specific channel
	RCW_VAL(SIF_DATASEL_REG)        = 0;
	RCW_OF(SIF_DATASEL_REG).DP_Sel  = uiSIFDPSel[Ch];
	RCW_ST(SIF_DATASEL_REG);

	// Set data register
	RCW_VAL(SIF_DATA0_REG) = 0xFF;
	RCW_ST(SIF_DATA0_REG);

	// Start 1st data
	RCW_VAL(SIF_CONF0_REG) = SIF_CONF_START_CH(Ch);
	RCW_ST(SIF_CONF0_REG);

	// Polling trnasfer end status
	while (1) {
		RCW_LD(SIF_INTSTS0_REG);
		if ((RCW_VAL(SIF_INTSTS0_REG) & SIF_INT_END(Ch)) != 0) {
			RCW_ST(SIF_INTSTS0_REG);
			break;
		}
	}

	// Select data port for specific channel
	// Set data register
	RCW_ST(SIF_DATASEL_REG);
	RCW_ST(SIF_DATA0_REG);

	// Start 2nd data
	RCW_ST(SIF_CONF0_REG);

	// Enable interrupt for data empty, data end and DMA end
	RCW_VAL(SIF_INTEN0_REG) = oldIntEn;
	RCW_ST(SIF_INTEN0_REG);

	// Leave critical section
	unl_cpu(flags);
}

/*
    (Verification code) Configure SIF

    (Verification code) Configuration for specific SIF channel.

    @param[in] Ch           SIF channel that will be configured
    @param[in] ConfigID     Configuration ID. Only support the following:
        - @b SIF_CONFIG_ID_BUSCLOCK
        - @b SIF_CONFIG_ID_SEND
        - @b SIF_CONFIG_ID_SENS
        - @b SIF_CONFIG_ID_SENH
        - @b SIF_CONFIG_ID_DELAY
    @param[in] uiConfig     Configuration value (Register setting)
    @return void
*/
void sifTest_setConfig(SIF_CH Ch, SIF_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	unsigned long      flags;
	RCW_DEF(SIF_CLKCTRL0_REG);
	RCW_DEF(SIF_TIMING0_REG);
	RCW_DEF(SIF_TIMING1_REG);
	RCW_DEF(SIF_DELAY0_REG);
	RCW_DEF(SIF_DMA0_CTRL3_REG);
	RCW_DEF(SIF_DMA1_CTRL3_REG);

	// Check channel number and SIF driver is opened
	if ((Ch >= SIF_TOTAL_CHANNEL) || ((uiSIFOpenStatus & (1 << Ch)) == 0)) {
		DBG_ERR("Invalid channel or SIF isn't opened!\r\n");
		return;
	}

	// Enter critical section
	loc_cpu(flags);

	switch (ConfigID) {
	case SIF_CONFIG_ID_BUSCLOCK:
		// Check parameter
		if (uiConfig > SIF_DIVIDER_MAX) {
			uiConfig = SIF_DIVIDER_MAX;
		}

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_CLKCTRL0_REG);
			RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch0 = uiConfig;
			RCW_ST(SIF_CLKCTRL0_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_CLKCTRL0_REG);
			RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch1 = uiConfig;
			RCW_ST(SIF_CLKCTRL0_REG);
			break;

		case SIF_CH2:
			RCW_LD(SIF_CLKCTRL0_REG);
			RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch2 = uiConfig;
			RCW_ST(SIF_CLKCTRL0_REG);
			break;

		case SIF_CH3:
			RCW_LD(SIF_CLKCTRL0_REG);
			RCW_OF(SIF_CLKCTRL0_REG).ClkDiv_Ch3 = uiConfig;
			RCW_ST(SIF_CLKCTRL0_REG);
			break;

		default:
			break;
		}
		break;

	case SIF_CONFIG_ID_SEND:
		// Check parameter
		if (uiConfig > (SIF_SEND_MAX - 1)) {
			uiConfig = (SIF_SEND_MAX - 1);
		}

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SEND_Ch0 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SEND_Ch1 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH2:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SEND_Ch2 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		case SIF_CH3:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SEND_Ch3 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		default:
			break;
		}
		break;

	case SIF_CONFIG_ID_SENS:
		// Check parameter
		if (uiConfig > SIF_SENS_MAX) {
			uiConfig = SIF_SENS_MAX;
		}

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SENS_Ch0 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SENS_Ch1 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH2:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SENS_Ch2 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		case SIF_CH3:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SENS_Ch3 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		default:
			break;
		}
		break;

	case SIF_CONFIG_ID_SENH:
		// Check parameter
		if (uiConfig > SIF_SENH_MAX) {
			uiConfig = SIF_SENH_MAX;
		}

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SENH_Ch0 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_TIMING0_REG);
			RCW_OF(SIF_TIMING0_REG).SENH_Ch1 = uiConfig;
			RCW_ST(SIF_TIMING0_REG);
			break;

		case SIF_CH2:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SENH_Ch2 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		case SIF_CH3:
			RCW_LD(SIF_TIMING1_REG);
			RCW_OF(SIF_TIMING1_REG).SENH_Ch3 = uiConfig;
			RCW_ST(SIF_TIMING1_REG);
			break;

		default:
			break;
		}
		break;

	case SIF_CONFIG_ID_DELAY:
		// Check parameter
		if (uiConfig > SIF_DELAY_MAX) {
			uiConfig = SIF_DELAY_MAX;
		}

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_DELAY0_REG);
			RCW_OF(SIF_DELAY0_REG).Delay_Ch0 = uiConfig;
			RCW_ST(SIF_DELAY0_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_DELAY0_REG);
			RCW_OF(SIF_DELAY0_REG).Delay_Ch1 = uiConfig;
			RCW_ST(SIF_DELAY0_REG);
			break;

		case SIF_CH2:
			RCW_LD(SIF_DELAY0_REG);
			RCW_OF(SIF_DELAY0_REG).Delay_Ch2 = uiConfig;
			RCW_ST(SIF_DELAY0_REG);
			break;

		case SIF_CH3:
			RCW_LD(SIF_DELAY0_REG);
			RCW_OF(SIF_DELAY0_REG).Delay_Ch3 = uiConfig;
			RCW_ST(SIF_DELAY0_REG);
			break;

		default:
			break;
		}
		break;

	case SIF_CONFIG_ID_BURST_DELAY:

		// Set register
		switch (Ch) {
		case SIF_CH0:
			RCW_LD(SIF_DMA0_CTRL3_REG);
			RCW_OF(SIF_DMA0_CTRL3_REG).DMA_Burst_Delay = uiConfig;
			RCW_ST(SIF_DMA0_CTRL3_REG);
			break;

		case SIF_CH1:
			RCW_LD(SIF_DMA1_CTRL3_REG);
			RCW_OF(SIF_DMA1_CTRL3_REG).DMA_Burst_Delay = uiConfig;
			RCW_ST(SIF_DMA1_CTRL3_REG);
			break;

		default:
			break;
		}
		break;

	default:
		DBG_ERR("ConfigID isn't supported!\r\n");
		break;
	}

	// Leave critical section
	unl_cpu(flags);
}

/*
    (Verification code) Wait for SIF DMA transmission end

    (Verification code) Call this API will wait for SIF DMA transmission end.
    And it won't show interrupt status.

    @note Don't call this function in ISR.

    @return void
*/
void sifTest_waitDMATransferEnd(SIF_CH Ch)
{
	FLGPTN  FlagPtn;

	// Check channel number and SIF driver is opened
	if (uiSIFOpenStatus == 0) {
		DBG_ERR("SIF isn't opened!\r\n");
		return;
	}

	// Wait for previous transfer done
	wai_flg(&FlagPtn, FLG_ID_SIF, SIFDMAFlagPtnEnd[Ch], TWF_ORW);
}

static SIF_REG_DEFAULT  SIFRegDefault[] = {
	{ 0x00,     SIF_MODE_REG_DEFAULT,             "Mode"          },
	{ 0x04,     SIF_CONF0_REG_DEFAULT,            "Config0"       },
	{ 0x08,     SIF_CONF1_REG_DEFAULT,            "Config1"       },
	{ 0x0C,     SIF_CONF2_REG_DEFAULT,            "Config2"       },
	{ 0x10,     SIF_CLKCTRL0_REG_DEFAULT,         "ClockCtrl0"    },
	{ 0x1C,     SIF_TXSIZE0_REG_DEFAULT,          "TxSize0"       },
	{ 0x2C,     SIF_STS_REG_DEFAULT,              "Status"        },
	{ 0x30,     SIF_DATA0_REG_DEFAULT,            "Data0"         },
	{ 0x34,     SIF_DATA1_REG_DEFAULT,            "Data1"         },
	{ 0x38,     SIF_DATA2_REG_DEFAULT,            "Data2"         },
	{ 0x3C,     SIF_DATASEL_REG_DEFAULT,          "DataSel"       },
	{ 0x40,     SIF_DELAY0_REG_DEFAULT,           "Delay0"        },
	{ 0x50,     SIF_TIMING0_REG_DEFAULT,          "Timing0"       },
	{ 0x54,     SIF_TIMING1_REG_DEFAULT,          "Timing1"       },

	{ 0x60,     SIF_DMA0_CTRL0_REG_DEFAULT,       "DMA0_Ctrl0"    },
	{ 0x64,     SIF_DMA0_CTRL1_REG_DEFAULT,       "DMA0_Ctrl1"    },
	{ 0x68,     SIF_DMA0_CTRL2_REG_DEFAULT,       "DMA0_Ctrl2"    },
	{ 0x6C,     SIF_DMA0_CTRL3_REG_DEFAULT,       "DMA0_Ctrl3"    },
	{ 0x70,     SIF_DMA0_STARTADDR_REG_DEFAULT,   "DMA0_Addr"     },
	{ 0x74,     SIF_DMA0_DELAY_TAG,               "DMA0_Tag"      },

	{ 0x80,     SIF_DMA1_CTRL0_REG_DEFAULT,       "DMA1_Ctrl0"    },
	{ 0x84,     SIF_DMA1_CTRL1_REG_DEFAULT,       "DMA1_Ctrl1"    },
	{ 0x88,     SIF_DMA1_CTRL2_REG_DEFAULT,       "DMA1_Ctrl2"    },
	{ 0x8C,     SIF_DMA1_CTRL3_REG_DEFAULT,       "DMA1_Ctrl3"    },
	{ 0x90,     SIF_DMA1_STARTADDR_REG_DEFAULT,   "DMA1_Addr"     },
	{ 0x94,     SIF_DMA1_DELAY_TAG,               "DMA1_Tag"      },
	{ 0x9C,     SIF_INTEN_REG_DEFAULT,            "IntEnable"     },
	{ 0xA4,     SIF_INTSTS_REG_DEFAULT,           "IntStatus"     }
};

/*
    (Verification code) Compare SIF register default value

    (Verification code) Compare SIF register default value.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL sifTest_compareRegDefaultValue(void)
{
	UINT32  uiIndex;
	BOOL    bReturn = TRUE;

	for (uiIndex = 0; uiIndex < (sizeof(SIFRegDefault) / sizeof(SIF_REG_DEFAULT)); uiIndex++) {
		if (INW(_REGIOBASE + SIFRegDefault[uiIndex].uiOffset) != SIFRegDefault[uiIndex].uiValue) {
			DBG_ERR("%s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
					SIFRegDefault[uiIndex].pName,
					(unsigned int)SIFRegDefault[uiIndex].uiOffset,
					(unsigned int)INW(_REGIOBASE + SIFRegDefault[uiIndex].uiOffset),
					(unsigned int)SIFRegDefault[uiIndex].uiValue);
			bReturn = FALSE;
		}
	}

	return bReturn;
}

#endif
