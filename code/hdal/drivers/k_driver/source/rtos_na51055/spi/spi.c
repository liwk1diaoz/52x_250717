/*
    SPI module driver

    This file is the driver of SPI module

    @file       spi.c
    @ingroup    mIDrvIO_SPI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#include <string.h>
//#include "interrupt.h"
#include "plat/interrupt.h"
#include "spi.h"
//#include "spi_protected.h"
#include "spi_reg.h"
#include "spi_int.h"
#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#include "gpio.h"
#include "comm/timer.h"
#include "dma.h"
//#include "ist.h"
//#include "Utility.h"
#include <kwrap/perf.h>
#include <kwrap/spinlock.h>
#include <kwrap/util.h>
#include <kwrap/cpu.h>
#include "plat/pad.h"

#define __MODULE__  freertos_drv_spi
#define __DBGLVL__    2
#include <kwrap/debug.h>

static  VK_DEFINE_SPINLOCK(spi_spinlock);

static SEM_HANDLE       SEMID_SPI;
static SEM_HANDLE       SEMID_SPI2;
static SEM_HANDLE       SEMID_SPI3;
static SEM_HANDLE       SEMID_SPI4;
static SEM_HANDLE       SEMID_SPI5;

static BOOL vSPIOpened[SPI_ID_COUNT] = {
	FALSE,	FALSE,	FALSE,	FALSE,	FALSE,
};

const static CG_EN vSpiClkEn[SPI_ID_COUNT] = {
	SPI_CLK,	SPI2_CLK,	SPI3_CLK,
	SPI4_CLK,	SPI5_CLK,
};

static SEM_HANDLE vSpiSemaphore[SPI_ID_COUNT];

#define FLGPTN_SPI          (1<<0)
#define FLGPTN_SPI2         (1<<1)
#define FLGPTN_SPI3         (1<<2)
#define FLGPTN_SPI4         (1<<3)
#define FLGPTN_SPI5         (1<<4)
#define FLGPTN_SPI_TIMEOUT  (1<<5)
#define FLGPTN_SPI_TIMEOUT2 (1<<6)
#define FLGPTN_SPI_TIMEOUT3 (1<<7)
#define FLGPTN_SPI_TIMEOUT4 (1<<8)
#define FLGPTN_SPI_TIMEOUT5 (1<<9)
#define FLGPTN_SPI_DMA      (1<<10)
#define FLGPTN_SPI_DMA2     (1<<11)
#define FLGPTN_SPI_DMA3     (1<<12)
#define FLGPTN_SPI_DMA4     (1<<13)
#define FLGPTN_SPI_DMA5     (1<<14)

static ID		FLG_ID_SPI;

const static FLGPTN vSpiFlags[SPI_ID_COUNT] = {
	FLGPTN_SPI,	FLGPTN_SPI2,	FLGPTN_SPI3,
	FLGPTN_SPI4,	FLGPTN_SPI5,
};

const static FLGPTN vSpiTimeoutFlags[SPI_ID_COUNT] = {
	FLGPTN_SPI_TIMEOUT,     FLGPTN_SPI_TIMEOUT2,    FLGPTN_SPI_TIMEOUT3,
	FLGPTN_SPI_TIMEOUT4,	FLGPTN_SPI_TIMEOUT5,
};

const static FLGPTN vSpiDmaFlags[SPI_ID_COUNT] = {
	FLGPTN_SPI_DMA, FLGPTN_SPI_DMA2,
	FLGPTN_SPI_DMA3,
	FLGPTN_SPI_DMA4,
	FLGPTN_SPI_DMA5,
};

#if 0
const static DRV_INT_NUM vSpiIntNum[SPI_ID_COUNT] = {
	DRV_INT_SPI,    DRV_INT_SPI2,   DRV_INT_SPI3,   //Temp mark for build pass
};
#endif

const static PLL_CLKFREQ vSpiPllClkID[SPI_ID_COUNT] = {
	SPICLK_FREQ,    SPI2CLK_FREQ,   SPI3CLK_FREQ,
	SPI4CLK_FREQ,	SPI5CLK_FREQ,
};

const static UINT32 vSpiHostCapability[2][SPI_ID_COUNT] = {
	// NT98520
	{
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS | SPI_CAPABILITY_GYRO,
		0,
		0,
	},
	// NT98528
	{
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS | SPI_CAPABILITY_GYRO,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
		SPI_CAPABILITY_PIO | SPI_CAPABILITY_DMA | SPI_CAPABILITY_1BIT | SPI_CAPABILITY_2BITS,
	},
};

static const UINT32 *P_HOST_CAP;

static ID       vSPILockStatus[SPI_ID_COUNT] = {
	NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED,
	NO_TASK_LOCKED, NO_TASK_LOCKED,
};

static BOOL     vSPIAttachStatus[SPI_ID_COUNT] = {
	FALSE,  FALSE,  FALSE,	FALSE,	FALSE,
};

static SPI_TRANSFER_LEN vSpiTransferLen[SPI_ID_COUNT] = {
	SPI_TRANSFER_LEN_1BYTE, SPI_TRANSFER_LEN_1BYTE, SPI_TRANSFER_LEN_1BYTE,
	SPI_TRANSFER_LEN_1BYTE, SPI_TRANSFER_LEN_1BYTE,
};

static SPI_BUS_WIDTH    vSpiBusWidth[SPI_ID_COUNT] = {
	SPI_BUS_WIDTH_1_BIT,    SPI_BUS_WIDTH_1_BIT,    SPI_BUS_WIDTH_1_BIT,
	SPI_BUS_WIDTH_1_BIT,	SPI_BUS_WIDTH_1_BIT,
};

static BOOL vUnderDma[SPI_ID_COUNT] = {
	FALSE,  FALSE,  FALSE,	FALSE,	FALSE,
};

static SPI_STATE vSpiState[SPI_ID_COUNT] = {
	SPI_STATE_IDLE,	SPI_STATE_IDLE,
	SPI_STATE_IDLE,	SPI_STATE_IDLE,
	SPI_STATE_IDLE,
};

static UINT32 vDmaStartAddr[SPI_ID_COUNT] = {0};
static UINT32 vDmaTotalSize[SPI_ID_COUNT] = {0};


static UINT32 vDmaPendingLen[SPI_ID_COUNT] = {0};

static BOOL vUnderTimerCount[SPI_ID_COUNT] = {
	FALSE,  FALSE,  FALSE,	FALSE,	FALSE,
};

static TIMER_ID vSpiTimerID[SPI_ID_COUNT] = {
	0,      0,      0,	0,	0,
};

static SPI_INIT_INFO    vSpiInitInfo[SPI_ID_COUNT] = {
	// SPI_ID_1
	{
		TRUE,               // active LOW
		TRUE,               // master mode
#if (_FPGA_EMULATION_ == DISABLE)
		24000000,           // clk freq
#else
		12000000,           // clk freq
#endif
		SPI_MODE_0,         // SPI mode
		TRUE,               // LSB
		SPI_WIDE_BUS_ORDER_NORMAL, // normal wide bus order
		0,
		0
	},
	// SPI_ID_2
	{
		TRUE,               // active LOW
		TRUE,               // master mode
		24000000,           // clk freq
		SPI_MODE_0,         // SPI mode
		TRUE,               // LSB
		SPI_WIDE_BUS_ORDER_NORMAL, // normal wide bus order
		0,
		0
	},
	// SPI_ID_3
	{
		TRUE,               // active LOW
		TRUE,               // master mode
		24000000,           // clk freq
		SPI_MODE_0,         // SPI mode
		TRUE,               // LSB
		SPI_WIDE_BUS_ORDER_NORMAL, // normal wide bus order
		0,
		0
	},
	// SPI_ID_4
	{
		TRUE,               // active LOW
		TRUE,               // master mode
		24000000,           // clk freq
		SPI_MODE_0,         // SPI mode
		TRUE,               // LSB
		SPI_WIDE_BUS_ORDER_NORMAL, // normal wide bus order
		0,
		0
	},
	// SPI_ID_5
	{
		TRUE,               // active LOW
		TRUE,               // master mode
		24000000,           // clk freq
		SPI_MODE_0,         // SPI mode
		TRUE,               // LSB
		SPI_WIDE_BUS_ORDER_NORMAL, // normal wide bus order
		0,
		0
	},
};
static UINT32 vSpiEngPktCnt[SPI_ID_COUNT] = {
	1,
	1,
	1,
	1,
	1,
};
static BOOL vbSpiDoHiZCtrl[SPI_ID_COUNT] = {    // TRUE: SPI DO will disable output when no data output; FALSE: SPI DO will be always driven by SPI controller
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
};
static BOOL vbSpiAutoPinmux[SPI_ID_COUNT] = {   // TRUE: SPI driver will control pinmux when open/close; FALSE: SPI driver will NOT control pinmux
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
};
static UINT32 vuiSpiLatchClkShift[SPI_ID_COUNT] = {
	0,
	0,
	0,
	0,
	0,
};
static GYRO_BUF_QUEUE vSpiGyroBuf[SPI_GYRO_CTRL_COUNT][SPI_GYRO_QUEUE_DEPTH] = {0};
static UINT32 vSpiGyroFrontIdx[SPI_GYRO_CTRL_COUNT] = {0};    // store front index of queue vSpiGyroBuf[]
static UINT32 vSpiGyroTailIdx[SPI_GYRO_CTRL_COUNT] = {0};     // store tail index of queue vSpiGyroBuf[]
static UINT32 vSpiGyroQueueCnt[SPI_GYRO_CTRL_COUNT] = {0};    // store how many element in queue vSpiGyroBuf[]
static UINT32 vSpiGyroFrameCnt[SPI_GYRO_CTRL_COUNT] = {0};    // record frame ID (SIE VSync) after gyro mode is started
static UINT32 vSpiGyroDataCnt[SPI_GYRO_CTRL_COUNT] = {0};     // record data count to mark in gyro queue
static UINT32 vSpiGyroVdSrc[SPI_GYRO_CTRL_COUNT] = {0};       // SIE VD source config
static BOOL bSpiTestDmaAbort = FALSE;
static BOOL bSpiGyroUnitIsClk = FALSE;                          // FALSE: unit is SPI CLK, TRUE: unit is us
static UINT32 uiSpiGyroIntMsk = 0;                              // Gyro interrupt mask for engineer path
static UINT32 vSpiGyroSyncEndOffset[SPI_GYRO_CTRL_COUNT] = {0}; // SPI_GYRO_INT_SYNC_END default at last transfer
static UINT32 vSpiGyroSyncEndMask[SPI_GYRO_CTRL_COUNT] = {0};
static UINT32 vSpiGyroSyncNextEndMask[SPI_GYRO_CTRL_COUNT] = {0};
static UINT32 vSpiGyroSyncNextIntEnReg[SPI_GYRO_CTRL_COUNT] = {0};
static SPI_GYRO_STATE vSpiGyroState[SPI_GYRO_CTRL_COUNT] = {0};

static SPI_RDY_ACT_LEVEL spiRdyActiveLevel = SPI_RDY_ACT_LEVEL_LOW;

static DRV_CB pfIstCB_SPI3 = NULL;

//
// Internal function prototype
//
static ER       spi_lock(SPI_ID spiID);
static ER       spi_unlock(SPI_ID spiID);
static void     spi_waitTDRE(SPI_ID spiID);
static void     spi_waitRDRF(SPI_ID spiID);
static UINT32   spi_getBuffer(SPI_ID spiID);
static void     spi_setBuffer(SPI_ID spiID, UINT32 uiSPIBuffer);
static void     spi_setPacketCount(SPI_ID spiID, UINT32 uiPackets);
static void     spi_setWideBusOutput(SPI_ID spiID, BOOL bOutputMode);
static void     spi_setTransferCount(SPI_ID spiID, UINT32 uiCount);
static void     spi_setDmaDirection(SPI_ID spiID, BOOL uiDmaDir);
static void     spi_enableDma(SPI_ID spiID, BOOL bDmaEn);
static void     spi_setDmaWrite(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiWordCount);
static void     spi_setDmaWriteByte(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiByteCount);
static void     spi_setDmaRead(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiWordCount);
static void     spi_setDmaReadByte(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiWordCount);
static ER       spi_waitFlag(SPI_ID spiID);
static void     spi_waitClrFlag(SPI_ID spiID);
static void     spi_timeout_hdl(UINT32 uiEvent);
static void     spi_timeout_hdl2(UINT32 uiEvent);
static void     spi_timeout_hdl3(UINT32 uiEvent);
static void     spi_timeout_hdl4(UINT32 uiEvent);
static void     spi_timeout_hdl5(UINT32 uiEvent);
static ER       spi_rstGyroQueue(SPI_ID spiID);
static ER       spi_addGyroQueue(SPI_ID spiID, UINT32 uiWord0, UINT32 uiWord1);

extern ER       spi_writeReadByte(SPI_ID spiID, UINT32 uiByteCount, UINT32 *pTxBuf, UINT32 *pRxBuf, BOOL bDmaMode);
extern ER       spi_getGyroFifoCnt(SPI_ID spiID, UINT32 *puiCnt);
extern ER       spi_readGyroFifo(SPI_ID spiID, UINT32 uiCnt, UINT32 *pBuf);
extern ER       spi_readGyroCounter(SPI_ID spiID, UINT32 *puiTransfer, UINT32 *puiOp);
extern void     spi_getCurrStatus(SPI_ID spiID, UINT32 *puiIO, UINT32 *puiCfg, UINT32 *puiDelay);
extern UINT32   spi_getGyroStatus(SPI_ID spiID);
extern void     spi_clrGyroStatus(SPI_ID spiID, UINT32 uiSts);
extern BOOL     spiTest_compareRegDefaultValue(void);
extern void     spiTest_setPktDelay(SPI_ID spiID, UINT32 uiDelay);

const static DRV_CB vSpiTimeoutHdl[SPI_ID_COUNT] = {
	spi_timeout_hdl,	spi_timeout_hdl2,	spi_timeout_hdl3,
	spi_timeout_hdl4,	spi_timeout_hdl5,
};

#if (DRV_SUPPORT_IST == ENABLE)

#else
static SPI_GYRO_CB vSpiGyroHdl[SPI_ID_COUNT] = {
	NULL,   NULL,   NULL,	NULL,	NULL,
};
#endif

/**
    @addtogroup mIDrvIO_SPI
*/
//@{

/**
    @name   SPI Driver API
*/
//@{

/*
    Get SPI controller register.

    @param[in] spiID        SPI controller ID
    @param[in] offset       register offset in SPI controller (word alignment)

    @return register value
*/
static REGVALUE spi_getReg(SPI_ID spiID, UINT32 offset)
{
	REGVALUE regVal;

	if (spiID == SPI_ID_1) {
		regVal = SPI_GETREG(offset);
	} else if (spiID == SPI_ID_2) {
		regVal = SPI2_GETREG(offset);
	} else if (spiID == SPI_ID_3) {
		regVal = SPI3_GETREG(offset);
	} else if (spiID == SPI_ID_4) {
		regVal = SPI4_GETREG(offset);
	} else { //if (spiID == SPI_ID_5) {
		regVal = SPI5_GETREG(offset);
	}

#ifdef SPI_DESIGN_WORKAROUND
#if (SPI_DESIGN_WORKAROUND == 1)
	/*
	    if (offset == SPI_CTRL_REG_OFS)
	    {
	        regVal = (regVal&0x01) | ((regVal&(~0x01))<<1);
	    }
	*/
#endif
#endif

	return regVal;
}

/*
    Set SPI controller register.

    @param[in] spiID        SPI controller ID
    @param[in] offset       register offset in SPI controller (word alignment)
    @param[in] value        register value

    @return void
*/
static void spi_setReg(SPI_ID spiID, UINT32 offset, REGVALUE value)
{
	if (spiID == SPI_ID_1) {
		SPI_SETREG(offset, value);
	} else if (spiID == SPI_ID_2) {
		SPI2_SETREG(offset, value);
	} else if (spiID == SPI_ID_3) {
		SPI3_SETREG(offset, value);
	} else if (spiID == SPI_ID_4) {
		SPI4_SETREG(offset, value);
	} else { //if (spiID == SPI_ID_5) {
		SPI5_SETREG(offset, value);
	}
}

/*
    Attach SPI driver

    @param[in] spiID        SPI controller ID

    Pre-initialize SPI driver before SPI driver is opened. This function should be the very first function to be invoked.

    @return void
*/
static void spi_attach(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	if (vSPIAttachStatus[spiID] == FALSE) {
		// Enable SPI clock
		pll_enableClock(vSpiClkEn[spiID]);

		vSPIAttachStatus[spiID] = TRUE;
	}

	switch (spiID) {
	case SPI_ID_1:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI, PINMUX_STORAGE_SEL_ACTIVE);
		}

		{
			int rt;
			UINT32 pinmux;
			PIN_GROUP_CONFIG pinmux_cfg[1];

			pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
			pinmux_cfg[0].config = 0;
			rt = nvt_pinmux_capture(pinmux_cfg, 1);
			if (rt) {
				DBG_ERR("get pinmux fail %d\n", rt);
				return;
			}

			pinmux = pinmux_cfg[0].config;
			if (pinmux & PIN_SPI_CFG_CH1_2ND_PINMUX) {
				pad_set_drivingsink(PAD_DS_SGPIO10, PAD_DRIVINGSINK_10MA);
			}
		}
		break;
	case SPI_ID_2:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI2, PINMUX_STORAGE_SEL_ACTIVE);
		}
		{
			int rt;
			UINT32 pinmux;
			PIN_GROUP_CONFIG pinmux_cfg[1];

			pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
			pinmux_cfg[0].config = 0;
			rt = nvt_pinmux_capture(pinmux_cfg, 1);
			if (rt) {
				DBG_ERR("get pinmux fail %d\n", rt);
				return;
			}

			pinmux = pinmux_cfg[0].config;
//			pinmux = pinmux_getPinmux(PIN_FUNC_SPI);
			if (pinmux & PIN_SPI_CFG_CH2_2ND_PINMUX) {
				if (nvt_get_chip_id() != CHIP_NA51055) {
					// NT98528 does NOT have SPI2_2
					DBG_WRN("%s: 528 does not have SPI2_2 pinmux\r\n", __func__);
					return;
				}
				pad_set_drivingsink(PAD_DS_PGPIO1, PAD_DRIVINGSINK_10MA);
			} else {
			    pad_set_drivingsink(PAD_DS_PGPIO14, PAD_DRIVINGSINK_10MA);
			}
		}
		break;
	case SPI_ID_3:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI3, PINMUX_STORAGE_SEL_ACTIVE);
		}

		{
			int rt;
			UINT32 pinmux;
			PIN_GROUP_CONFIG pinmux_cfg[1];

			pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
			pinmux_cfg[0].config = 0;
			rt = nvt_pinmux_capture(pinmux_cfg, 1);
			if (rt) {
				DBG_ERR("get pinmux fail %d\n", rt);
				return;
			}

			pinmux = pinmux_cfg[0].config;
			//			pinmux = pinmux_getPinmux(PIN_FUNC_SPI);
			if (pinmux & PIN_SPI_CFG_CH3_2ND_PINMUX) {
				pad_set_drivingsink(PAD_DS_CGPIO11, PAD_DRIVINGSINK_10MA);
			} else if (pinmux & PIN_SPI_CFG_CH3_3RD_PINMUX) {
				if (nvt_get_chip_id() != CHIP_NA51055) {
					// NT98528 does NOT have SPI3_3
					DBG_WRN("%s: 528 does not have SPI3_3 pinmux\r\n", __func__);
					return;
				}
				pad_set_drivingsink(PAD_DS_PGPIO5, PAD_DRIVINGSINK_10MA);
			} else {
				pad_set_drivingsink(PAD_DS_PGPIO18, PAD_DRIVINGSINK_10MA);
			}
		}
		break;
	case SPI_ID_4:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI4, PINMUX_STORAGE_SEL_ACTIVE);
		}

		{
			int rt;
			UINT32 pinmux;
			PIN_GROUP_CONFIG pinmux_cfg[1];

			pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
			pinmux_cfg[0].config = 0;
			rt = nvt_pinmux_capture(pinmux_cfg, 1);
			if (rt) {
				DBG_ERR("get pinmux fail %d\n", rt);
				return;
			}

			pinmux = pinmux_cfg[0].config;
			if (pinmux & PIN_SPI_CFG_CH4_2ND_PINMUX) {
				pad_set_drivingsink(PAD_DS_CGPIO5, PAD_DRIVINGSINK_10MA);
			} else {
				pad_set_drivingsink(PAD_DS_PGPIO1, PAD_DRIVINGSINK_10MA);
			}
		}
		break;
	case SPI_ID_5:
	default:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI5, PINMUX_STORAGE_SEL_ACTIVE);
		}

		{
			int rt;
			UINT32 pinmux;
			PIN_GROUP_CONFIG pinmux_cfg[1];

			pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
			pinmux_cfg[0].config = 0;
			rt = nvt_pinmux_capture(pinmux_cfg, 1);
			if (rt) {
				DBG_ERR("get pinmux fail %d\n", rt);
				return;
			}

			pinmux = pinmux_cfg[0].config;
			if (pinmux & PIN_SPI_CFG_CH5_2ND_PINMUX) {
				// TBD
//				pad_set_drivingsink(PAD_DS_DGPIO12, PAD_DRIVINGSINK_8MA);
			} else {
				pad_set_drivingsink(PAD_DS_PGPIO5, PAD_DRIVINGSINK_10MA);
			}
		}
		break;
	}
}

/*
    Detach SPI driver

    De-initialize SPI driver after SPI driver is closed.

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_detach(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	if (vSPIAttachStatus[spiID] == TRUE) {
		pll_disableClock(vSpiClkEn[spiID]);
		vSPIAttachStatus[spiID] = FALSE;
	}

	switch (spiID) {
	case SPI_ID_1:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI, PINMUX_STORAGE_SEL_INACTIVE);
		}
		break;
	case SPI_ID_2:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI2, PINMUX_STORAGE_SEL_INACTIVE);
		}
		break;
	case SPI_ID_3:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI3, PINMUX_STORAGE_SEL_INACTIVE);
		}
		break;
	case SPI_ID_4:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI4, PINMUX_STORAGE_SEL_INACTIVE);
		}
		break;
	case SPI_ID_5:
	default:
		if (vbSpiAutoPinmux[spiID] == TRUE) {
//			pinmux_setPinmux(PINMUX_FUNC_ID_SPI5, PINMUX_STORAGE_SEL_INACTIVE);
		}
		break;
	}
}

/**
    Open SPI driver

    @note This function should be called after spi_init() and be called
        before other SPI operation functions.

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER spi_open(SPI_ID spiID)
{
	ER erReturn;
	T_SPI_CTRL_REG ctrlReg = {0};
	T_SPI_IO_REG ioReg = {0};
	T_SPI_CONFIG_REG configReg = {0};
	T_SPI_TIMING_REG timingReg = {0};
	T_SPI_DLY_CHAIN_REG dlyReg = {0};
	T_SPI_INTEN_REG intenReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}
	if (nvt_get_chip_id() == CHIP_NA51055) {
		// NT98520 only has 3 SPI
		if (spiID >= SPI_ID_4) {
			return E_ID;
		}
	}

	if (vSPIOpened[spiID]) {
		return E_OK;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	erReturn = spi_lock(spiID);
	if (erReturn != E_OK) {
		DBG_ERR("SPI%d: lock fail\r\n", spiID);
		return erReturn;
	}

	// Enable SPI interrupt
//	drv_enableInt(vSpiIntNum[spiID]);

	pll_setClockFreq(vSpiPllClkID[spiID], vSpiInitInfo[spiID].uiFreq);

	spi_attach(spiID);

	spi_rstGyroQueue(spiID);

	// Disable SPI_EN first
	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	ctrlReg.bit.spi_en = 0;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
	while (1) {
		ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		if (ctrlReg.bit.spi_en == 0) break;
	}
//	while (spi_getReg(spiID, SPI_CTRL_REG_OFS) != 0);

	dlyReg.reg = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
	configReg.bit.spi_pkt_lsb_mode = vSpiInitInfo[spiID].bLSB;
	switch (vSpiInitInfo[spiID].spiMODE) {
	case SPI_MODE_0:
		ioReg.bit.spi_cpha = 0;
		ioReg.bit.spi_cpol = 0;
		dlyReg.bit.latch_clk_edge = 1;  // latch at falling edge
		dlyReg.bit.latch_clk_shift = 0;
		break;
	case SPI_MODE_1:
		ioReg.bit.spi_cpha = 1;
		ioReg.bit.spi_cpol = 0;
		dlyReg.bit.latch_clk_edge = 0;  // latch at rising edge
		dlyReg.bit.latch_clk_shift = 1;
		break;
	case SPI_MODE_2:
		ioReg.bit.spi_cpha = 0;
		ioReg.bit.spi_cpol = 1;
		dlyReg.bit.latch_clk_edge = 1;  // latch at falling edge
		dlyReg.bit.latch_clk_shift = 0;
		break;
	default:
		ioReg.bit.spi_cpha = 1;
		ioReg.bit.spi_cpol = 1;
		dlyReg.bit.latch_clk_edge = 0;  // latch at rising edge
		dlyReg.bit.latch_clk_shift = 1;
		break;
	}
	if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
		configReg.bit.spi_pktlen = SPI_PKT_LEN_ENUM_1BYTE;
	} else {
		configReg.bit.spi_pktlen = SPI_PKT_LEN_ENUM_2BYTES;
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	if (vSpiInitInfo[spiID].bCSActiveLow == TRUE) {
		ioReg.bit.spi_gyro_cs_pol = SPI_CS_ACT_LVL_LOW;
		ctrlReg.bit.spics_value = 1;                // CS default to HIGH
	} else {
		ioReg.bit.spi_gyro_cs_pol = SPI_CS_ACT_LVL_HIGH;
		ctrlReg.bit.spics_value = 0;                // CS default to LOW
	}

	spi_setReg(spiID, SPI_DLY_CHAIN_REG_OFS, dlyReg.reg);

	if (vSpiInitInfo[spiID].wideBusOrder == SPI_WIDE_BUS_ORDER_NORMAL) {
		ioReg.bit.spi_io_order = 1;
	} else {
		ioReg.bit.spi_io_order = 0;
	}
	ioReg.bit.spi_bus_width = 0;       // default to 1 bit
	ioReg.bit.spi_io_out_en = 1;        // output enable
	if (vbSpiDoHiZCtrl[spiID] == FALSE) {
		ioReg.bit.spi_auto_io_out_en = 0;   // obey spi_io_outen
	} else {
		ioReg.bit.spi_auto_io_out_en = 1;   // not output when spi is idle
	}
	spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);

	ctrlReg.bit.spi_en = 1;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
	while (spi_getReg(spiID, SPI_CTRL_REG_OFS) != ctrlReg.reg);

	// Setup interrupt enable
	intenReg.bit.spi_dmaed_en = 1;
	intenReg.bit.spi_rdstsed_en = 1;
	spi_setReg(spiID, SPI_INTEN_REG_OFS, intenReg.reg);

	if (bSpiGyroUnitIsClk == TRUE) {
		timingReg.bit.spi_post_cond_dly = vSpiInitInfo[spiID].uiPktDelay;
		timingReg.bit.spi_cs_dly = vSpiInitInfo[spiID].uiCsCkDelay;
	} else {
		UINT32 uiCsDelay;
		UINT32 uiPktDelay;
		UINT32 uiSpiFreq = 0;

		if (pll_getClockFreq(vSpiPllClkID[spiID], &uiSpiFreq) != E_OK) {
			DBG_WRN("spi%d get src freq fail\r\n", spiID);
		}

		// Calculate CS<-->CLK delay
		uiCsDelay = ((UINT64)uiSpiFreq * vSpiInitInfo[spiID].uiCsCkDelay + 999999) / 1000000;
		if (vSpiInitInfo[spiID].uiCsCkDelay && (uiCsDelay == 0)) {
			uiCsDelay = 1;
		}
		if (uiSpiFreq == 0) {
			uiSpiFreq = 1;
		}
		if (((UINT64)uiCsDelay * 1000000 / uiSpiFreq) != vSpiInitInfo[spiID].uiCsCkDelay) {
			DBG_WRN("expec CS delay is %d us, real delay will be %lld us\r\n",
					vSpiInitInfo[spiID].uiCsCkDelay, ((UINT64)uiCsDelay * 1000000 / uiSpiFreq));
		}
		timingReg.bit.spi_cs_dly = uiCsDelay;

		// Calculate PKT delay
		uiPktDelay = ((UINT64)uiSpiFreq * vSpiInitInfo[spiID].uiPktDelay + 999999) / 1000000;
		if (vSpiInitInfo[spiID].uiPktDelay && (uiPktDelay == 0)) {
			uiPktDelay = 1;
		}
		if (((UINT64)uiPktDelay * 1000000 / uiSpiFreq) != vSpiInitInfo[spiID].uiPktDelay) {
			DBG_WRN("expec PKT delay is %d us, real delay will be %lld us\r\n",
					vSpiInitInfo[spiID].uiPktDelay, ((UINT64)uiPktDelay * 1000000 / uiSpiFreq));
		}
		if (uiPktDelay > 4) {
			uiPktDelay -= 4;    // Actual delay between packet burst is (SPI_POST_COND_DLY + 4) SPI clock.
		}
		timingReg.bit.spi_post_cond_dly = uiPktDelay;
	}
#if 0       // Removed here. Only enable before trigger DMA or Gyro
	if (timingReg.bit.spi_post_cond_dly != 0) {
		configReg.bit.spi_pkt_burst_handshake_en = 1;
		configReg.bit.spi_pkt_burst_post_cond = 1;
		spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);
	}
#endif
	spi_setReg(spiID, SPI_TIMING_REG_OFS, timingReg.reg);

	vSPIOpened[spiID] = TRUE;
	vUnderDma[spiID] = FALSE;

	vSpiState[spiID] = SPI_STATE_IDLE;

	// Pre-set the interrupt flag
	erReturn = set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
	if (erReturn != E_OK) {
		DBG_ERR("SPI%d: open: pre set flag fail\r\n", spiID);
		return erReturn;
	}

	// Pre-set the dma flag
	if (P_HOST_CAP[spiID] & SPI_CAPABILITY_DMA) {
		erReturn = set_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);
		if (erReturn != E_OK) {
			DBG_ERR("SPI%d: open: pre set dma flag fail\r\n", spiID);
			return erReturn;
		}
	}

	return E_OK;
}

/**
    Close SPI driver

    The last function to called before leaving this module.
    Disable the SPI module interrupt and spi clock.

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER spi_close(SPI_ID spiID)
{
	T_SPI_CTRL_REG ctrlReg;
	UINT32 uiSpiFreq = 0;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}
	if (nvt_get_chip_id() == CHIP_NA51055) {
		// NT98520 only has 3 SPI
		if (spiID >= SPI_ID_4) {
			return E_ID;
		}
	}

	if (!vSPIOpened[spiID]) {
		return E_OK;
	}

	spi_waitFlag(spiID);

	{
		ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		ctrlReg.bit.spi_en = 0;
		spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
	}

	// Release interrupt
//	drv_disableInt(vSpiIntNum[spiID]);

	if (pll_getClockFreq(vSpiPllClkID[spiID], &uiSpiFreq) != E_OK) {
		DBG_WRN("spi%d get src freq fail\r\n", spiID);
	}
	// One critical case: CS := HIGH then SPI_CLKEN := LOW with SPI Freq very slow
	// f/w should wait 3T SPI_CLK to ensure SPI_CS is raised by SPI controller before disable SPI CLK
	DBG_IND("%d us\r\n", (3000000 + uiSpiFreq - 1) / uiSpiFreq);
	if (uiSpiFreq == 0) {
		uiSpiFreq = 1;
	}
	vos_util_delay_us((3000000 + uiSpiFreq - 1) / uiSpiFreq);

	spi_detach(spiID);

	vSPIOpened[spiID] = FALSE;

	return spi_unlock(spiID);
}

/**
    Check SPI opened

    Check if SPI driver is opened or not.

    @param[in] spiID        SPI controller ID

    @return
        - @b TRUE: SPI driver is opened
        - @b FALSE: SPI driver is not opened
*/
BOOL spi_isOpened(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return FALSE;
	}

	return vSPIOpened[spiID];
}

/**
    SPI set transfer length

    Set transfer length of following SPI transfer.

    @param[in] spiID    SPI controller ID
    @param[in] length   SPI transfer length
                        - SPI_TRANSFER_LEN_1BYTE
                        - SPI_TRANSFER_LEN_2BYTES

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER spi_setTransferLen(SPI_ID spiID, SPI_TRANSFER_LEN length)
{
	T_SPI_CONFIG_REG configReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (length > SPI_TRANSFER_LEN_2BYTES) {
		DBG_ERR("SPI%d: incorrect length: 0x%lx. Only support 1 or 2 byte\r\n", spiID, length);
		return E_PAR;
	}

	spi_waitFlag(spiID);

	vSpiTransferLen[spiID] = length;
	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (length == SPI_TRANSFER_LEN_1BYTE) {
		configReg.bit.spi_pktlen = SPI_PKT_LEN_ENUM_1BYTE;
	} else {
		configReg.bit.spi_pktlen = SPI_PKT_LEN_ENUM_2BYTES;
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	return E_OK;
}

/**
    SPI set CS state

    @note Invoked after spi_open()

    @param[in] spiID        SPI controller ID
    @param[in] bCSActive    CS (chip select) state
                            - TRUE: active CS
                            - FALSE: inactive CS

    @return void
*/
void spi_setCSActive(SPI_ID spiID, BOOL bCSActive)
{
	T_SPI_CTRL_REG ctrlReg;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	spi_waitFlag(spiID);

	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	if (vSpiInitInfo[spiID].bCSActiveLow == TRUE) {
		if (bCSActive) {
//			drv_enableInt(vSpiIntNum[spiID]);
			ctrlReg.bit.spics_value = 0;
		} else {
//			drv_disableInt(vSpiIntNum[spiID]);
			ctrlReg.bit.spics_value = 1;
		}
	} else {
		if (bCSActive) {
//			drv_enableInt(vSpiIntNum[spiID]);
			ctrlReg.bit.spics_value = 1;
		} else {
//			drv_disableInt(vSpiIntNum[spiID]);
			ctrlReg.bit.spics_value = 0;
		}
	}
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	// Due to MIPS outstanding feature
	// Polling to ensure register write is passed to controller
	while (1) {
		T_SPI_CTRL_REG readReg;

		readReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		if (readReg.bit.spics_value == ctrlReg.bit.spics_value) {
			break;
		}
	}
}

/**
    SPI write/read data

    @note Invoked after spi_open(). In DMA mode, this function is half-duplex.
    In this mode, data should be placed in DRAM continously.

    SPI data transfer function. This function is desired to transfer continous data (bytes counts >= 8).

    @param[in] spiID            SPI controller ID
    @param[in] uiWordCount      transfer word count (unit: word)
    @param[in] pTxBuf           tx buffer
                                - NULL: do not do transmit
                                - Else: DRAM address of data to be transmitted
    @param[out] pRxBuf          rx buffer
                                - NULL: do not do receive
                                - Else: DRAM address of data to be received
    @param[in] bDmaMode         DMA or PIO transfer
                                - TRUE: DMA mode
                                - FALSE: PIO mode

    @return
        - @b E_OK: success
        - @b E_RSATR: uiWordCount exceed maimum dma length but automatically handle by driver
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER spi_writeReadData(SPI_ID spiID, UINT32 uiWordCount, UINT32 *pTxBuf, UINT32 *pRxBuf, BOOL bDmaMode)
{
	UINT32 i;
	BOOL bAutoCoverDmaLen = FALSE;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

#if 0
	if ((uiWordCount * 4) > SPI_MAX_TRANSFER_CNT) {
		DBG_ERR("SPI%d: don't support byte count exceed 0x%lx\r\n", spiID, SPI_MAX_TRANSFER_CNT);
		DBG_ERR("SPI%d: input word count: 0x%lx\r\n", spiID, uiWordCount);
		return E_PAR;
	}
#endif

	if (uiWordCount == 0) {
		DBG_WRN("Input uiWordCount is 0, just return\r\n");
		return E_PAR;
	}

	if (bDmaMode &&
		(!(P_HOST_CAP[spiID] & SPI_CAPABILITY_DMA))) {
		DBG_WRN("spi%d not support DMA mode, switch to PIO\r\n", spiID);
		bDmaMode = FALSE;
	}

	if ((bDmaMode == TRUE) && (((UINT32)pTxBuf & 0x03) || ((UINT32)pRxBuf & 0x03))) {
		DBG_ERR("buffer not word alignment, switch to PIO\r\n");
		bDmaMode = FALSE;
	}

	if (bDmaMode == TRUE) {
		if ((pTxBuf != NULL) && (pRxBuf != NULL)) {
			DBG_ERR("SPI%d: don't support full duplex transfer under DMA mode\r\n", spiID);
			return E_PAR;
		}

		spi_waitClrFlag(spiID);

		if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
			spi_setPacketCount(spiID, 4);
		} else {
			spi_setPacketCount(spiID, 2);
		}

		if (pTxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, TRUE);
			}

			spi_setDmaWrite(spiID, pTxBuf, uiWordCount);
		} else if (pRxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, FALSE);
			}
			spi_setDmaRead(spiID, pRxBuf, uiWordCount);
		} else {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, FALSE);
			}
			spi_setDmaWrite(spiID, 0, uiWordCount);

			DBG_WRN("SPI%d: both tx and rx buffer are NULL, send dummy data\r\n", spiID);
		}

		if ((uiWordCount * 4) > SPI_MAX_TRANSFER_CNT) {
			bAutoCoverDmaLen = TRUE;
		}

//        vUnderDma[spiID] = TRUE;
	} else {
		if ((pTxBuf != NULL) && (pRxBuf != NULL) && (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT)) {
			DBG_ERR("SPI%d: don't support full duplex transfer under wide bus PIO mode\r\n", spiID);
			return E_PAR;
		}

		spi_waitClrFlag(spiID);

		if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
			spi_setPacketCount(spiID, 4);
		} else {
			spi_setPacketCount(spiID, 2);
		}

		if ((pTxBuf != NULL) && (pRxBuf != NULL)) {
			for (i = uiWordCount; i; i--) {
				spi_waitTDRE(spiID);
				spi_setBuffer(spiID, *pTxBuf++);

				spi_waitRDRF(spiID);
				*pRxBuf++ = spi_getBuffer(spiID);
			}
		} else if (pTxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, TRUE);
			}
			for (i = uiWordCount; i; i--) {
				spi_waitTDRE(spiID);
				spi_setBuffer(spiID, *pTxBuf++);

				spi_waitRDRF(spiID);
				spi_getBuffer(spiID);
			}
		} else if (pRxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, FALSE);
			}
			for (i = uiWordCount; i; i--) {
				spi_waitTDRE(spiID);
				spi_setBuffer(spiID, 0);

				spi_waitRDRF(spiID);
				*pRxBuf++ = spi_getBuffer(spiID);
			}
		} else {
			DBG_WRN("SPI%d: both tx and rx buffer are NULL, send dummy data\r\n", spiID);

			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, FALSE);
			}
			for (i = uiWordCount; i; i--) {
				spi_waitTDRE(spiID);
				spi_setBuffer(spiID, 0);

				spi_waitRDRF(spiID);
				spi_getBuffer(spiID);
			}
		}

		set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
	}

	// If driver break dma length background, return E_RSATR to inform caller
	if (bAutoCoverDmaLen) {
		return E_RSATR;
	}

	return E_OK;
}

/*
    SPI write/read data (byte align version)
    (Undocument API)

    @note Invoked after spi_open(). In DMA mode, this function is half-duplex.
    In this mode, data should be placed in DRAM continously.

    SPI data transfer function. This function is desired to transfer continous data (bytes counts >= 8).

    @param[in] spiID            SPI controller ID
    @param[in] uiByteCount      transfer byte count (unit: byte)
    @param[in] pTxBuf           tx buffer
                                - NULL: do not do transmit
                                - Else: DRAM address of data to be transmitted
    @param[out] pRxBuf          rx buffer
                                - NULL: do not do receive
                                - Else: DRAM address of data to be received
    @param[in] bDmaMode         DMA or PIO transfer
                                - TRUE: DMA mode
                                - FALSE: PIO mode

    @return
        - @b E_OK: success
        - @b E_ID: invalid spi ID
        - @b Else: fail
*/
ER spi_writeReadByte(SPI_ID spiID, UINT32 uiByteCount, UINT32 *pTxBuf, UINT32 *pRxBuf, BOOL bDmaMode)
{
	BOOL bAutoCoverDmaLen = FALSE;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (uiByteCount == 0) {
		DBG_WRN("Input uiByteCount is 0, just return\r\n");
		return E_PAR;
	}

	if (bDmaMode &&
		(!(P_HOST_CAP[spiID] & SPI_CAPABILITY_DMA))) {
		DBG_WRN("spi%d not support DMA mode, switch to PIO\r\n", spiID);
		bDmaMode = FALSE;
	}

	if ((bDmaMode == TRUE) && (((UINT32)pTxBuf & 0x03) || ((UINT32)pRxBuf & 0x03))) {
		DBG_WRN("buffer 0x%lx, 0x%lx not word alignment, switch to PIO\r\n", (UINT32)pTxBuf, (UINT32)pRxBuf);
		bDmaMode = FALSE;
	}

	if (bDmaMode == TRUE) {
		if ((pTxBuf != NULL) && (pRxBuf != NULL)) {
			DBG_ERR("SPI%d: don't support full duplex transfer under DMA mode\r\n", spiID);
			return E_PAR;
		}

		spi_waitClrFlag(spiID);

		if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
			spi_setPacketCount(spiID, 4);
		} else {
			spi_setPacketCount(spiID, 2);
		}

		if (pTxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, TRUE);
			}
			spi_setDmaWriteByte(spiID, pTxBuf, uiByteCount);
		} else if (pRxBuf != NULL) {
			if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
				spi_setWideBusOutput(spiID, FALSE);
			}
			spi_setDmaReadByte(spiID, pRxBuf, uiByteCount);
		} else {
			DBG_ERR("SPI%d: both tx and rx buffer are NULL, NOP\r\n", spiID);
		}

		if (uiByteCount > SPI_MAX_TRANSFER_CNT) {
			bAutoCoverDmaLen = TRUE;
		}

//        vUnderDma[spiID] = TRUE;
	} else {
		DBG_ERR("NOT support PIO mode\r\n");
		return E_NOSPT;
	}

	// If driver break dma length background, return E_RSATR to inform caller
	if (bAutoCoverDmaLen) {
		return E_RSATR;
	}

	return E_OK;
}

/**
    SPI wait data done

    Wait previous invoked spi_writeReadData() done.

    @param[in] spiID            SPI controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_waitDataDone(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	spi_waitFlag(spiID);

	return E_OK;
}

/**
    SPI write single word

    Write single word to SPI device.
    Actually transmitted bytes count is controlled by SPI transfer length.
    Please also refer to spi_setTransferLen().

    @param[in] spiID            SPI controller ID
    @param[in] uiTxWord         word to be sent

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_writeSingle(SPI_ID spiID, UINT32 uiTxWord)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	spi_waitClrFlag(spiID);

	if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
		spi_setWideBusOutput(spiID, TRUE);
	}

	spi_setPacketCount(spiID, vSpiEngPktCnt[spiID]);

	spi_waitTDRE(spiID);
	spi_setBuffer(spiID, uiTxWord);

	spi_waitRDRF(spiID);
	spi_getBuffer(spiID);

	set_flg(FLG_ID_SPI, vSpiFlags[spiID]);

	return E_OK;
}

/**
    SPI write packet word

    Write packet word to SPI device.
    Actually transmitted bytes count is controlled by SPI transfer length.
    Please also refer to spi_setTransferLen().

    @param[in] spiID            SPI controller ID
    @param[in] uiPktCnt         packet size
    @param[in] uiTxWord         word to be sent

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_writePacket(SPI_ID spiID, UINT32 uiPktCnt, UINT32 uiTxWord)
{
	T_SPI_CTRL_REG ctrlReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	///spi_waitClrFlag(spiID);

	// toggle CS
	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	if (vSpiInitInfo[spiID].bCSActiveLow == TRUE) {
		ctrlReg.bit.spics_value = 0;
	} else {
		ctrlReg.bit.spics_value = 1;
	}
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
		spi_setWideBusOutput(spiID, TRUE);
	}

	spi_setPacketCount(spiID, uiPktCnt);

	spi_waitTDRE(spiID);
	spi_setBuffer(spiID, uiTxWord);

	spi_waitRDRF(spiID);
	spi_getBuffer(spiID);

	// toggle CS
	if (vSpiInitInfo[spiID].bCSActiveLow == TRUE) {
		ctrlReg.bit.spics_value = 1;
	} else {
		ctrlReg.bit.spics_value = 0;
	}
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	///set_flg(FLG_ID_SPI, vSpiFlags[spiID]);

	return E_OK;
}

/**
    SPI receive single word

    Receive single word from SPI device.
    Actually received bytes count is controlled by SPI transfer length.
    Please also refer to spi_setTransferLen().

    @param[in] spiID            SPI controller ID
    @param[out] pRxWord         word buffer to be received

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_readSingle(SPI_ID spiID, UINT32 *pRxWord)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (pRxWord == NULL) {
		DBG_ERR("SPI%d: rx buffer is NULL\r\n", spiID);
		return E_PAR;
	}

	spi_waitClrFlag(spiID);

	if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
		spi_setWideBusOutput(spiID, FALSE);
	}

	spi_setPacketCount(spiID, vSpiEngPktCnt[spiID]);

	spi_waitTDRE(spiID);
	spi_setBuffer(spiID, 0);

	spi_waitRDRF(spiID);
	*pRxWord = spi_getBuffer(spiID);

	set_flg(FLG_ID_SPI, vSpiFlags[spiID]);

	return E_OK;
}

/**
    SPI send and receive single word

    Send and receive single word to/from SPI device.
    Actually transmitted bytes count is controlled by SPI transfer length.
    Please also refer to spi_setTransferLen().

    @param[in] spiID            SPI controller ID
    @param[in] uiTxWord         word to be sent
    @param[out] pRxWord         word buffer to be received

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_writeReadSingle(SPI_ID spiID, UINT32 uiTxWord, UINT32 *pRxWord)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (pRxWord == NULL) {
		DBG_ERR("SPI%d: rx buffer is NULL\r\n", spiID);
		return E_PAR;
	}

	spi_waitClrFlag(spiID);

	if (vSpiBusWidth[spiID] != SPI_BUS_WIDTH_1_BIT) {
		spi_setWideBusOutput(spiID, TRUE);
	}

	spi_setPacketCount(spiID, vSpiEngPktCnt[spiID]);

	spi_waitTDRE(spiID);
	spi_setBuffer(spiID, uiTxWord);

	spi_waitRDRF(spiID);
	*pRxWord = spi_getBuffer(spiID);

	set_flg(FLG_ID_SPI, vSpiFlags[spiID]);

	return E_OK;
}

/*
    SPI config gyro

    Setup SPI to polling GYRO information.
    (No trigger is asserted)

    @param[in] spiID            SPI controller ID
    @param[in] pGyroInfo        GYRO control information

    @return
        - @b E_OK: success
        - @b E_MACV: spi controller is busy
        - @b Else: fail
*/
static ER spi_configGyro(SPI_ID spiID, SPI_GYRO_INFO *pGyroInfo)
{
//    UINT32 uiReg;
	UINT32 uiSpiFreq;
	UINT32 uiOpDelay, uiTransDelay;
//    T_SPI_CTRL_REG ctrlReg;
	T_SPI_CONFIG_REG configReg;
//    T_SPI_STATUS_REG intStsReg = {0};
//    T_SPI_INTEN_REG intEnReg;
	T_SPI_GYROSEN_CONFIG_REG gyroConfigReg = {0};
	T_SPI_GYROSEN_OP_INTERVAL_REG opIntervalReg = {0};
	T_SPI_GYROSEN_TRS_INTERVAL_REG transIntervalReg = {0};
//    T_SPI_GYROSEN_TRS_INTEN_REG trsIntEnReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

	if (pll_getClockFreq(vSpiPllClkID[spiID], &uiSpiFreq) != E_OK) {
		DBG_ERR("spi%d get src freq fail\r\n", spiID);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiOpDelay = pGyroInfo->uiOpInterval;
	} else {
		uiOpDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiOpInterval + 999999) / 1000000;
		if ((((UINT64)uiOpDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiOpInterval) {
			DBG_WRN("expec OP delay is %d us, real delay will be %llu us\r\n",
					pGyroInfo->uiOpInterval, (((UINT64)uiOpDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiOpDelay > SPI_GYRO_OP_INTERVAL_MAX) {
		DBG_ERR("OP interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiOpInterval,
				SPI_GYRO_OP_INTERVAL_MAX);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiTransDelay = pGyroInfo->uiTransferInterval;
	} else {
		uiTransDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiTransferInterval + 999999) / 1000000;
		if ((((UINT64)uiTransDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiTransferInterval) {
			DBG_WRN("expect Transfer delay is %d us, real delay will be %llu us\r\n",
					pGyroInfo->uiTransferInterval, (((UINT64)uiTransDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiTransDelay > SPI_GYRO_TRANSFER_INTERVAL_MAX) {
		DBG_ERR("transfer interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiTransferInterval,
				SPI_GYRO_TRANSFER_INTERVAL_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferCount > SPI_GYRO_TRANSFER_MAX) ||
		(pGyroInfo->uiTransferCount < SPI_GYRO_TRANSFER_MIN)) {
		DBG_ERR("transfer count %d should between %d and %d\r\n",
				pGyroInfo->uiTransferCount,
				SPI_GYRO_TRANSFER_MIN,
				SPI_GYRO_TRANSFER_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen > SPI_GYRO_TRSLEN_MAX) ||
		(pGyroInfo->uiTransferLen < SPI_GYRO_TRSLEN_MIN)) {
		DBG_ERR("Transfer len %d should between %d OP and %d OP\r\n",
				pGyroInfo->uiTransferLen,
				SPI_GYRO_TRSLEN_MIN,
				SPI_GYRO_TRSLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiOp0Length > SPI_GYRO_OPLEN_MAX) ||
		(pGyroInfo->uiOp0Length < SPI_GYRO_OPLEN_MIN)) {
		DBG_ERR("OP0 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp0Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 2 &&
		((pGyroInfo->uiOp1Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp1Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP1 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp1Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 3 &&
		((pGyroInfo->uiOp2Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp2Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP2 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp2Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 4 &&
		((pGyroInfo->uiOp3Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp3Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP3 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp3Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

#if 0
	if (uiSpiGyroSyncEndOffset >= pGyroInfo->uiTransferCount) {
		DBG_ERR("You have already invoke spi_setConfig() to setup SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET as %d, but spi_startGyro() as %d\r\n",
				uiSpiGyroSyncEndOffset, pGyroInfo->uiTransferCount);
		return E_NOSPT;
	}
	uiSpiGyroSyncEndMask = 1 << (pGyroInfo->uiTransferCount - uiSpiGyroSyncEndOffset - 1);

	spi_waitClrFlag(spiID);
#endif
#if 0
#if (DRV_SUPPORT_IST == ENABLE)
	if (pGyroInfo->pEventHandler != NULL) {
		if (spiID == SPI_ID_1) {
			pfIstCB_SPI = (DRV_CB)pGyroInfo->pEventHandler;
		} else {
			pfIstCB_SPI3 = (DRV_CB)pGyroInfo->pEventHandler;
		}
	} else {
		if (spiID == SPI_ID_1) {
			pfIstCB_SPI = NULL;
		} else {
			pfIstCB_SPI3 = NULL;
		}
	}
#else
	if (pGyroInfo->pEventHandler != NULL) {
		vSpiGyroHdl[spiID] = pGyroInfo->pEventHandler;
	} else {
		vSpiGyroHdl[spiID] = NULL;
	}
#endif
#endif

//    spi_rstGyroQueue(spiID);

	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (pGyroInfo->gyroMode == SPI_GYRO_MODE_SIE_SYNC) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
		configReg.bit.spi_pkt_burst_pre_cond = 0;
		if (configReg.bit.spi_pkt_burst_post_cond == 0) {
			configReg.bit.spi_pkt_burst_handshake_en = 0;
		}
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_ONE_SHOT) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_ONESHOT;
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_FREE_RUN) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_FREERUN;
	} else { // SPI_GYRO_MODE_SIE_SYNC_WITH_RDY
		if (spiID == SPI_ID_3) {
            int rt;
			UINT32 uiPinmux;
            T_SPI_IO_REG ioReg;
            PIN_GROUP_CONFIG pinmux_cfg[1];

            pinmux_cfg[0].pin_function = PIN_FUNC_SPI;
        	pinmux_cfg[0].config = 0;
        	rt = nvt_pinmux_capture(pinmux_cfg, 1);
            if (rt) {
        		DBG_ERR("get pinmux fail %d\n", rt);
        		return E_NOSPT;
        	}

            uiPinmux = pinmux_cfg[0].config;
//			uiPinmux = pinmux_getPinmux(PIN_FUNC_SPI);
			if (!(uiPinmux & (PIN_SPI_CFG_CH3_RDY))) {
				DBG_ERR("SPI3_RDY not enabled in top_init()\r\n");
				return E_NOSPT;
			}

			configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
			// In addition, enable wait ready feature
			configReg.bit.spi_pkt_burst_handshake_en = 1;
			configReg.bit.spi_pkt_burst_pre_cond = 1;   // wait until SPI_RDY ready
			ioReg.reg = spi_getReg(spiID, SPI_IO_REG_OFS);
			ioReg.bit.spi_rdy_pol = spiRdyActiveLevel;
			spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);
		} else {
			DBG_ERR("SPI%d not support RDY PIN\r\n", spiID);
			return E_NOSPT;
		}
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	gyroConfigReg.bit.trscnt = pGyroInfo->uiTransferCount - 1;
	gyroConfigReg.bit.trslen = pGyroInfo->uiTransferLen - 1;
	gyroConfigReg.bit.len_op0 = pGyroInfo->uiOp0Length - 1;
	gyroConfigReg.bit.len_op1 = pGyroInfo->uiOp1Length - 1;
	gyroConfigReg.bit.len_op2 = pGyroInfo->uiOp2Length - 1;
	gyroConfigReg.bit.len_op3 = pGyroInfo->uiOp3Length - 1;
	gyroConfigReg.bit.gyro_vdsrc = vSpiGyroVdSrc[SPI_GYRO_ID_HASH(spiID)];
	spi_setReg(spiID, SPI_GYROSEN_CONFIG_REG_OFS, gyroConfigReg.reg);

	// SPI need 3T to see VD pulse
	// gyro need 3T to start
	// if both 3T, gyro may see false VD pulse when VD SRC is changed
	// insert 1T delay to make sure false VD pulse is generated before gyro started
	if (gyroConfigReg.bit.gyro_vdsrc != SPI_VD_SRC_SIE1) {
		UINT32 delay = 1000000 / uiSpiFreq;

		delay = (delay == 0) ? 1 : delay;
		vos_util_delay_us(delay);
	}

	opIntervalReg.bit.op_interval = uiOpDelay;
	spi_setReg(spiID, SPI_GYROSEN_OP_INTERVAL_REG_OFS, opIntervalReg.reg);

	transIntervalReg.bit.trs_interval = uiTransDelay;
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTERVAL_REG_OFS, transIntervalReg.reg);

#if 0
	uiReg = (pGyroInfo->vOp0OutData[3] << 24) | (pGyroInfo->vOp0OutData[2] << 16) |
			(pGyroInfo->vOp0OutData[1] << 8) | pGyroInfo->vOp0OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA1_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp0OutData[7] << 24) | (pGyroInfo->vOp0OutData[6] << 16) |
			(pGyroInfo->vOp0OutData[5] << 8) | pGyroInfo->vOp0OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA2_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp1OutData[3] << 24) | (pGyroInfo->vOp1OutData[2] << 16) |
			(pGyroInfo->vOp1OutData[1] << 8) | pGyroInfo->vOp1OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA3_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp1OutData[7] << 24) | (pGyroInfo->vOp1OutData[6] << 16) |
			(pGyroInfo->vOp1OutData[5] << 8) | pGyroInfo->vOp1OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA4_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp2OutData[3] << 24) | (pGyroInfo->vOp2OutData[2] << 16) |
			(pGyroInfo->vOp2OutData[1] << 8) | pGyroInfo->vOp2OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA5_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp2OutData[7] << 24) | (pGyroInfo->vOp2OutData[6] << 16) |
			(pGyroInfo->vOp2OutData[5] << 8) | pGyroInfo->vOp2OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA6_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp3OutData[3] << 24) | (pGyroInfo->vOp3OutData[2] << 16) |
			(pGyroInfo->vOp3OutData[1] << 8) | pGyroInfo->vOp3OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA7_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp3OutData[7] << 24) | (pGyroInfo->vOp3OutData[6] << 16) |
			(pGyroInfo->vOp3OutData[5] << 8) | pGyroInfo->vOp3OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA8_REG_OFS, uiReg);

	// clear interrupt status
	spi_setReg(spiID, SPI_GYROSEN_TRS_STS_REG_OFS, 0xFFFFFFFF);
	intStsReg.bit.gyro_trs_rdy_sts = 1;
	intStsReg.bit.gyro_overrun_sts = 1;
	intStsReg.bit.all_gyrotrs_done_sts = 1;
	intStsReg.bit.gyro_seq_err_sts = 1;
	intStsReg.bit.gyro_trs_timeout_sts = 1;
	spi_setReg(spiID, SPI_STATUS_REG_OFS, intStsReg.reg);

	// enable needed interrupt
	if (bSpiGyroUnitIsClk == TRUE) {
		trsIntEnReg.reg = uiSpiGyroIntMsk;
		DBG_IND("%s: INT mask 0x%lx\r\n", __func__, uiSpiGyroIntMsk);
	} else {
		// Gyro FIFO depth is 32 OP (64 word)
		// Rule: Interrupt per 24 OP
		UINT32 i;
		const UINT32 uiGyroIntPerTrs = SPI_GYRO_FIFO_THRESHOLD / pGyroInfo->uiTransferLen;

		trsIntEnReg.reg = 0;
		for (i = uiGyroIntPerTrs; i <= pGyroInfo->uiTransferCount; i += uiGyroIntPerTrs) {
			trsIntEnReg.reg |= 1 << (i - 1);
		}
		// Force alwasy int at transfer of sync end
		trsIntEnReg.reg |= uiSpiGyroSyncEndMask;
		// Force always int at last transfer done
		trsIntEnReg.reg |= 1 << (pGyroInfo->uiTransferCount - 1);
	}
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTEN_REG_OFS, trsIntEnReg.reg);
	intEnReg.reg = spi_getReg(spiID, SPI_INTEN_REG_OFS);
	intEnReg.bit.gyro_trs_rdy_en = 1;
	intEnReg.bit.gyro_overrun_en = 1;
	intEnReg.bit.all_gyrotrs_done_en = 1;
	intEnReg.bit.gyro_seq_err_en = 1;
	intEnReg.bit.gyro_trs_timeout_en = 1;
	spi_setReg(spiID, SPI_INTEN_REG_OFS, intEnReg.reg);

	vSpiState[spiID] = SPI_STATE_GYRO;

	// start GYRO polling
	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	ctrlReg.bit.spi_gyro_en = 1;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	drv_enableInt(vSpiIntNum[spiID]);
#endif
	return E_OK;
}

/**
    SPI start gyro

    Setup and trigger SPI to polling GYRO information.

    @param[in] spiID            SPI controller ID
    @param[in] pGyroInfo        GYRO control information

    @return
        - @b E_OK: success
        - @b E_MACV: spi controller is busy
        - @b Else: fail
*/
ER spi_startGyro(SPI_ID spiID, SPI_GYRO_INFO *pGyroInfo)
{
	UINT32 uiReg;
//    UINT32 uiSpiFreq;
//    UINT32 uiOpDelay, uiTransDelay;
	ER erReturn;
	T_SPI_CTRL_REG ctrlReg;
//    T_SPI_CONFIG_REG configReg;
	T_SPI_STATUS_REG intStsReg = {0};
	T_SPI_INTEN_REG intEnReg;
//    T_SPI_GYROSEN_CONFIG_REG gyroConfigReg = {0};
//    T_SPI_GYROSEN_OP_INTERVAL_REG opIntervalReg = {0};
//    T_SPI_GYROSEN_TRS_INTERVAL_REG transIntervalReg = {0};
	T_SPI_GYROSEN_TRS_INTEN_REG trsIntEnReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

#if 0
	if (pll_getClockFreq(vSpiPllClkID[spiID], &uiSpiFreq) != E_OK) {
		DBG_ERR("spi%d get src freq fail\r\n", spiID);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiOpDelay = pGyroInfo->uiOpInterval;
	} else {
		uiOpDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiOpInterval + 999999) / 1000000;
		if ((((UINT64)uiOpDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiOpInterval) {
			DBG_WRN("expec OP delay is %d us, real delay will be %d us\r\n",
					pGyroInfo->uiOpInterval, (((UINT64)uiOpDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiOpDelay > SPI_GYRO_OP_INTERVAL_MAX) {
		DBG_ERR("OP interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiOpInterval,
				SPI_GYRO_OP_INTERVAL_MAX);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiTransDelay = pGyroInfo->uiTransferInterval;
	} else {
		uiTransDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiTransferInterval + 999999) / 1000000;
		if ((((UINT64)uiTransDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiTransferInterval) {
			DBG_WRN("expect Transfer delay is %d us, real delay will be %d us\r\n",
					pGyroInfo->uiTransferInterval, (((UINT64)uiTransDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiTransDelay > SPI_GYRO_TRANSFER_INTERVAL_MAX) {
		DBG_ERR("transfer interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiTransferInterval,
				SPI_GYRO_TRANSFER_INTERVAL_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferCount > SPI_GYRO_TRANSFER_MAX) ||
		(pGyroInfo->uiTransferCount < SPI_GYRO_TRANSFER_MIN)) {
		DBG_ERR("transfer count %d should between %d and %d\r\n",
				pGyroInfo->uiTransferCount,
				SPI_GYRO_TRANSFER_MIN,
				SPI_GYRO_TRANSFER_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen > SPI_GYRO_TRSLEN_MAX) ||
		(pGyroInfo->uiTransferLen < SPI_GYRO_TRSLEN_MIN)) {
		DBG_ERR("Transfer len %d should between %d OP and %d OP\r\n",
				pGyroInfo->uiTransferLen,
				SPI_GYRO_TRSLEN_MIN,
				SPI_GYRO_TRSLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiOp0Length > SPI_GYRO_OPLEN_MAX) ||
		(pGyroInfo->uiOp0Length < SPI_GYRO_OPLEN_MIN)) {
		DBG_ERR("OP0 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp0Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 2 &&
		((pGyroInfo->uiOp1Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp1Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP1 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp1Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 3 &&
		((pGyroInfo->uiOp2Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp2Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP2 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp2Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 4 &&
		((pGyroInfo->uiOp3Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp3Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP3 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp3Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}
#endif
//    erReturn = spi_configGyro(spiID, pGyroInfo);
//    if (erReturn != E_OK)
//    {
//        return erReturn;
//    }

	if (vSpiGyroSyncEndOffset[SPI_GYRO_ID_HASH(spiID)] >= pGyroInfo->uiTransferCount) {
		DBG_ERR("You have already invoke spi_setConfig() to setup SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET as %d, but spi_startGyro() as %d\r\n",
				vSpiGyroSyncEndOffset[SPI_GYRO_ID_HASH(spiID)], pGyroInfo->uiTransferCount);
		return E_NOSPT;
	}

	spi_waitClrFlag(spiID);

	erReturn = spi_configGyro(spiID, pGyroInfo);
	if (erReturn != E_OK) {
		set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
		return erReturn;
	}

	vSpiGyroSyncEndMask[SPI_GYRO_ID_HASH(spiID)] = 1 << (pGyroInfo->uiTransferCount - vSpiGyroSyncEndOffset[SPI_GYRO_ID_HASH(spiID)] - 1);

#if 1
#if (DRV_SUPPORT_IST == ENABLE)
	if (pGyroInfo->pEventHandler != NULL) {
		pfIstCB_SPI3 = (DRV_CB)pGyroInfo->pEventHandler;
	} else {
		pfIstCB_SPI3 = NULL;
	}
#else
	if (pGyroInfo->pEventHandler != NULL) {
		vSpiGyroHdl[spiID] = pGyroInfo->pEventHandler;
	} else {
		vSpiGyroHdl[spiID] = NULL;
	}
#endif
#endif

	spi_rstGyroQueue(spiID);

#if 0
	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (pGyroInfo->gyroMode == SPI_GYRO_MODE_SIE_SYNC) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
		configReg.bit.spi_pkt_burst_pre_cond = 0;
		if (configReg.bit.spi_pkt_burst_post_cond == 0) {
			configReg.bit.spi_pkt_burst_handshake_en = 0;
		}
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_ONE_SHOT) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_ONESHOT;
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_FREE_RUN) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_FREERUN;
	} else { // SPI_GYRO_MODE_SIE_SYNC_WITH_RDY
		if (spiID == SPI_ID_3) {
			UINT32 uiPinmux;
			T_SPI_IO_REG ioReg;

			uiPinmux = pinmux_getPinmux(PIN_FUNC_SPI);
			if (!(uiPinmux & (PIN_SPI_CFG_CH3_RDY | PIN_SPI_CFG_CH3_RDY_2ND_PINMUX))) {
				DBG_ERR("SPI3_RDY not enabled in top_init()\r\n");
				return E_NOSPT;
			}

			configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
			// In addition, enable wait ready feature
			configReg.bit.spi_pkt_burst_handshake_en = 1;
			configReg.bit.spi_pkt_burst_pre_cond = 1;   // wait until SPI_RDY ready
			ioReg.reg = spi_getReg(spiID, SPI_IO_REG_OFS);
			ioReg.bit.spi_rdy_pol = spiRdyActiveLevel;
			spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);
		} else {
			DBG_ERR("SPI%d not support RDY PIN\r\n", spiID);
			return E_NOSPT;
		}
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	gyroConfigReg.bit.trscnt = pGyroInfo->uiTransferCount - 1;
	gyroConfigReg.bit.trslen = pGyroInfo->uiTransferLen - 1;
	gyroConfigReg.bit.len_op0 = pGyroInfo->uiOp0Length - 1;
	gyroConfigReg.bit.len_op1 = pGyroInfo->uiOp1Length - 1;
	gyroConfigReg.bit.len_op2 = pGyroInfo->uiOp2Length - 1;
	gyroConfigReg.bit.len_op3 = pGyroInfo->uiOp3Length - 1;
	gyroConfigReg.bit.gyro_vdsrc = vSpiGyroVdSrc[SPI_GYRO_ID_HASH(spiID)];
	spi_setReg(spiID, SPI_GYROSEN_CONFIG_REG_OFS, gyroConfigReg.reg);

	opIntervalReg.bit.op_interval = uiOpDelay;
	spi_setReg(spiID, SPI_GYROSEN_OP_INTERVAL_REG_OFS, opIntervalReg.reg);

	transIntervalReg.bit.trs_interval = uiTransDelay;
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTERVAL_REG_OFS, transIntervalReg.reg);
#endif
	uiReg = (pGyroInfo->vOp0OutData[3] << 24) | (pGyroInfo->vOp0OutData[2] << 16) |
			(pGyroInfo->vOp0OutData[1] << 8) | pGyroInfo->vOp0OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA1_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp0OutData[7] << 24) | (pGyroInfo->vOp0OutData[6] << 16) |
			(pGyroInfo->vOp0OutData[5] << 8) | pGyroInfo->vOp0OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA2_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp1OutData[3] << 24) | (pGyroInfo->vOp1OutData[2] << 16) |
			(pGyroInfo->vOp1OutData[1] << 8) | pGyroInfo->vOp1OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA3_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp1OutData[7] << 24) | (pGyroInfo->vOp1OutData[6] << 16) |
			(pGyroInfo->vOp1OutData[5] << 8) | pGyroInfo->vOp1OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA4_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp2OutData[3] << 24) | (pGyroInfo->vOp2OutData[2] << 16) |
			(pGyroInfo->vOp2OutData[1] << 8) | pGyroInfo->vOp2OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA5_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp2OutData[7] << 24) | (pGyroInfo->vOp2OutData[6] << 16) |
			(pGyroInfo->vOp2OutData[5] << 8) | pGyroInfo->vOp2OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA6_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp3OutData[3] << 24) | (pGyroInfo->vOp3OutData[2] << 16) |
			(pGyroInfo->vOp3OutData[1] << 8) | pGyroInfo->vOp3OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA7_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp3OutData[7] << 24) | (pGyroInfo->vOp3OutData[6] << 16) |
			(pGyroInfo->vOp3OutData[5] << 8) | pGyroInfo->vOp3OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA8_REG_OFS, uiReg);

	// clear interrupt status
	spi_setReg(spiID, SPI_GYROSEN_TRS_STS_REG_OFS, 0xFFFFFFFF);
	intStsReg.bit.gyro_trs_rdy_sts = 1;
	intStsReg.bit.gyro_overrun_sts = 1;
	intStsReg.bit.all_gyrotrs_done_sts = 1;
	intStsReg.bit.gyro_seq_err_sts = 1;
	intStsReg.bit.gyro_trs_timeout_sts = 1;
	spi_setReg(spiID, SPI_STATUS_REG_OFS, intStsReg.reg);

	// enable needed interrupt
	if (bSpiGyroUnitIsClk == TRUE) {
		trsIntEnReg.reg = uiSpiGyroIntMsk;
		DBG_IND("%s: INT mask 0x%lx\r\n", __func__, uiSpiGyroIntMsk);
	} else {
		// Gyro FIFO depth is 32 OP (64 word)
		// Rule: Interrupt per 24 OP
		UINT32 i;
		const UINT32 uiGyroIntPerTrs = SPI_GYRO_FIFO_THRESHOLD / pGyroInfo->uiTransferLen;

		trsIntEnReg.reg = 0;
		for (i = uiGyroIntPerTrs; i <= pGyroInfo->uiTransferCount; i += uiGyroIntPerTrs) {
			trsIntEnReg.reg |= 1 << (i - 1);
		}
		// Force alwasy int at transfer of sync end
		trsIntEnReg.reg |= vSpiGyroSyncEndMask[SPI_GYRO_ID_HASH(spiID)];
		// Force always int at last transfer done
		trsIntEnReg.reg |= 1 << (pGyroInfo->uiTransferCount - 1);
	}
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTEN_REG_OFS, trsIntEnReg.reg);
	intEnReg.reg = spi_getReg(spiID, SPI_INTEN_REG_OFS);
	intEnReg.bit.gyro_trs_rdy_en = 1;
	intEnReg.bit.gyro_overrun_en = 1;
	intEnReg.bit.all_gyrotrs_done_en = 1;
	intEnReg.bit.gyro_seq_err_en = 1;
	intEnReg.bit.gyro_trs_timeout_en = 1;
	spi_setReg(spiID, SPI_INTEN_REG_OFS, intEnReg.reg);

	vSpiState[spiID] = SPI_STATE_GYRO;
	vSpiGyroState[SPI_GYRO_ID_HASH(spiID)] = SPI_GYRO_STATE_RUN;

	// start GYRO polling
	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	ctrlReg.bit.spi_gyro_en = 1;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

//	drv_enableInt(vSpiIntNum[spiID]);

	return E_OK;
}

/**
    SPI update gyro

    Setup and trigger SPI to polling GYRO information.

    @param[in] spiID            SPI controller ID
    @param[in] pGyroInfo        GYRO control information
    @param[in] uiSyncEndOffset  sync end offset (SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET) for new gyro setting

    @return
        - @b E_OK: success
        - @b E_MACV: spi controller is busy
        - @b Else: fail
*/
ER spi_changeGyro(SPI_ID spiID, SPI_GYRO_INFO *pGyroInfo, UINT32 uiSyncEndOffset)
{
//    UINT32 uiReg;
//    UINT32 uiSpiFreq;
//    UINT32 uiOpDelay, uiTransDelay;
	ER erReturn;
	UINT32 trCount = 0, opCount = 0;
	UINT32 flags;
	T_SPI_CTRL_REG ctrlReg;
//    T_SPI_CONFIG_REG configReg;
//    T_SPI_STATUS_REG intStsReg = {0};
//    T_SPI_INTEN_REG intEnReg;
//    T_SPI_GYROSEN_CONFIG_REG gyroConfigReg = {0};
//    T_SPI_GYROSEN_OP_INTERVAL_REG opIntervalReg = {0};
//    T_SPI_GYROSEN_TRS_INTERVAL_REG transIntervalReg = {0};
	T_SPI_GYROSEN_CONFIG_REG gyroConfigReg = {0};
	T_SPI_GYROSEN_TRS_INTEN_REG trsIntEnReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

#if 0
	if (pll_getClockFreq(vSpiPllClkID[spiID], &uiSpiFreq) != E_OK) {
		DBG_ERR("spi%d get src freq fail\r\n", spiID);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiOpDelay = pGyroInfo->uiOpInterval;
	} else {
		uiOpDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiOpInterval + 999999) / 1000000;
		if ((((UINT64)uiOpDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiOpInterval) {
			DBG_WRN("expec OP delay is %d us, real delay will be %d us\r\n",
					pGyroInfo->uiOpInterval, (((UINT64)uiOpDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiOpDelay > SPI_GYRO_OP_INTERVAL_MAX) {
		DBG_ERR("OP interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiOpInterval,
				SPI_GYRO_OP_INTERVAL_MAX);
		return E_NOSPT;
	}

	if (bSpiGyroUnitIsClk == TRUE) {
		uiTransDelay = pGyroInfo->uiTransferInterval;
	} else {
		uiTransDelay = (((UINT64)uiSpiFreq) * pGyroInfo->uiTransferInterval + 999999) / 1000000;
		if ((((UINT64)uiTransDelay) * 1000000 / uiSpiFreq) != pGyroInfo->uiTransferInterval) {
			DBG_WRN("expect Transfer delay is %d us, real delay will be %d us\r\n",
					pGyroInfo->uiTransferInterval, (((UINT64)uiTransDelay) * 1000000 / uiSpiFreq));
		}
	}

	if (uiTransDelay > SPI_GYRO_TRANSFER_INTERVAL_MAX) {
		DBG_ERR("transfer interval 0x%x exceed MAX 0x%x\r\n",
				pGyroInfo->uiTransferInterval,
				SPI_GYRO_TRANSFER_INTERVAL_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferCount > SPI_GYRO_TRANSFER_MAX) ||
		(pGyroInfo->uiTransferCount < SPI_GYRO_TRANSFER_MIN)) {
		DBG_ERR("transfer count %d should between %d and %d\r\n",
				pGyroInfo->uiTransferCount,
				SPI_GYRO_TRANSFER_MIN,
				SPI_GYRO_TRANSFER_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen > SPI_GYRO_TRSLEN_MAX) ||
		(pGyroInfo->uiTransferLen < SPI_GYRO_TRSLEN_MIN)) {
		DBG_ERR("Transfer len %d should between %d OP and %d OP\r\n",
				pGyroInfo->uiTransferLen,
				SPI_GYRO_TRSLEN_MIN,
				SPI_GYRO_TRSLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiOp0Length > SPI_GYRO_OPLEN_MAX) ||
		(pGyroInfo->uiOp0Length < SPI_GYRO_OPLEN_MIN)) {
		DBG_ERR("OP0 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp0Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 2 &&
		((pGyroInfo->uiOp1Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp1Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP1 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp1Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 3 &&
		((pGyroInfo->uiOp2Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp2Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP2 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp2Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}

	if ((pGyroInfo->uiTransferLen) >= 4 &&
		((pGyroInfo->uiOp3Length > SPI_GYRO_OPLEN_MAX) ||
		 (pGyroInfo->uiOp3Length < SPI_GYRO_OPLEN_MIN))) {
		DBG_ERR("OP3 Len %d should between %d and %d\r\n",
				pGyroInfo->uiOp3Length,
				SPI_GYRO_OPLEN_MIN,
				SPI_GYRO_OPLEN_MAX);
		return E_NOSPT;
	}
#endif
//    erReturn = spi_configGyro(spiID, pGyroInfo);
//    if (erReturn != E_OK)
//    {
//        return erReturn;
//    }

	if (uiSyncEndOffset >= pGyroInfo->uiTransferCount) {
		DBG_ERR("Input uiSyncEndOffset is %d, but transfer count is %d\r\n",
				uiSyncEndOffset, pGyroInfo->uiTransferCount);
		return E_NOSPT;
	}

	if (vSpiGyroState[SPI_GYRO_ID_HASH(spiID)] != SPI_GYRO_STATE_RUN) {
		DBG_ERR("Gyro mode is not running (0x%lx), please use spi_startGyro()\r\n", vSpiGyroState[SPI_GYRO_ID_HASH(spiID)]);
		return E_SYS;
	}

//    spi_waitClrFlag(spiID);

	// backup previous gyro setting
	gyroConfigReg.reg = spi_getReg(spiID, SPI_GYROSEN_CONFIG_REG_OFS);

	erReturn = spi_configGyro(spiID, pGyroInfo);
	if (erReturn != E_OK) {
//        set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
		return erReturn;
	}

	vSpiGyroSyncNextEndMask[SPI_GYRO_ID_HASH(spiID)] = 1 << (pGyroInfo->uiTransferCount - uiSyncEndOffset - 1);

#if 0
#if (DRV_SUPPORT_IST == ENABLE)
	if (pGyroInfo->pEventHandler != NULL) {
		if (spiID == SPI_ID_1) {
			pfIstCB_SPI = (DRV_CB)pGyroInfo->pEventHandler;
		} else {
			pfIstCB_SPI3 = (DRV_CB)pGyroInfo->pEventHandler;
		}
	} else {
		if (spiID == SPI_ID_1) {
			pfIstCB_SPI = NULL;
		} else {
			pfIstCB_SPI3 = NULL;
		}
	}
#else
	if (pGyroInfo->pEventHandler != NULL) {
		vSpiGyroHdl[spiID] = pGyroInfo->pEventHandler;
	} else {
		vSpiGyroHdl[spiID] = NULL;
	}
#endif
#endif

//    spi_rstGyroQueue(spiID);

#if 0
	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (pGyroInfo->gyroMode == SPI_GYRO_MODE_SIE_SYNC) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
		configReg.bit.spi_pkt_burst_pre_cond = 0;
		if (configReg.bit.spi_pkt_burst_post_cond == 0) {
			configReg.bit.spi_pkt_burst_handshake_en = 0;
		}
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_ONE_SHOT) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_ONESHOT;
	} else if (pGyroInfo->gyroMode == SPI_GYRO_MODE_FREE_RUN) {
		configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_FREERUN;
	} else { // SPI_GYRO_MODE_SIE_SYNC_WITH_RDY
		if (spiID == SPI_ID_3) {
			UINT32 uiPinmux;
			T_SPI_IO_REG ioReg;

			uiPinmux = pinmux_getPinmux(PIN_FUNC_SPI);
			if (!(uiPinmux & (PIN_SPI_CFG_CH3_RDY | PIN_SPI_CFG_CH3_RDY_2ND_PINMUX))) {
				DBG_ERR("SPI3_RDY not enabled in top_init()\r\n");
				return E_NOSPT;
			}

			configReg.bit.spi_gyro_mode = SPI_GYRO_MODE_ENUM_SIETRIG;
			// In addition, enable wait ready feature
			configReg.bit.spi_pkt_burst_handshake_en = 1;
			configReg.bit.spi_pkt_burst_pre_cond = 1;   // wait until SPI_RDY ready
			ioReg.reg = spi_getReg(spiID, SPI_IO_REG_OFS);
			ioReg.bit.spi_rdy_pol = spiRdyActiveLevel;
			spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);
		} else {
			DBG_ERR("SPI%d not support RDY PIN\r\n", spiID);
			return E_NOSPT;
		}
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	gyroConfigReg.bit.trscnt = pGyroInfo->uiTransferCount - 1;
	gyroConfigReg.bit.trslen = pGyroInfo->uiTransferLen - 1;
	gyroConfigReg.bit.len_op0 = pGyroInfo->uiOp0Length - 1;
	gyroConfigReg.bit.len_op1 = pGyroInfo->uiOp1Length - 1;
	gyroConfigReg.bit.len_op2 = pGyroInfo->uiOp2Length - 1;
	gyroConfigReg.bit.len_op3 = pGyroInfo->uiOp3Length - 1;
	gyroConfigReg.bit.gyro_vdsrc = vSpiGyroVdSrc[SPI_GYRO_ID_HASH(spiID)];
	spi_setReg(spiID, SPI_GYROSEN_CONFIG_REG_OFS, gyroConfigReg.reg);

	opIntervalReg.bit.op_interval = uiOpDelay;
	spi_setReg(spiID, SPI_GYROSEN_OP_INTERVAL_REG_OFS, opIntervalReg.reg);

	transIntervalReg.bit.trs_interval = uiTransDelay;
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTERVAL_REG_OFS, transIntervalReg.reg);
#endif
#if 0
	uiReg = (pGyroInfo->vOp0OutData[3] << 24) | (pGyroInfo->vOp0OutData[2] << 16) |
			(pGyroInfo->vOp0OutData[1] << 8) | pGyroInfo->vOp0OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA1_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp0OutData[7] << 24) | (pGyroInfo->vOp0OutData[6] << 16) |
			(pGyroInfo->vOp0OutData[5] << 8) | pGyroInfo->vOp0OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA2_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp1OutData[3] << 24) | (pGyroInfo->vOp1OutData[2] << 16) |
			(pGyroInfo->vOp1OutData[1] << 8) | pGyroInfo->vOp1OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA3_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp1OutData[7] << 24) | (pGyroInfo->vOp1OutData[6] << 16) |
			(pGyroInfo->vOp1OutData[5] << 8) | pGyroInfo->vOp1OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA4_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp2OutData[3] << 24) | (pGyroInfo->vOp2OutData[2] << 16) |
			(pGyroInfo->vOp2OutData[1] << 8) | pGyroInfo->vOp2OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA5_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp2OutData[7] << 24) | (pGyroInfo->vOp2OutData[6] << 16) |
			(pGyroInfo->vOp2OutData[5] << 8) | pGyroInfo->vOp2OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA6_REG_OFS, uiReg);

	uiReg = (pGyroInfo->vOp3OutData[3] << 24) | (pGyroInfo->vOp3OutData[2] << 16) |
			(pGyroInfo->vOp3OutData[1] << 8) | pGyroInfo->vOp3OutData[0];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA7_REG_OFS, uiReg);
	uiReg = (pGyroInfo->vOp3OutData[7] << 24) | (pGyroInfo->vOp3OutData[6] << 16) |
			(pGyroInfo->vOp3OutData[5] << 8) | pGyroInfo->vOp3OutData[4];
	spi_setReg(spiID, SPI_GYROSEN_TX_DATA8_REG_OFS, uiReg);

	// clear interrupt status
	spi_setReg(spiID, SPI_GYROSEN_TRS_STS_REG_OFS, 0xFFFFFFFF);
	intStsReg.bit.gyro_trs_rdy_sts = 1;
	intStsReg.bit.gyro_overrun_sts = 1;
	intStsReg.bit.all_gyrotrs_done_sts = 1;
	intStsReg.bit.gyro_seq_err_sts = 1;
	intStsReg.bit.gyro_trs_timeout_sts = 1;
	spi_setReg(spiID, SPI_STATUS_REG_OFS, intStsReg.reg);
#endif
	// enable needed interrupt
	if (bSpiGyroUnitIsClk == TRUE) {
		trsIntEnReg.reg = uiSpiGyroIntMsk;
		DBG_IND("%s: INT mask 0x%lx\r\n", __func__, uiSpiGyroIntMsk);
	} else {
		// Gyro FIFO depth is 32 OP (64 word)
		// Rule: Interrupt per 24 OP
		UINT32 i;
		const UINT32 uiGyroIntPerTrs = SPI_GYRO_FIFO_THRESHOLD / pGyroInfo->uiTransferLen;

		trsIntEnReg.reg = 0;
		for (i = uiGyroIntPerTrs; i <= pGyroInfo->uiTransferCount; i += uiGyroIntPerTrs) {
			trsIntEnReg.reg |= 1 << (i - 1);
		}
		// Force alwasy int at transfer of sync end
		trsIntEnReg.reg |= vSpiGyroSyncNextEndMask[SPI_GYRO_ID_HASH(spiID)];
		// Force always int at last transfer done
		trsIntEnReg.reg |= 1 << (pGyroInfo->uiTransferCount - 1);
	}
	// Save gyro interrupt enable for later ISR reference
	vSpiGyroSyncNextIntEnReg[SPI_GYRO_ID_HASH(spiID)] = trsIntEnReg.reg;
#if 0
	spi_setReg(spiID, SPI_GYROSEN_TRS_INTEN_REG_OFS, trsIntEnReg.reg);
	intEnReg.reg = spi_getReg(spiID, SPI_INTEN_REG_OFS);
	intEnReg.bit.gyro_trs_rdy_en = 1;
	intEnReg.bit.gyro_overrun_en = 1;
	intEnReg.bit.all_gyrotrs_done_en = 1;
	intEnReg.bit.gyro_seq_err_en = 1;
	intEnReg.bit.gyro_trs_timeout_en = 1;
	spi_setReg(spiID, SPI_INTEN_REG_OFS, intEnReg.reg);
#endif

	spi_readGyroCounter(spiID, &trCount, &opCount);

	// Enter critical section
	vk_spin_lock_irqsave(&spi_spinlock, flags);

//    vSpiState[spiID] = SPI_STATE_GYRO;
	if (trCount == ((UINT32)gyroConfigReg.bit.trscnt + 1)) {
		// If change at blanking region of a VD
		// must change interrupt enable register immediately
		vSpiGyroSyncEndMask[SPI_GYRO_ID_HASH(spiID)] = vSpiGyroSyncNextEndMask[SPI_GYRO_ID_HASH(spiID)];
		spi_setReg(spiID, SPI_GYROSEN_TRS_INTEN_REG_OFS,
				   vSpiGyroSyncNextIntEnReg[SPI_GYRO_ID_HASH(spiID)]);
	} else {
		vSpiGyroState[SPI_GYRO_ID_HASH(spiID)] = SPI_GYRO_STATE_CHANGE_ISSUED;
	}

	// change GYRO setting
	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	ctrlReg.bit.spi_gyro_update = 1;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	// Leave critical section
	vk_spin_unlock_irqrestore(&spi_spinlock, flags);

	if (trCount > (UINT32)gyroConfigReg.bit.trscnt / 2) {
		DBG_WRN("Not change at 1st half of a VD, trs %d, op %d\r\n", trCount, opCount);
	}

	if (trCount == ((UINT32)gyroConfigReg.bit.trscnt + 1)) {
		DBG_IND("force trig ist, trs %d, op %d\r\n", trCount, opCount);
#if (DRV_SUPPORT_IST == ENABLE)
		switch (spiID) {
		case SPI_ID_3:
		default:
			if (pfIstCB_SPI3 != NULL) {
				kick_bh(INT_ID_SPI3, SPI_GYRO_INT_CHANGE_END, NULL);
//				drv_setIstEvent(DRV_IST_MODULE_SPI3, SPI_GYRO_INT_CHANGE_END);
			}
			break;
		}
#else
		if (vSpiGyroHdl != NULL) {
			vSpiGyroHdl[spiID](SPI_GYRO_INT_CHANGE_END);
		}
#endif
	}

	return E_OK;
}


/**
    SPI stop gyro

    Stop SPI from polling GYRO information.

    @param[in] spiID            SPI controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_stopGyro(SPI_ID spiID)
{
	T_SPI_CTRL_REG ctrlReg;
	T_SPI_CONFIG_REG configReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (vSpiState[spiID] != SPI_STATE_GYRO) {
		DBG_WRN("spi%d: not under gyro state\r\n", spiID);
		return E_CTX;
	}

	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	if (ctrlReg.bit.spi_gyro_sts) { // If gyro is ongoing, stop it
		ctrlReg.bit.spi_gyro_dis = 1;
		spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
#if _FPGA_EMULATION_
#if 0
		Perf_Open();
		Perf_Mark();
#endif
#endif
		do {
			ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		} while (ctrlReg.bit.spi_gyro_sts);
#if _FPGA_EMULATION_
#if 0
		DBG_MSG("%s: elps %d us\r\n", __func__, Perf_GetDuration());
		Perf_Close();
#endif
#endif
	} else {
#if _FPGA_EMULATION_
		DBG_MSG("%s: abort when gyro is under stop state\r\n", __func__);
#endif
	}

	// Disable handshake mode (in order not to effect PIO, flash polling)
	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	configReg.bit.spi_pkt_burst_pre_cond = 0;
	configReg.bit.spi_pkt_burst_post_cond = 0;
	configReg.bit.spi_pkt_burst_handshake_en = 0;
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

//	drv_disableInt(vSpiIntNum[spiID]);

	vSpiState[spiID] = SPI_STATE_IDLE;
	vSpiGyroState[SPI_GYRO_ID_HASH(spiID)] = SPI_GYRO_STATE_IDLE;

	pfIstCB_SPI3 = NULL;

	set_flg(FLG_ID_SPI, vSpiFlags[spiID]);

	return E_OK;
}

/*
    SPI get gyro fifo count

    Get FIFO count in gyro mode.
    @note This API is interrupt safe.

    @param[in] spiID            SPI controller ID
    @param[in] puiCnt           received word count in gyro mode

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_getGyroFifoCnt(SPI_ID spiID, UINT32 *puiCnt)
{
	T_SPI_GYROSEN_FIFO_STS_REG fifoStsReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

	if (puiCnt != NULL) {
		fifoStsReg.reg = spi_getReg(spiID, SPI_GYROSEN_FIFO_STS_REG_OFS);
		*puiCnt = fifoStsReg.bit.valid_entry;
	}

	return E_OK;
}

/*
    SPI read gyro fifo

    Read gyro data from FIFO.

    @param[in] spiID            SPI controller ID
    @param[in] uiCnt            FIFO count to read (unit: word)
    @param[out] pBuf            buffer to store read data

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_readGyroFifo(SPI_ID spiID, UINT32 uiCnt, UINT32 *pBuf)
{
	UINT32 i;
	UINT32 uiFifoCount = 0;
	T_SPI_GYROSEN_RX_DATA_REG dataReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (pBuf == NULL) {
		DBG_ERR("input buffer NULL\r\n");
		return E_PAR;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

	spi_getGyroFifoCnt(spiID, &uiFifoCount);
	if (uiCnt > uiFifoCount) {
		DBG_WRN("current fifo count %d is smaller than expected count %d\r\n", uiFifoCount, uiCnt);
		uiCnt = uiFifoCount;
	}

	for (i = 0; i < uiCnt; i++) {
		dataReg.reg = spi_getReg(spiID, SPI_GYROSEN_RX_DATA_REG_OFS);
		pBuf[i] = dataReg.reg;
	}

	return E_OK;
}

/*
    SPI read gyro counter


    @param[in] spiID            SPI controller ID
    @param[out] puiTransfer     pointer to return transfer counter
    @param[out] puiOp           pointer to return OP counter

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_readGyroCounter(SPI_ID spiID, UINT32 *puiTransfer, UINT32 *puiOp)
{
	T_SPI_GYROSEN_COUNTER_REG counterReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (puiTransfer == NULL) {
		DBG_ERR("input transfer buffer NULL\r\n");
		return E_PAR;
	}

	if (puiOp == NULL) {
		DBG_ERR("input OP buffer NULL\r\n");
		return E_PAR;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_NOSPT;
	}

	spi_getReg(spiID, SPI_GYROSEN_COUNTER_REG_OFS);
	counterReg.reg = spi_getReg(spiID, SPI_GYROSEN_COUNTER_REG_OFS);
	*puiTransfer = counterReg.bit.transfer_counter;
	*puiOp = counterReg.bit.op_counter;

	return E_OK;
}

#if 0
/**
    SPI IO Control

    @param[in] spiID            SPI controller ID
    @param[in] ctrlID           SPI IO control ID
    @param[in] context          context for ctrlID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_ioctrl(SPI_ID spiID, SPI_IOCTRL ctrlID, UINT32 context)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	return E_OK;
}
#endif

/**
    Configure SPI IO

    @param[in] spiID            SPI controller ID
    @param[in] configID         Configuration identifier
    @param[in] configContext    context for configID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_setConfig(SPI_ID spiID, SPI_CONFIG_ID configID, UINT32 configContext)
{
	T_SPI_DLY_CHAIN_REG dlyReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}
	if (nvt_get_chip_id() == CHIP_NA51055) {
		// NT98520 only has 3 SPI
		if (spiID >= SPI_ID_4) {
			return E_ID;
		}
	}

	switch (configID) {
	case SPI_CONFIG_ID_BUSMODE:
		vSpiInitInfo[spiID].spiMODE = configContext;
		break;
	case SPI_CONFIG_ID_FREQ:
		if (configContext > 48000000) {
			DBG_WRN("SPI_ID_%d upper limit is 48MHz, not %d Hz\r\n",
					spiID + 1,
					configContext);
			configContext = 48000000;
		}
		vSpiInitInfo[spiID].uiFreq = configContext;
		break;
	case SPI_CONFIG_ID_MSB_LSB:
		if (configContext == SPI_MSB) {
			vSpiInitInfo[spiID].bLSB = FALSE;
		} else {
			vSpiInitInfo[spiID].bLSB = TRUE;
		}
		break;
	case SPI_CONFIG_ID_WIDE_BUS_ORDER:
		vSpiInitInfo[spiID].wideBusOrder = configContext;
		break;
	case SPI_CONFIG_ID_CS_ACT_LEVEL:
		if (configContext == SPI_CS_ACT_LEVEL_LOW) {
			vSpiInitInfo[spiID].bCSActiveLow = TRUE;
		} else {
			vSpiInitInfo[spiID].bCSActiveLow = FALSE;
		}
		break;
	case SPI_CONFIG_ID_CS_CK_DLY:
		vSpiInitInfo[spiID].uiCsCkDelay = configContext;
		break;
	case SPI_CONFIG_ID_PKT_DLY:
		vSpiInitInfo[spiID].uiPktDelay = configContext;
		break;
	case SPI_CONFIG_ID_DO_HZ_EN:
		vbSpiDoHiZCtrl[spiID] = configContext;
		break;
	case SPI_CONFIG_ID_AUTOPINMUX:
		DBG_WRN("%s: not support SPI_CONFIG_ID_AUTOPINMUX\r\n", __func__);
		return E_NOSPT;
//		vbSpiAutoPinmux[spiID] = configContext;
		break;
	case SPI_CONFIG_ID_VD_SRC:
		vSpiGyroVdSrc[SPI_GYRO_ID_HASH(spiID)] = configContext;
		break;
	case SPI_CONFIG_ID_ENG_PKT_COUNT:
		vSpiEngPktCnt[spiID] = configContext;
		break;
	case SPI_CONFIG_ID_RDY_POLARITY:
		spiRdyActiveLevel = configContext;
		break;
	case SPI_CONFIG_ID_ENG_MSB_LSB: {
			T_SPI_CONFIG_REG configReg;

			configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
			if (configContext == SPI_MSB) {
				configReg.bit.spi_pkt_lsb_mode = FALSE;
			} else {
				configReg.bit.spi_pkt_lsb_mode = TRUE;
			}
			spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);
		}
		break;
	case SPI_CONFIG_ID_ENG_DMA_ABORT:
		bSpiTestDmaAbort = configContext;
		break;
	case SPI_CONFIG_ID_ENG_GYRO_UNIT:
		bSpiGyroUnitIsClk = configContext;
		break;
	case SPI_CONFIG_ID_ENG_GYRO_INTMSK:
		uiSpiGyroIntMsk = configContext;
		break;
	case SPI_CONFIG_ID_LATCH_CLK_SHIFT:
		if (configContext > SPI_LATCH_CLK_2T) {
			DBG_WRN("configID 0x%lx with invalid configContext 0x%lx, force it to 2T\r\n", SPI_CONFIG_ID_LATCH_CLK_SHIFT, configContext);
			configContext = SPI_LATCH_CLK_2T;
		}
		dlyReg.reg = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
		dlyReg.bit.latch_clk_shift = vuiSpiLatchClkShift[spiID] = configContext;
		spi_setReg(spiID, SPI_DLY_CHAIN_REG_OFS, dlyReg.reg);
		break;
	case SPI_CONFIG_ID_LATCH_CLK_EDGE:
		if (configContext > SPI_LATCH_EDGE_FALLING) {
			DBG_WRN("configID 0x%lx with invalid configContext 0x%lx, force it to rising edge\r\n", SPI_CONFIG_ID_LATCH_CLK_EDGE, configContext);
			configContext = SPI_LATCH_EDGE_RISING;
		}
		dlyReg.reg = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
		dlyReg.bit.latch_clk_edge = configContext;
		spi_setReg(spiID, SPI_DLY_CHAIN_REG_OFS, dlyReg.reg);
		break;
	case SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET:
		vSpiGyroSyncEndOffset[SPI_GYRO_ID_HASH(spiID)] = configContext;
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Query SPI configuration

    @param[in] spiID            SPI controller ID
    @param[in] configID         Configuration identifier
    @param[in] pConfigContext   returning context for configID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_getConfig(SPI_ID spiID, SPI_CONFIG_ID configID, UINT32 *pConfigContext)
{
	T_SPI_DLY_CHAIN_REG dlyReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}
	if (nvt_get_chip_id() == CHIP_NA51055) {
		// NT98520 only has 3 SPI
		if (spiID >= SPI_ID_4) {
			return E_ID;
		}
	}

	switch (configID) {
	case SPI_CONFIG_ID_BUSMODE:
		*pConfigContext = vSpiInitInfo[spiID].spiMODE;
		break;
	case SPI_CONFIG_ID_FREQ:
		*pConfigContext = vSpiInitInfo[spiID].uiFreq;
		break;
	case SPI_CONFIG_ID_MSB_LSB:
		*pConfigContext = vSpiInitInfo[spiID].bLSB;
		break;
	case SPI_CONFIG_ID_WIDE_BUS_ORDER:
		*pConfigContext = vSpiInitInfo[spiID].wideBusOrder;
		break;
	case SPI_CONFIG_ID_CS_ACT_LEVEL:
		if (vSpiInitInfo[spiID].bCSActiveLow == TRUE) {
			*pConfigContext = SPI_CS_ACT_LEVEL_LOW;
		} else {
			*pConfigContext = SPI_CS_ACT_LEVEL_HIGH;
		}
		break;
	case SPI_CONFIG_ID_CS_CK_DLY:
		*pConfigContext = vSpiInitInfo[spiID].uiCsCkDelay;
		break;
	case SPI_CONFIG_ID_PKT_DLY:
		*pConfigContext = vSpiInitInfo[spiID].uiPktDelay;
		break;
	case SPI_CONFIG_ID_DO_HZ_EN:
		*pConfigContext = vbSpiDoHiZCtrl[spiID];
		break;
	case SPI_CONFIG_ID_AUTOPINMUX:
		*pConfigContext = vbSpiAutoPinmux[spiID];
		break;
	case SPI_CONFIG_ID_ENG_PKT_COUNT:
		*pConfigContext = vSpiEngPktCnt[spiID];
		break;
	case SPI_CONFIG_ID_RDY_POLARITY:
		*pConfigContext = spiRdyActiveLevel;
		break;
	case SPI_CONFIG_ID_ENG_MSB_LSB: {
			T_SPI_CONFIG_REG configReg;

			configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
			if (configReg.bit.spi_pkt_lsb_mode == FALSE) {
				*pConfigContext = SPI_MSB;
			} else {
				*pConfigContext = SPI_LSB;
			}
		}
		break;
	case SPI_CONFIG_ID_ENG_GYRO_UNIT:
		*pConfigContext = bSpiGyroUnitIsClk;
		break;
	case SPI_CONFIG_ID_ENG_GYRO_INTMSK:
		*pConfigContext = uiSpiGyroIntMsk;
		break;
	case SPI_CONFIG_ID_LATCH_CLK_SHIFT:
		dlyReg.reg = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
		*pConfigContext = dlyReg.bit.latch_clk_shift;
		break;
	case SPI_CONFIG_ID_LATCH_CLK_EDGE:
		dlyReg.reg = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
		*pConfigContext = dlyReg.bit.latch_clk_edge;
		break;
	case SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET:
		*pConfigContext = vSpiGyroSyncEndOffset[SPI_GYRO_ID_HASH(spiID)];
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}

/**
    SPI enable read bit match operation

    Enable bit value compare function.
    After invoke spi_enBitMatch(), it will send uiCmd to SPI device
    and continously read data from SPI device.
    Once bit position specified by uiBitPosition of read data becomes bWaitValue,
    SPI module will stop checking and raise a interrupt.
    Caller of spi_enBitMatch() can use spi_waitBitMatch() to wait this checking complete.

    @param[in] spiID            SPI controller ID
    @param[in] uiCmd            issued command
    @param[in] uiBitPosition    wait bit position of read data
    @param[in] bWaitValue       wait value of uiWaitPosition

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_enBitMatch(SPI_ID spiID, UINT32 uiCmd, UINT32 uiBitPosition, BOOL bWaitValue)
{
	T_SPI_CTRL_REG ctrlReg;
	T_SPI_FLASH_CTRL_REG flashCtrlReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	spi_waitClrFlag(spiID);

	spi_setPacketCount(spiID, 1);

	flashCtrlReg.bit.spi_rdysts_bit = uiBitPosition;
	flashCtrlReg.bit.spi_rdysts_val = bWaitValue;
	flashCtrlReg.bit.spi_rdsts_cmd = uiCmd;
	spi_setReg(spiID, SPI_FLASH_CTRL_REG_OFS, flashCtrlReg.reg);

	vSpiState[spiID] = SPI_STATE_FLASH;

	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	ctrlReg.bit.spi_rdsts_en = 1;
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	return E_OK;
}

/**
    SPI wait read bit match

    Wait read bit match complete. This function should be invoked after spi_enBitMatch().

    @param[in] spiID        SPI controller ID
    @param[in] uiTimeoutMs  timeout for wait bit match (unit: ms)

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER spi_waitBitMatch(SPI_ID spiID, UINT32 uiTimeoutMs)
{
	ER erReturn;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (vSpiState[spiID] != SPI_STATE_FLASH) {
		DBG_WRN("spi%d: not under flash polling state\r\n", spiID);
		return E_CTX;
	}

	if (timer_open(&vSpiTimerID[spiID], vSpiTimeoutHdl[spiID]) != E_OK) {
		DBG_WRN("spi%d: allocate timer fail\r\n", spiID);
	} else {
		vUnderTimerCount[spiID] = TRUE;
		vSpiState[spiID] = SPI_STATE_FLASH_WAIT;
		timer_cfg(vSpiTimerID[spiID], uiTimeoutMs * 1000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}

	erReturn = spi_waitFlag(spiID);
	vSpiState[spiID] = SPI_STATE_IDLE;

	return erReturn;
}

//@}

/*
    @name   SPI Driver internal API
*/
//@{

/**
    SPI set bus width

    @note Invoked after spi_open(). This function is only valid when SPI is init to SPI_PINMUX_STORAGE.
    Please also refer to spi_init().

    @param[in] spiID        SPI controller ID
    @param[in] busWidth     bus width
                            - @b SPI_BUS_WIDTH_1_BIT:   1 bit mode
                            - @b SPI_BUS_WIDTH_2_BITS:  2 bits mode
                            - @b SPI_BUS_WIDTH_HD_1BIT: 1 bit half duplex mode (i.e. 3-wire SPI)

    @return
        - @b E_OK: success
        - @b E_PAR: bus width not correct
        - @b E_NOSPT: not support
*/
ER spi_setBusWidth(SPI_ID spiID, SPI_BUS_WIDTH busWidth)
{
	T_SPI_IO_REG ioReg;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & (SPI_CAPABILITY_2BITS | SPI_CAPABILITY_4BITS))) {
		if (busWidth & (SPI_CAPABILITY_2BITS | SPI_CAPABILITY_4BITS)) {
			return E_NOSPT;
		}
	}

	if (busWidth > SPI_BUS_WIDTH_HD_1BIT) {
		return E_PAR;
	}

	spi_waitFlag(spiID);

	vSpiBusWidth[spiID] = busWidth;

	ioReg.reg = spi_getReg(spiID, SPI_IO_REG_OFS);
	ioReg.bit.spi_io_out_en = 0;            // default to input mode
	switch (busWidth) {
	case SPI_BUS_WIDTH_1_BIT:
		ioReg.bit.spi_io_out_en = 1;    // output enable
		ioReg.bit.spi_bus_width = 0;        // 1 bit full-duplex
		break;
	case SPI_BUS_WIDTH_2_BITS:
		ioReg.bit.spi_bus_width = 2;        // 2 bit half-duplex
		break;
	case SPI_BUS_WIDTH_HD_1BIT:
	default:
		ioReg.bit.spi_bus_width = 1;        // 1 bit half-duplex (3-wire spi)
		break;
	}
	spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);

	return E_OK;
}

#if (DRV_SUPPORT_IST == ENABLE)
/*
    SPI control IST

    SPI control Interrupt Service Task

    @param[in] uiEvent      input event bit mask

    @return void
*/
static void spi_ist3(UINT32 uiEvent)
{
    if (uiEvent != 0) {
        UINT32 i;

		// One event at a time when calling callback function
		i = 0;
		while (uiEvent != 0) {
			if (uiEvent & 0x01) {
                if (pfIstCB_SPI3) {
    				pfIstCB_SPI3(1 << i);
                }
			}

			i++;
			uiEvent >>= 1;
		}
	}

}
#endif

/*
    SPI interrupt handler

    @return void
*/
static irqreturn_t spi_isr(int irq, void *devid)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.reg = SPI_GETREG(SPI_STATUS_REG_OFS);
	intenReg.reg = SPI_GETREG(SPI_INTEN_REG_OFS);
	if (stsReg.bit.dma_abort_sts == 1) {
		DBG_IND("%s: dma abort\r\n", __func__);
	}
	stsReg.reg &= intenReg.reg;
	if (stsReg.reg == 0) {
        return IRQ_NONE;
	}
	SPI_SETREG(SPI_STATUS_REG_OFS, stsReg.reg);

	if (vSpiState[SPI_ID_1] == SPI_STATE_FLASH_WAIT)
//    if (vUnderTimerCount[SPI_ID_1])
	{
		timer_pause_play(vSpiTimerID[SPI_ID_1], TIMER_STATE_PAUSE);
	}

	// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
	// To prevent caller waitDataDone too early:
	// If this dma transfer is not last piece, spi driver should take care background
	// If this dma transfer is last piece, set normal flag to act as normal flow
	if (stsReg.bit.spi_dmaed) {
		if (vDmaPendingLen[SPI_ID_1]) {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA);
		} else {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI);
		}
	} else if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI);
	}

	return IRQ_HANDLED;
}

/*
    SPI2 interrupt handler

    @return void
*/
static irqreturn_t spi2_isr(int irq, void *devid)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.reg = SPI2_GETREG(SPI_STATUS_REG_OFS);
	intenReg.reg = SPI2_GETREG(SPI_INTEN_REG_OFS);
	stsReg.reg &= intenReg.reg;
	if (stsReg.reg == 0) {
		return IRQ_NONE;
	}
	SPI2_SETREG(SPI_STATUS_REG_OFS, stsReg.reg);

	if (vSpiState[SPI_ID_2] == SPI_STATE_FLASH_WAIT) {
		timer_pause_play(vSpiTimerID[SPI_ID_2], TIMER_STATE_PAUSE);
	}

	// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
	// To prevent caller waitDataDone too early:
	// If this dma transfer is not last piece, spi driver should take care background
	// If this dma transfer is last piece, set normal flag to act as normal flow
	if (stsReg.bit.spi_dmaed) {
		if (vDmaPendingLen[SPI_ID_2]) {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA2);
		} else {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI2);
		}
	} else if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI2_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA2);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI2);
	}

	return IRQ_HANDLED;
}

/*
    SPI3 interrupt handler

    @return void
*/
static irqreturn_t spi3_isr(int irq, void *devid)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.reg = SPI3_GETREG(SPI_STATUS_REG_OFS);
	intenReg.reg = SPI3_GETREG(SPI_INTEN_REG_OFS);
	stsReg.reg &= intenReg.reg;
	if (stsReg.reg == 0) {
		return IRQ_NONE;
	}
	SPI3_SETREG(SPI_STATUS_REG_OFS, stsReg.reg);

	// Check if under GYRO mode
	if (vSpiState[SPI_ID_3] == SPI_STATE_GYRO) {
		T_SPI_GYROSEN_TRS_STS_REG trsStsReg;
		T_SPI_GYROSEN_TRS_INTEN_REG trsIntEnReg;
#if (DRV_SUPPORT_IST == ENABLE)
		UINT32 uiEvent;

		uiEvent = 0;
#endif

		trsStsReg.reg = SPI3_GETREG(SPI_GYROSEN_TRS_STS_REG_OFS);
		trsIntEnReg.reg = SPI3_GETREG(SPI_GYROSEN_TRS_INTEN_REG_OFS);
		trsStsReg.reg &= trsIntEnReg.reg;
		// If transfer status, gyro data is already in FIFO, and read it out
		if (trsStsReg.reg != 0) {
			T_SPI_GYROSEN_FIFO_STS_REG fifoStsReg;

			// Write clear Transfer Status
			SPI3_SETREG(SPI_GYROSEN_TRS_STS_REG_OFS, trsStsReg.reg);

			// Read current gyro fifo counter
			fifoStsReg.reg = SPI3_GETREG(SPI_GYROSEN_FIFO_STS_REG_OFS);
			while (fifoStsReg.bit.valid_entry) {
				UINT32 uiWord0, uiWord1;

				// Read out h/w gyro fifo
				uiWord0 = SPI3_GETREG(SPI_GYROSEN_RX_DATA_REG_OFS);
				uiWord1 = SPI3_GETREG(SPI_GYROSEN_RX_DATA_REG_OFS);
				// Push to driver queue
				spi_addGyroQueue(SPI_ID_3, uiWord0, uiWord1);
				// Modify current driver queue depth
				vSpiGyroDataCnt[SPI_GYRO_ID_HASH(SPI_ID_3)]++;

				fifoStsReg.bit.valid_entry -= 2;
			}

			// If driver gyro queue overflow, notify callback
			if (spi_getGyroQueueCount(SPI_ID_3) == (SPI_GYRO_QUEUE_DEPTH - 1)) {
#if (DRV_SUPPORT_IST == ENABLE)
				if (pfIstCB_SPI3 != NULL) {
					uiEvent |= SPI_GYRO_INT_QUEUE_OVERRUN;
				}
#else
				if (vSpiGyroHdl != NULL) {
					vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_QUEUE_OVERRUN);
				}
#endif
			}
			// If driver gyro queue greater than threshold, notify callback
			else if (spi_getGyroQueueCount(SPI_ID_3) > (SPI_GYRO_QUEUE_DEPTH * 2 / 3)) {
#if (DRV_SUPPORT_IST == ENABLE)
				if (pfIstCB_SPI3 != NULL) {
					uiEvent |= SPI_GYRO_INT_QUEUE_THRESHOLD;
				}
#else
				if (vSpiGyroHdl != NULL) {
					vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_QUEUE_THRESHOLD);
				}
#endif
			}
		}
		// If h/w fifo overrun, notify callback (possible system occupied by somebody)
		if (stsReg.bit.gyro_overrun_sts == 1) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI3 != NULL) {
				uiEvent |= SPI_GYRO_INT_OVERRUN;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_OVERRUN);
			}
#endif
		}
		// If SIE VD comes while previous gyro run still on-going, notify callback
		if (stsReg.bit.gyro_seq_err_sts == 1) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI3 != NULL) {
				uiEvent |= SPI_GYRO_INT_SEQ_ERR;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_SEQ_ERR);
			}
#endif
		}
		// If one gyro run is done, notify callback
		if (trsStsReg.reg & vSpiGyroSyncEndMask[SPI_GYRO_ID_HASH(SPI_ID_3)]) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI3 != NULL) {
				uiEvent |= SPI_GYRO_INT_SYNC_END;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_SYNC_END);
			}
#endif
		}
		if (stsReg.bit.all_gyrotrs_done_sts == 1) { // If last transfer, advance VD count and signal to upper layer
			vSpiGyroFrameCnt[SPI_GYRO_ID_HASH(SPI_ID_3)]++;
			vSpiGyroDataCnt[SPI_GYRO_ID_HASH(SPI_ID_3)] = 0;
			if (vSpiGyroState[SPI_GYRO_ID_HASH(SPI_ID_3)] == SPI_GYRO_STATE_CHANGE_ISSUED) {
				// transfer end of deprecated gyro setting
				// Need to do:
				// 1. Modify interrupt enable bit for new gyro setting
				// 2. Issue SPI_GYRO_INT_CHANGE_END to inform upper layer
				vSpiGyroSyncEndMask[SPI_GYRO_ID_HASH(SPI_ID_3)] = vSpiGyroSyncNextEndMask[SPI_GYRO_ID_HASH(SPI_ID_3)];
				spi_setReg(SPI_ID_3, SPI_GYROSEN_TRS_INTEN_REG_OFS,
						   vSpiGyroSyncNextIntEnReg[SPI_GYRO_ID_HASH(SPI_ID_3)]);
#if (DRV_SUPPORT_IST == ENABLE)
				if (pfIstCB_SPI3 != NULL) {
					uiEvent |= SPI_GYRO_INT_CHANGE_END;
				}
#else
				if (vSpiGyroHdl != NULL) {
					vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_CHANGE_END);
				}
#endif

				vSpiGyroState[SPI_GYRO_ID_HASH(SPI_ID_3)] = SPI_GYRO_STATE_RUN;
			}

#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI3 != NULL) {
				uiEvent |= SPI_GYRO_INT_LAST_TRS;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_LAST_TRS);
			}
#endif
		}
		// If transfer timeout, notify callback
		if (stsReg.bit.gyro_trs_timeout_sts == 1) {
			DBG_WRN("Gyro TRS tmo\r\n");
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI3 != NULL) {
				uiEvent |= SPI_GYRO_INT_TRS_TIMEOUT;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_3](SPI_GYRO_INT_TRS_TIMEOUT);
			}
#endif
		}

#if (DRV_SUPPORT_IST == ENABLE)
		if (uiEvent != 0) {
			kick_bh(INT_ID_SPI3, uiEvent, NULL);
//			drv_setIstEvent(DRV_IST_MODULE_SPI3, uiEvent);
		}
#endif
	}

	if (vSpiState[SPI_ID_3] == SPI_STATE_FLASH_WAIT) {
		timer_pause_play(vSpiTimerID[SPI_ID_3], TIMER_STATE_PAUSE);
	}

	// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
	// To prevent caller waitDataDone too early:
	// If this dma transfer is not last piece, spi driver should take care background
	// If this dma transfer is last piece, set normal flag to act as normal flow
	if (stsReg.bit.spi_dmaed) {
		if (vDmaPendingLen[SPI_ID_3]) {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA3);
		} else {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI3);
		}
	} else if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI3_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA3);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI3);
	}

    return IRQ_HANDLED;
}

/*
    SPI4 interrupt handler

    @return void
*/
static irqreturn_t spi4_isr(int irq, void *devid)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.reg = SPI4_GETREG(SPI_STATUS_REG_OFS);
	intenReg.reg = SPI4_GETREG(SPI_INTEN_REG_OFS);
	stsReg.reg &= intenReg.reg;
	if (stsReg.reg == 0) {
		return IRQ_NONE;
	}
	SPI4_SETREG(SPI_STATUS_REG_OFS, stsReg.reg);

	if (vSpiState[SPI_ID_4] == SPI_STATE_FLASH_WAIT) {
		timer_pause_play(vSpiTimerID[SPI_ID_4], TIMER_STATE_PAUSE);
	}

	// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
	// To prevent caller waitDataDone too early:
	// If this dma transfer is not last piece, spi driver should take care background
	// If this dma transfer is last piece, set normal flag to act as normal flow
	if (stsReg.bit.spi_dmaed) {
		if (vDmaPendingLen[SPI_ID_4]) {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA4);
		} else {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI4);
		}
	} else if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI4_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA4);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI4);
	}

	return IRQ_HANDLED;
}

/*
    SPI5 interrupt handler

    @return void
*/
static irqreturn_t spi5_isr(int irq, void *devid)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.reg = SPI5_GETREG(SPI_STATUS_REG_OFS);
	intenReg.reg = SPI5_GETREG(SPI_INTEN_REG_OFS);
	stsReg.reg &= intenReg.reg;
	if (stsReg.reg == 0) {
		return IRQ_NONE;
	}
	SPI5_SETREG(SPI_STATUS_REG_OFS, stsReg.reg);

	if (vSpiState[SPI_ID_5] == SPI_STATE_FLASH_WAIT) {
		timer_pause_play(vSpiTimerID[SPI_ID_5], TIMER_STATE_PAUSE);
	}

	// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
	// To prevent caller waitDataDone too early:
	// If this dma transfer is not last piece, spi driver should take care background
	// If this dma transfer is last piece, set normal flag to act as normal flow
	if (stsReg.bit.spi_dmaed) {
		if (vDmaPendingLen[SPI_ID_5]) {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA5);
		} else {
			iset_flg(FLG_ID_SPI, FLGPTN_SPI5);
		}
	} else if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI5_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA5);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI5);
	}

	return IRQ_HANDLED;
}



#if 0
/*
    SPI4 interrupt handler

    @return void
*/
void spi4_isr(void)
{
//    T_SPI_CTRL_REG ctrlReg;
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.Reg = SPI4_GETREG(SPI_STATUS_REG_OFS);
	intenReg.Reg = SPI4_GETREG(SPI_INTEN_REG_OFS);
	stsReg.Reg &= intenReg.Reg;
	if (stsReg.Reg == 0) {
		return;
	}
	SPI4_SETREG(SPI_STATUS_REG_OFS, stsReg.Reg);

//    ctrlReg.Reg = SPI4_GETREG(SPI_CTRL_REG_OFS);
	// Check if under GYRO mode
	if (vSpiState[SPI_ID_4] == SPI_STATE_GYRO)
//    if (ctrlReg.Bit.spi_gyro_sts != 0)
	{
		T_SPI_GYROSEN_TRS_STS_REG trsStsReg;
		T_SPI_GYROSEN_TRS_INTEN_REG trsIntEnReg;
//        T_SPI_GYROSEN_OP_STS_REG opStsReg;
//        T_SPI_GYROSEN_OP_INTEN_REG opIntEnReg;
#if (DRV_SUPPORT_IST == ENABLE)
		UINT32 uiEvent;

		uiEvent = 0;
#endif

		trsStsReg.Reg = SPI4_GETREG(SPI_GYROSEN_TRS_STS_REG_OFS);
		trsIntEnReg.Reg = SPI4_GETREG(SPI_GYROSEN_TRS_INTEN_REG_OFS);
		trsStsReg.Reg &= trsIntEnReg.Reg;
		// If transfer status, gyro data is already in FIFO, and read it out
		if (trsStsReg.Reg != 0) {
			T_SPI_GYROSEN_FIFO_STS_REG fifoStsReg;

			// Write clear Transfer Status
			SPI4_SETREG(SPI_GYROSEN_TRS_STS_REG_OFS, trsStsReg.Reg);

			// Read current gyro fifo counter
			fifoStsReg.Reg = SPI4_GETREG(SPI_GYROSEN_FIFO_STS_REG_OFS);
			while (fifoStsReg.Bit.valid_entry) {
				UINT32 uiWord0, uiWord1;

				// Read out h/w gyro fifo
				uiWord0 = SPI4_GETREG(SPI_GYROSEN_RX_DATA_REG_OFS);
				uiWord1 = SPI4_GETREG(SPI_GYROSEN_RX_DATA_REG_OFS);
				// Push to driver queue
				spi_addGyroQueue(SPI_ID_4, uiWord0, uiWord1);
				// Modify current driver queue depth
				vSpiGyroDataCnt[SPI_GYRO_ID_HASH(SPI_ID_4)]++;

				fifoStsReg.Bit.valid_entry -= 2;
			}

			// If driver gyro queue overflow, notify callback
			if (spi_getGyroQueueCount(SPI_ID_4) == (SPI_GYRO_QUEUE_DEPTH - 1)) {
#if (DRV_SUPPORT_IST == ENABLE)
				if (pfIstCB_SPI4 != NULL) {
					uiEvent |= SPI_GYRO_INT_QUEUE_OVERRUN;
				}
#else
				if (vSpiGyroHdl != NULL) {
					vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_QUEUE_OVERRUN);
				}
#endif
			}
			// If driver gyro queue greater than threshold, notify callback
			else if (spi_getGyroQueueCount(SPI_ID_4) > (SPI_GYRO_QUEUE_DEPTH * 2 / 3)) {
#if (DRV_SUPPORT_IST == ENABLE)
				if (pfIstCB_SPI4 != NULL) {
					uiEvent |= SPI_GYRO_INT_QUEUE_THRESHOLD;
				}
#else
				if (vSpiGyroHdl != NULL) {
					vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_QUEUE_THRESHOLD);
				}
#endif
			}
		}
		// If h/w fifo overrun, notify callback (possible system occupied by somebody)
		if (stsReg.Bit.gyro_overrun_sts == 1) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI4 != NULL) {
				uiEvent |= SPI_GYRO_INT_OVERRUN;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_OVERRUN);
			}
#endif
		}
		// If SIE VD comes while previous gyro run still on-going, notify callback
		if (stsReg.Bit.gyro_seq_err_sts == 1) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI4 != NULL) {
				uiEvent |= SPI_GYRO_INT_SEQ_ERR;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_SEQ_ERR);
			}
#endif
		}
		// If one gyro run is done, notify callback
		if (trsStsReg.Reg & uiSpiGyroSyncEndMask)
//        if (stsReg.Bit.all_gyrotrs_done_sts == 1)   // If last transfer, advance VD count and signal to upper layer
		{
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI4 != NULL) {
				uiEvent |= SPI_GYRO_INT_SYNC_END;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_SYNC_END);
			}
#endif
//            vSpiGyroFrameCnt[SPI_GYRO_ID_HASH(SPI_ID_4)]++;
//            vSpiGyroDataCnt[SPI_GYRO_ID_HASH(SPI_ID_4)] = 0;
		}
		if (stsReg.Bit.all_gyrotrs_done_sts == 1) { // If last transfer, advance VD count and signal to upper layer
			vSpiGyroFrameCnt[SPI_GYRO_ID_HASH(SPI_ID_4)]++;
			vSpiGyroDataCnt[SPI_GYRO_ID_HASH(SPI_ID_4)] = 0;
		}
		// If transfer timeout, notify callback
		if (stsReg.Bit.gyro_trs_timeout_sts == 1) {
#if (DRV_SUPPORT_IST == ENABLE)
			if (pfIstCB_SPI4 != NULL) {
				uiEvent |= SPI_GYRO_INT_TRS_TIMEOUT;
			}
#else
			if (vSpiGyroHdl != NULL) {
				vSpiGyroHdl[SPI_ID_4](SPI_GYRO_INT_TRS_TIMEOUT);
			}
#endif
		}

#if (DRV_SUPPORT_IST == ENABLE)
		if (uiEvent != 0) {
			drv_setIstEvent(DRV_IST_MODULE_SPI4, uiEvent);
		}
#endif
	}

	if (vSpiState[SPI_ID_4] == SPI_STATE_FLASH_WAIT) {
		timer_pausePlay(vSpiTimerID[SPI_ID_4], TIMER_STATE_PAUSE);
	}
	if (stsReg.bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.bit.spi_rdr_full = 0;
		SPI4_SETREG(SPI_INTEN_REG_OFS, intenReg.reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA4);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI4);
	}
}

/*
    SPI5 interrupt handler

    @return void
*/
void spi5_isr(void)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_INTEN_REG intenReg;

	stsReg.Reg = SPI5_GETREG(SPI_STATUS_REG_OFS);
	intenReg.Reg = SPI5_GETREG(SPI_INTEN_REG_OFS);
	stsReg.Reg &= intenReg.Reg;
	if (stsReg.Reg == 0) {
		return;
	}
	SPI5_SETREG(SPI_STATUS_REG_OFS, stsReg.Reg);

	if (vSpiState[SPI_ID_5] == SPI_STATE_FLASH_WAIT)
//    if (vUnderTimerCount[SPI_ID_5])
	{
		timer_pausePlay(vSpiTimerID[SPI_ID_5], TIMER_STATE_PAUSE);
	}

	if (stsReg.Bit.spi_rdr_full) {
		// Because spi_rdr_full is level trigger,
		// we must disable it's interrupt enable bit to prevent CPU is always pulled into interrupt mode
		intenReg.Bit.spi_rdr_full = 0;
		SPI3_SETREG(SPI_INTEN_REG_OFS, intenReg.Reg);

		iset_flg(FLG_ID_SPI, FLGPTN_SPI_DMA5);
	} else {
		iset_flg(FLG_ID_SPI, FLGPTN_SPI5);
	}
}
#endif
/*
    Lock SPI module

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER spi_lock(SPI_ID spiID)
{
	ER erReturn;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	erReturn        = SEM_WAIT(vSpiSemaphore[spiID]);
	if (erReturn != E_OK) {
		DBG_ERR("SPI%d: wait semaphore fail\r\n", spiID);
		return erReturn;
	}
	vSPILockStatus[spiID]   = TASK_LOCKED;
	return E_OK;
}

/*
    Unlock SPI module

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER spi_unlock(SPI_ID spiID)
{
	ER erReturn = E_OK;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	SEM_SIGNAL(vSpiSemaphore[spiID]);
	vSPILockStatus[spiID]   = NO_TASK_LOCKED;

	return erReturn;
}

/*
    Wait for SPI TDRE Flag and Clear this Flag

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_waitTDRE(SPI_ID spiID)
{
	T_SPI_STATUS_REG stsReg;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	while (1) {
		stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
		if (stsReg.bit.spi_tdr_empty) {
			break;
		}
	}

}

/*
    Wait for SPI RDRF Flag and Clear this Flag

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_waitRDRF(SPI_ID spiID)
{
	T_SPI_STATUS_REG stsReg;
	T_SPI_CONFIG_REG configReg = {0};
	UINT32 uiDelay = 0;     // unit: us

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (configReg.bit.spi_pkt_burst_post_cond &&
		configReg.bit.spi_pkt_burst_handshake_en) {
		T_SPI_TIMING_REG timingReg = {0};
		UINT32 uiFreq;

#if (_EMULATION_ == ENABLE)
		pll_getClockFreq(SPICLK_FREQ + spiID, &uiFreq);
#else
		uiFreq = vSpiInitInfo[spiID].uiFreq;
#endif
		timingReg.reg = spi_getReg(spiID, SPI_TIMING_REG_OFS);
		// Actual delay between packet burst is (SPI_POST_COND_DLY + 4) SPI clock.
		uiDelay = (timingReg.bit.spi_post_cond_dly + 4) * 1000000 /
				  uiFreq;
	}

	// If SPI frequency <= 500 KHz, use interrupt to wait RDRF
	// (1/500KHz * 8 = 16 us, equal to Delay.c software polling threshold)
	// OR if packet delay is greater than 16 us
	if ((vSpiInitInfo[spiID].uiFreq <= 500000) || (uiDelay >= 16)) {
		FLGPTN  uiFlag;
		T_SPI_INTEN_REG intenReg;
		UINT32 flags;

		// Due to DMA will not work with PIO simultaneously, we use DMA flag for RDRF
		clr_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);

		// Enable RDRF interrupt
		// And this interrupt will be disabled when ISR is triggered by RDRF
		vk_spin_lock_irqsave(&spi_spinlock, flags);
		intenReg.reg = spi_getReg(spiID, SPI_INTEN_REG_OFS);
		intenReg.bit.spi_rdr_full = 1;
		spi_setReg(spiID, SPI_INTEN_REG_OFS, intenReg.reg);
		vk_spin_unlock_irqrestore(&spi_spinlock, flags);

		wai_flg(&uiFlag, FLG_ID_SPI, vSpiDmaFlags[spiID], TWF_ORW | TWF_CLR);
	} else {
		while (1) {
			stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
			if (stsReg.bit.spi_rdr_full) {
				break;
			}
		}
	}

	vSpiState[spiID] = SPI_STATE_IDLE;
}

/*
    Get SPI receive data

    @param[in] spiID        SPI controller ID

    @return received data
*/
static UINT32 spi_getBuffer(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return 0;
	}

	return spi_getReg(spiID, SPI_RDR_REG_OFS);
}

/*
    Set SPI transmit data

    @param[in] spiID        SPI controller ID
    @param[in] uiSPIBuffer  value of transmit data

    @return void
*/
static void spi_setBuffer(SPI_ID spiID, UINT32 uiSPIBuffer)
{
	T_SPI_TDR_REG tdrReg;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	vSpiState[spiID] = SPI_STATE_PIO;

	tdrReg.reg = uiSPIBuffer;
	spi_setReg(spiID, SPI_TDR_REG_OFS, tdrReg.reg);
}

/*
    Configure SPI packet count

    @param[in] spiID        SPI controller ID
    @param[in] uiPackets    SPI packet count
                            - @b 1: 1 packet
                            - @b 2: 2 packets
                            - @b 4: 4 packets

    @return void
*/
static void spi_setPacketCount(SPI_ID spiID, UINT32 uiPackets)
{
	T_SPI_CONFIG_REG configReg;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

//	DBG_ERR("%s: pkt count %d\r\n", __func__, uiPackets+1);

	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	switch (uiPackets) {
	case 1:
		configReg.bit.spi_pkt_cnt = SPI_PKT_CNT_ENUM_1PKT;
		break;
	case 2:
		configReg.bit.spi_pkt_cnt = SPI_PKT_CNT_ENUM_2PKT;
		break;
	case 3:
		configReg.bit.spi_pkt_cnt = SPI_PKT_CNT_ENUM_3PKT;
		break;
	default:
		configReg.bit.spi_pkt_cnt = SPI_PKT_CNT_ENUM_4PKT;
		break;
	}

	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);
}

/*
    Set SPI to output mode in wide bus.

    If SPI is set to SPI_BUS_WIDTH_2_BITS, it can only do output or input
    at the same time.
    If SPI is set to SPI_BUS_WIDTH_1_BIT, it will alter SPI_DO(SPI_D0) output/input mode

    @param[in] spiID        SPI controller ID
    @param[in] bOutputMode  wide bus direction setting
                            - @b TRUE: output mode
                            - @b FALSE: input mode

    @return void
*/
static void spi_setWideBusOutput(SPI_ID spiID, BOOL bOutputMode)
{
	T_SPI_IO_REG ioReg;
//    T_SPI_CTRL2_REG ctrl2Reg;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	ioReg.reg = spi_getReg(spiID, SPI_IO_REG_OFS);

	if (bOutputMode == TRUE) {
		ioReg.bit.spi_io_out_en = 1;
	} else {
		ioReg.bit.spi_io_out_en = 0;
	}
	spi_setReg(spiID, SPI_IO_REG_OFS, ioReg.reg);
}

/*
    Set SPI DMA transfer count

    This function can set DMA transfer count.
    Count means needing to write SPI_TDR how many times.

    @param[in] spiID        SPI controller ID
    @param[in] uiCount      transfer count. (unit: word)

    @return void
*/
static void spi_setTransferCount(SPI_ID spiID, UINT32 uiCount)
{
	T_SPI_DMA_BUFSIZE_REG dmaSizeReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	uiCount <<= 2;
	dmaSizeReg.bit.spi_dma_bufsize = uiCount;
	spi_setReg(spiID, SPI_DMA_BUFSIZE_REG_OFS, dmaSizeReg.reg);
}

/*
    Set SPI DMA starting address

    This function can set DMA starting address.

    @param[in] spiID        SPI controller ID
    @param[in] uiAddr       DMA starting address

    @return void
*/
static void spi_setDmaStartingAddress(SPI_ID spiID, UINT32 uiAddr)
{
	T_SPI_DMA_STARTADDR_REG dmaAddrReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	dmaAddrReg.bit.spi_dma_start_addr = vos_cpu_get_phy_addr(uiAddr);
	spi_setReg(spiID, SPI_DMA_STARTADDR_REG_OFS, dmaAddrReg.reg);
}

/*
    Set SPI DMA transfer direction

    This function can set DMA transfer direction (SPI->DRAM or DRAM->SPI).

    @param[in] spiID        SPI controller ID
    @param[in] uiDmaDir     DMA direction setting.
                            - @b SPI_DMA_DIR_SPI2RAM
                            - @b SPI_DMA_DIR_RAM2SPI
    @return void
*/
static void spi_setDmaDirection(SPI_ID spiID, BOOL uiDmaDir)
{
	T_SPI_DMA_CTRL_REG dmaCtrlReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	dmaCtrlReg.bit.dma_dir = uiDmaDir;
	spi_setReg(spiID, SPI_DMA_CTRL_REG_OFS, dmaCtrlReg.reg);
}

/*
    Get SPI DMA transfer direction

    This function can get DMA transfer direction (SPI->DRAM or DRAM->SPI).

    @param[in] spiID        SPI controller ID


    @return DMA direction setting.
            - @b SPI_DMA_DIR_SPI2RAM
            - @b SPI_DMA_DIR_RAM2SPI
*/
static UINT32 spi_getDmaDirection(SPI_ID spiID)
{
	T_SPI_DMA_CTRL_REG dmaCtrlReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return SPI_DMA_DIR_RAM2SPI;
	}

	dmaCtrlReg.reg = spi_getReg(spiID, SPI_DMA_CTRL_REG_OFS);

	return dmaCtrlReg.bit.dma_dir;
}


/*
    Enable SPI DMA transfer

    This function can enable/disable SPI DMA function

    @param[in] spiID    SPI controller ID
    @param[in] bDmaEn   DMA mode selection
                        - @b TRUE: enable DMA
                        - @b FALSE: disable DMA

    @return void
*/
static void spi_enableDma(SPI_ID spiID, BOOL bDmaEn)
{
	T_SPI_CTRL_REG ctrlReg;
	T_SPI_CONFIG_REG configReg;
	T_SPI_STATUS_REG stsReg = {0};

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	if (bDmaEn) {
		T_SPI_TIMING_REG timingReg;

		timingReg.reg = spi_getReg(spiID, SPI_TIMING_REG_OFS);

		if (timingReg.bit.spi_post_cond_dly) {
			configReg.bit.spi_pkt_burst_post_cond = 1;
			configReg.bit.spi_pkt_burst_handshake_en = 1;
			spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);
		}
	} else {
		configReg.bit.spi_pkt_burst_post_cond = 0;
		configReg.bit.spi_pkt_burst_handshake_en = 0;
		spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);
	}

	stsReg.bit.dma_abort_sts = 1;
	spi_setReg(spiID, SPI_STATUS_REG_OFS, stsReg.reg);

	ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
	if (bDmaEn) {
		ctrlReg.bit.spi_dma_en = 1;
	} else {
		ctrlReg.bit.spi_dma_dis = 1;
	}
	spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);

	if (bDmaEn && bSpiTestDmaAbort) {
		UINT32 i;

		for (i = 0; i < 4; i++) {
		}
		ctrlReg.bit.spi_dma_en = 0;
		ctrlReg.bit.spi_dma_dis = 1;
#if _FPGA_EMULATION_
		gpio_setPin(C_GPIO_15);
#endif
		spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
#if _FPGA_EMULATION_
		gpio_clearPin(C_GPIO_15);
#endif
		DBG_MSG("\r\nabort DMA, 0x%lx\r\n", ctrlReg.reg);
	}
}

/*
    Write data to dma

    @param[in] spiID        SPI controller ID
    @param[in] puiBuffer    the dma buffer address
    @param[in] uiWordCount  the transfer word count

    @return void
*/
static void spi_setDmaWrite(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiWordCount)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	vos_cpu_dcache_sync((UINT32)puiBuffer, uiWordCount * 4, VOS_DMA_TO_DEVICE_NON_ALIGN);
	/*
	    spi_enableDma(spiID, FALSE);                   // disable DMA

	    spi_setTransferCount(spiID, uiWordCount);
	    spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
	    spi_setDmaDirection(spiID, SPI_DMA_DIR_RAM2SPI); // DRAM to SPI

	    spi_enableDma(spiID, TRUE);
	*/
	vSpiState[spiID] = SPI_STATE_DMA;
	vUnderDma[spiID] = TRUE;
	do {
		UINT32 uiChunk;
		FLGPTN  uiFlag;

		clr_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);

		// If pending length > max dma length, split it to many chunks
		if (uiWordCount > (SPI_MAX_TRANSFER_CNT >> 2)) {
			uiChunk = SPI_MAX_TRANSFER_CNT >> 2;
		} else {
			uiChunk = uiWordCount;
		}
		uiWordCount -= uiChunk;

		// Save pending length for spi isr to check
		vDmaPendingLen[spiID] = uiWordCount * 4;

		spi_enableDma(spiID, FALSE);                   // disable DMA
		spi_setTransferCount(spiID, uiChunk);
		spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
		spi_setDmaDirection(spiID, SPI_DMA_DIR_RAM2SPI); // DRAM to SPI
		spi_enableDma(spiID, TRUE);

		puiBuffer += uiChunk;       // word pointer, just add word count

		// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
		// To prevent caller waitDataDone too early:
		// If this dma transfer is not last piece, spi driver should take care background
		if (uiWordCount) {
			T_SPI_STATUS_REG stsReg;

			wai_flg(&uiFlag, FLG_ID_SPI, vSpiDmaFlags[spiID], TWF_ORW | TWF_CLR);

			stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
			if (stsReg.bit.dma_abort_sts) {
				DBG_MSG("DMA aborted\r\n");
				set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
				break;
			}

		}
	} while (uiWordCount);
}

/*
    Write data to dma (Byte version)

    @param[in] spiID        SPI controller ID
    @param[in] puiBuffer    the dma buffer address
    @param[in] uiByteCount  the transfer byte count

    @return void
*/
static void spi_setDmaWriteByte(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiByteCount)
{
	T_SPI_DMA_BUFSIZE_REG dmaSizeReg;
	UINT32 uiMaxDmaLen;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	vos_cpu_dcache_sync((UINT32)puiBuffer, uiByteCount, VOS_DMA_TO_DEVICE_NON_ALIGN);
	/*
	    spi_enableDma(spiID, FALSE);                   // disable DMA

	    {
	        dmaSizeReg.reg = 0;
	        dmaSizeReg.bit.spi_dma_bufsize = uiByteCount;
	        spi_setReg(spiID, SPI_DMA_BUFSIZE_REG_OFS, dmaSizeReg.reg);
	    }
	    spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
	    spi_setDmaDirection(spiID, SPI_DMA_DIR_RAM2SPI); // DRAM to SPI

	    spi_enableDma(spiID, TRUE);
	*/

	if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
		// When transfer length is 1 byte, dma length is byte align
		uiMaxDmaLen = SPI_MAX_TRANSFER_CNT;
	} else {
		// When transfer length is 2 byte, dma length is half word (2 byte) align
		uiMaxDmaLen = SPI_MAX_TRANSFER_CNT & (~0x01);
	}

	vUnderDma[spiID] = TRUE;
	vSpiState[spiID] = SPI_STATE_DMA;
	do {
		UINT32 uiChunk;
		FLGPTN  uiFlag;

		clr_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);

		// If pending length > max dma length, split it to many chunks
		if (uiByteCount > uiMaxDmaLen) {
			uiChunk = uiMaxDmaLen;
		} else {
			uiChunk = uiByteCount;
		}
		uiByteCount -= uiChunk;

		// Save pending length for spi isr to check
		vDmaPendingLen[spiID] = uiByteCount;

		spi_enableDma(spiID, FALSE);                   // disable DMA
		{
			dmaSizeReg.reg = 0;
			dmaSizeReg.bit.spi_dma_bufsize = uiChunk;
			spi_setReg(spiID, SPI_DMA_BUFSIZE_REG_OFS, dmaSizeReg.reg);
		}
		spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
		spi_setDmaDirection(spiID, SPI_DMA_DIR_RAM2SPI); // DRAM to SPI
		spi_enableDma(spiID, TRUE);

		puiBuffer += uiChunk / 4;   // word pointer, just add word count, not add byte count

		// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
		// To prevent caller waitDataDone too early:
		// If this dma transfer is not last piece, spi driver should take care background
		if (uiByteCount) {
			T_SPI_STATUS_REG stsReg;

			wai_flg(&uiFlag, FLG_ID_SPI, vSpiDmaFlags[spiID], TWF_ORW | TWF_CLR);

			stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
			if (stsReg.bit.dma_abort_sts) {
				DBG_MSG("DMA aborted\r\n");
				set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
				break;
			}

		}
	} while (uiByteCount);
}

/*
    Read data from dma

    @param[in] spiID        SPI controller ID
    @param[in] puiBuffer    the dma buffer address
    @param[in] uiWordCount  the transfer word count

    @return void
*/
static void spi_setDmaRead(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiWordCount)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	vos_cpu_dcache_sync((UINT32)puiBuffer, uiWordCount * 4, VOS_DMA_FROM_DEVICE_NON_ALIGN);
	/*
	    spi_enableDma(spiID, FALSE);                   // disable DMA

	    spi_setTransferCount(spiID, uiWordCount);
	    spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
	    spi_setDmaDirection(spiID, SPI_DMA_DIR_SPI2RAM); // SPI to DRAM

	    spi_enableDma(spiID, TRUE);
	*/
	vUnderDma[spiID] = TRUE;
	vSpiState[spiID] = SPI_STATE_DMA;
	vDmaStartAddr[spiID] = (UINT32)puiBuffer;
	vDmaTotalSize[spiID] = uiWordCount * 4;
	do {
		UINT32 uiChunk;
		FLGPTN  uiFlag;

		clr_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);

		// If pending length > max dma length, split it to many chunks
		if (uiWordCount > (SPI_MAX_TRANSFER_CNT >> 2)) {
			uiChunk = SPI_MAX_TRANSFER_CNT >> 2;
		} else {
			uiChunk = uiWordCount;
		}
		uiWordCount -= uiChunk;

		// Save pending length for spi isr to check
		vDmaPendingLen[spiID] = uiWordCount * 4;

		spi_enableDma(spiID, FALSE);                   // disable DMA
		spi_setTransferCount(spiID, uiChunk);
		spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
		spi_setDmaDirection(spiID, SPI_DMA_DIR_SPI2RAM); // SPI to DRAM
		spi_enableDma(spiID, TRUE);

		puiBuffer += uiChunk;       // word pointer, just add word count

		// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
		// To prevent caller waitDataDone too early:
		// If this dma transfer is not last piece, spi driver should take care background
		if (uiWordCount) {
			T_SPI_STATUS_REG stsReg;

			wai_flg(&uiFlag, FLG_ID_SPI, vSpiDmaFlags[spiID], TWF_ORW | TWF_CLR);

			stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
			if (stsReg.bit.dma_abort_sts) {
				DBG_MSG("DMA aborted\r\n");
				set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
				break;
			}

		}
	} while (uiWordCount);
}

/*
    Read data from dma (Byte version)

    @param[in] spiID        SPI controller ID
    @param[in] puiBuffer    the dma buffer address
    @param[in] uiByteCount  the transfer byte count

    @return void
*/
static void spi_setDmaReadByte(SPI_ID spiID, UINT32 *puiBuffer, UINT32 uiByteCount)
{
	T_SPI_DMA_BUFSIZE_REG dmaSizeReg;
	UINT32 uiMaxDmaLen;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	vos_cpu_dcache_sync((UINT32)puiBuffer, uiByteCount, VOS_DMA_FROM_DEVICE_NON_ALIGN);
	/*
	    spi_enableDma(spiID, FALSE);                   // disable DMA

	    {
	        dmaSizeReg.reg = 0;
	        dmaSizeReg.bit.spi_dma_bufsize = uiByteCount;
	        spi_setReg(spiID, SPI_DMA_BUFSIZE_REG_OFS, dmaSizeReg.reg);
	    }
	    spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
	    spi_setDmaDirection(spiID, SPI_DMA_DIR_SPI2RAM); // SPI to DRAM

	    spi_enableDma(spiID, TRUE);
	*/
	if (vSpiTransferLen[spiID] == SPI_TRANSFER_LEN_1BYTE) {
		// When transfer length is 1 byte, dma length is byte align
		uiMaxDmaLen = SPI_MAX_TRANSFER_CNT;
	} else {
		// When transfer length is 2 byte, dma length is half word (2 byte) align
		uiMaxDmaLen = SPI_MAX_TRANSFER_CNT & (~0x01);
	}

	vUnderDma[spiID] = TRUE;
	vSpiState[spiID] = SPI_STATE_DMA;
	vDmaStartAddr[spiID] = (UINT32)puiBuffer;
	vDmaTotalSize[spiID] = uiByteCount;
	do {
		UINT32 uiChunk;
		FLGPTN  uiFlag;

		clr_flg(FLG_ID_SPI, vSpiDmaFlags[spiID]);

		// If pending length > max dma length, split it to many chunks
		if (uiByteCount > uiMaxDmaLen) {
			uiChunk = uiMaxDmaLen;
		} else {
			uiChunk = uiByteCount;
		}
		uiByteCount -= uiChunk;

		// Save pending length for spi isr to check
		vDmaPendingLen[spiID] = uiByteCount;

		spi_enableDma(spiID, FALSE);                   // disable DMA
		{
			dmaSizeReg.reg = 0;
			dmaSizeReg.bit.spi_dma_bufsize = uiChunk;
			spi_setReg(spiID, SPI_DMA_BUFSIZE_REG_OFS, dmaSizeReg.reg);
		}
		spi_setDmaStartingAddress(spiID, (UINT32)puiBuffer);
		spi_setDmaDirection(spiID, SPI_DMA_DIR_SPI2RAM); // SPI to DRAM
		spi_enableDma(spiID, TRUE);

		puiBuffer += uiChunk / 4;   // word pointer, just add word count, not add byte count

		// Normal dma flow is spi_writeReadData() --> spi_waitDataDone()
		// To prevent caller waitDataDone too early:
		// If this dma transfer is not last piece, spi driver should take care background
		if (uiByteCount) {
			T_SPI_STATUS_REG stsReg;

			wai_flg(&uiFlag, FLG_ID_SPI, vSpiDmaFlags[spiID], TWF_ORW | TWF_CLR);

			stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
			if (stsReg.bit.dma_abort_sts) {
				DBG_MSG("DMA aborted\r\n");
				set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
				break;
			}

		}
	} while (uiByteCount);
}

/*
    Wait SPI flag

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: wait success
        - @b Else: wait timeout
*/
static ER spi_waitFlag(SPI_ID spiID)
{
	FLGPTN  uiFlag = 0;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	wai_flg(&uiFlag, FLG_ID_SPI, vSpiFlags[spiID] | vSpiTimeoutFlags[spiID], TWF_ORW);

	if (vSpiState[spiID] == SPI_STATE_DMA)
//    if (vUnderDma[spiID])
	{
		T_SPI_STATUS_REG stsReg;

		stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
		if (stsReg.bit.dma_abort_sts) {
			DBG_MSG("%s: DMA aborted\r\n", __func__);
		}

		if (spi_getDmaDirection(spiID) == SPI_DMA_DIR_SPI2RAM) {
			vos_cpu_dcache_sync(vDmaStartAddr[spiID], vDmaTotalSize[spiID], VOS_DMA_FROM_DEVICE_NON_ALIGN);
		}

		vUnderDma[spiID] = FALSE;
		vSpiState[spiID] = SPI_STATE_IDLE;
		vDmaStartAddr[spiID] = 0;
		vDmaTotalSize[spiID] = 0;

		spi_enableDma(spiID, FALSE);
	}

	if (vSpiState[spiID] == SPI_STATE_FLASH_WAIT)
//    if (vUnderTimerCount[spiID])
	{
		vUnderTimerCount[spiID] = FALSE;
		vSpiState[spiID] = SPI_STATE_IDLE;

		timer_close(vSpiTimerID[spiID]);
	}

	if (!(uiFlag & vSpiFlags[spiID])) {
		T_SPI_CTRL_REG ctrlReg;

		ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		ctrlReg.bit.spi_rdsts_dis = 1;
		spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
#if _FPGA_EMULATION_
#if 0
		Perf_Open();
		Perf_Mark();
		DBG_IND("%s: dis flash %d us\r\n", __func__, Perf_GetDuration());
		Perf_Close();
#endif
#endif

		clr_flg(FLG_ID_SPI, vSpiTimeoutFlags[spiID]);
		set_flg(FLG_ID_SPI, vSpiFlags[spiID]);
		vSpiState[spiID] = SPI_STATE_IDLE;

		DBG_ERR("spi%d: enBitMatch() timeout\r\n", spiID);

		return E_TMOUT;
	}

	return E_OK;
}

/*
    Wait/Clear SPI flag

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_waitClrFlag(SPI_ID spiID)
{
	FLGPTN  uiFlag = 0;

	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	wai_flg(&uiFlag, FLG_ID_SPI, vSpiFlags[spiID] | vSpiTimeoutFlags[spiID], TWF_ORW | TWF_CLR);

	if (vSpiState[spiID] == SPI_STATE_DMA)
//    if (vUnderDma[spiID])
	{
		T_SPI_STATUS_REG stsReg;

		stsReg.reg = spi_getReg(spiID, SPI_STATUS_REG_OFS);
		if (stsReg.bit.dma_abort_sts) {
			DBG_MSG("DMA aborted\r\n");
		}

		if (spi_getDmaDirection(spiID) == SPI_DMA_DIR_SPI2RAM) {
			vos_cpu_dcache_sync(vDmaStartAddr[spiID], vDmaTotalSize[spiID], VOS_DMA_FROM_DEVICE_NON_ALIGN);
		}

		vUnderDma[spiID] = FALSE;
		vSpiState[spiID] = SPI_STATE_IDLE;
		vDmaStartAddr[spiID] = 0;
		vDmaTotalSize[spiID] = 0;

		spi_enableDma(spiID, FALSE);
	}

	if (vSpiState[spiID] == SPI_STATE_FLASH_WAIT)
//    if (vUnderTimerCount[spiID])
	{
		vUnderTimerCount[spiID] = FALSE;
		vSpiState[spiID] = SPI_STATE_IDLE;

		timer_close(vSpiTimerID[spiID]);
	}

	if (!(uiFlag & vSpiFlags[spiID])) {
		T_SPI_CTRL_REG ctrlReg;

		ctrlReg.reg = spi_getReg(spiID, SPI_CTRL_REG_OFS);
		ctrlReg.bit.spi_rdsts_dis = 1;
		spi_setReg(spiID, SPI_CTRL_REG_OFS, ctrlReg.reg);
#if _FPGA_EMULATION_
#if 0
		Perf_Open();
		Perf_Mark();
		DBG_IND("%s: dis flash %d us\r\n", __func__, Perf_GetDuration());
		Perf_Close();
#endif
#endif

		DBG_ERR("spi%d: enBitMatch() timeout\r\n", spiID);
	}
}

/*
    SPI timeout handler

    This function is provided to timer API to be called back when timeout occured.

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_timeout_hdl(UINT32 uiEvent)
{
	set_flg(FLG_ID_SPI, vSpiTimeoutFlags[SPI_ID_1]);
}

/*
    SPI2 timeout handler

    This function is provided to timer API to be called back when timeout occured.

    @return void
*/
static void spi_timeout_hdl2(UINT32 uiEvent)
{
	set_flg(FLG_ID_SPI, vSpiTimeoutFlags[SPI_ID_2]);
}

/*
    SPI3 timeout handler

    This function is provided to timer API to be called back when timeout occured.

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_timeout_hdl3(UINT32 uiEvent)
{
	set_flg(FLG_ID_SPI, vSpiTimeoutFlags[SPI_ID_3]);
}

/*
    SPI4 timeout handler

    This function is provided to timer API to be called back when timeout occured.

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_timeout_hdl4(UINT32 uiEvent)
{
	set_flg(FLG_ID_SPI, vSpiTimeoutFlags[SPI_ID_4]);
}

/*
    SPI5 timeout handler

    This function is provided to timer API to be called back when timeout occured.

    @param[in] spiID        SPI controller ID

    @return void
*/
static void spi_timeout_hdl5(UINT32 uiEvent)
{
	set_flg(FLG_ID_SPI, vSpiTimeoutFlags[SPI_ID_5]);
}

/*
    Get current SPI control and delay register status

    (Engineer usage API)

    This function is provided to show current control and delay register status

    @param[in] spiID        SPI controller ID
    @param[out] puiIO       The pointer to IO register
    @param[out] puiCfg      The pointer to config register
    @param[out] puiDelay    The pointer to delay register

    @return void
*/
void spi_getCurrStatus(SPI_ID spiID, UINT32 *puiIO, UINT32 *puiCfg, UINT32 *puiDelay)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	*puiIO = spi_getReg(spiID, SPI_IO_REG_OFS);
	*puiCfg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	*puiDelay = spi_getReg(spiID, SPI_DLY_CHAIN_REG_OFS);
}

/*
    Get GYRO transfer interrupt status

    (Engineer usage API)

    This function is provided to assit emulation/verfication

    @param[in] spiID        SPI controller ID

    @return current gyro transfer/op status
*/
UINT32 spi_getGyroStatus(SPI_ID spiID)
{
	T_SPI_GYROSEN_TRS_STS_REG trsStsReg;

	if (spiID >= SPI_ID_COUNT) {
		return 0;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return 0;
	}

	trsStsReg.reg = spi_getReg(spiID, SPI_GYROSEN_TRS_STS_REG_OFS);
	return trsStsReg.reg;
}

/*
    Clear GYRO transfer interrupt status

    This function is provided to assit emulation/verfication

    @param[in] spiID        SPI controller ID
    @param[in] uiSts        Cleared status

    @return void
*/
void spi_clrGyroStatus(SPI_ID spiID, UINT32 uiSts)
{
	if (spiID >= SPI_ID_COUNT) {
		return;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return;
	}

	spi_setReg(spiID, SPI_GYROSEN_TRS_STS_REG_OFS, uiSts);
}

/*
    Reset GYRO queue buffer

    This function will reset software queue stores GYRO data

    @param[in] spiID        SPI controller ID

    @return
        - @b E_OK: success
        - @b E_ID: invalid spiID
*/
static ER spi_rstGyroQueue(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		return E_ID;
	}

	memset(&(vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)]), 0, sizeof(vSpiGyroBuf[0][0])*SPI_GYRO_QUEUE_DEPTH);
	vSpiGyroQueueCnt[SPI_GYRO_ID_HASH(spiID)] = 0;
	vSpiGyroFrontIdx[SPI_GYRO_ID_HASH(spiID)] = 0;
	vSpiGyroTailIdx[SPI_GYRO_ID_HASH(spiID)] = 0;
	vSpiGyroFrameCnt[SPI_GYRO_ID_HASH(spiID)] = 0;
	vSpiGyroDataCnt[SPI_GYRO_ID_HASH(spiID)] = 0;

	return E_OK;
}

/*
    Add GYRO queue

    This function will add element to software GYRO queue

    @param[in] spiID        SPI controller ID
    @param[in] uiWord0      GYRO data word0
    @param[in] uiWord1      GYRO data word1

    @return
        - @b E_OK: success
        - @b E_QOVR: buffer overflow
*/
static ER spi_addGyroQueue(SPI_ID spiID, UINT32 uiWord0, UINT32 uiWord1)
{
	UINT32 uiIdx, uiNextIdx;

	// Due to this API is invoked by driver ISR, skip ID range check
#if 0
	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		debug_err(("%s: spi%d not support GYRO mode\r\n", __func__, spiID));
		return E_ID;
	}
#endif
	// get current write index
	uiIdx = vSpiGyroTailIdx[SPI_GYRO_ID_HASH(spiID)];

	// check next write index
	uiNextIdx = uiIdx + 1;
	if (uiNextIdx >= SPI_GYRO_QUEUE_DEPTH) {
		uiNextIdx = 0;
	}
	if (uiNextIdx == vSpiGyroFrontIdx[SPI_GYRO_ID_HASH(spiID)]) {
		// Buffer full condition
		DBG_ERR("SPI %d drv s/w buf full: spi_getGyroData() invoking count is NOT enough\r\n", spiID);
		return E_QOVR;
	}

	// Push element to queue
	vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].uiFrameID = vSpiGyroFrameCnt[SPI_GYRO_ID_HASH(spiID)];
	vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].uiDataID = vSpiGyroDataCnt[SPI_GYRO_ID_HASH(spiID)];
	vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].vRecvWord[0] = uiWord0;
	vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].vRecvWord[1] = uiWord1;
	vSpiGyroQueueCnt[SPI_GYRO_ID_HASH(spiID)]++;    // increment queue count

	// Advance write index
	vSpiGyroTailIdx[SPI_GYRO_ID_HASH(spiID)] = uiNextIdx;

	return E_OK;
}

/**
    Get GYRO queue count

    This function will return current count of GYRO queue

    @param[in] spiID        SPI controller ID

    @return current GYRO queue depth
*/
UINT32 spi_getGyroQueueCount(SPI_ID spiID)
{
	if (spiID >= SPI_ID_COUNT) {
		return 0;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return 0;
	}

	return vSpiGyroQueueCnt[SPI_GYRO_ID_HASH(spiID)];
}

/**
    Get GYRO data

    This function will get element from GYRO queue

    @param[in] spiID        SPI controller ID
    @param[out] pGyroData   pointer to return Gyro data. Caller should NOT point it to NULL.

    @return
        - @b E_OK: success
        - @b E_QOVR: buffer underflow
        - @b E_MACV: pGyroData is NULL or invalid
*/
ER spi_getGyroData(SPI_ID spiID, PGYRO_BUF_QUEUE pGyroData)
{
	UINT32 uiIdx, uiNextIdx;
	UINT32 flags;

	if (spiID >= SPI_ID_COUNT) {
		return E_ID;
	}

	if (P_HOST_CAP == NULL) {
		DBG_ERR("%s: spi_platform_init() never exec\r\n", __func__);
		return E_SYS;
	}

	if (!(P_HOST_CAP[spiID] & SPI_CAPABILITY_GYRO)) {
		DBG_ERR("spi%d not support GYRO mode\r\n", spiID);
		return E_ID;
	}

	if (pGyroData == NULL) {
		DBG_ERR("pGyroData is NULL\r\n");
		return E_MACV;
	}

	vk_spin_lock_irqsave(&spi_spinlock, flags);

	// get current read index
	uiIdx = vSpiGyroFrontIdx[SPI_GYRO_ID_HASH(spiID)];
	if (uiIdx == vSpiGyroTailIdx[SPI_GYRO_ID_HASH(spiID)]) {
		// Buffer empty condition
		vk_spin_unlock_irqrestore(&spi_spinlock, flags);
		DBG_ERR("drv s/w buf empty: %s invoking count too much\r\n", __func__);
		return E_QOVR;
	}

	// check next read index
	uiNextIdx = uiIdx + 1;
	if (uiNextIdx >= SPI_GYRO_QUEUE_DEPTH) {
		uiNextIdx = 0;
	}

	// Get element from queue
	pGyroData->uiFrameID = vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].uiFrameID;
	pGyroData->uiDataID = vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].uiDataID;
	pGyroData->vRecvWord[0] = vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].vRecvWord[0];
	pGyroData->vRecvWord[1] = vSpiGyroBuf[SPI_GYRO_ID_HASH(spiID)][uiIdx].vRecvWord[1];
	vSpiGyroQueueCnt[SPI_GYRO_ID_HASH(spiID)]--;    // decrement queue count

	// Advance read index
	vSpiGyroFrontIdx[SPI_GYRO_ID_HASH(spiID)] = uiNextIdx;

	vk_spin_unlock_irqrestore(&spi_spinlock, flags);

	return E_OK;
}

#if (_EMULATION_ == ENABLE)
// Only compare the register that won't update value from RTC macro
const static SPI_REG_DEFAULT     SPIRegDefault[] = {
	{ 0x00,     0x0002,                     "CtrlReg"     },
	{ 0x04,     0x10000,                    "IOReg"    },
	{ 0x08,     0x00,                       "CfgReg"     },
	{ 0x0C,     0x00,                       "TimingReg"    },
	{ 0x10,     0x50000,                    "FlashCtrlReg"   },
	{ 0x14,     0x00,                       "DelayReg"   },
	{ 0x18,     0x01,                       "StsReg"      },
	{ 0x1C,     0x00,                       "IntEnReg"   },
	{ 0x20,     0x00,                       "RDR_Reg"  },
	{ 0x24,     0x00,                       "TDR_Reg"  },
	{ 0x30,     0x00,                       "DmaCtrlReg"  },
	{ 0x34,     0x00,                       "DmaSizeReg"  },
	{ 0x38,     0x00,                       "DmaAddrReg"  },
	{ 0x40,     0x00,                       "AutoModeConfigReg"  },
	{ 0x44,     0x00,                       "OpIntervalReg"  },
	{ 0x48,     0x00,                       "TrsIntervalReg"  },
	{ 0x4C,     0x00,                       "GyroFifoStsReg"  },
	{ 0x50,     0x00,                       "GyroStsReg"  },
	{ 0x54,     0x00,                       "GyroIntEnReg"  },
	{ 0x58,     0x00,                       "GyroCounterReg"  },
	{ 0x5C,     0x00,                       "GryoRxDataReg"  },
	{ 0x60,     0x00,                       "Op0DataReg1"  },
	{ 0x64,     0x00,                       "Op0DataReg2"  },
	{ 0x70,     0x00,                       "Op1DataReg1"  },
	{ 0x74,     0x00,                       "Op1DataReg2"  },
	{ 0x80,     0x00,                       "Op2DataReg1"  },
	{ 0x84,     0x00,                       "Op2DataReg2"  },
	{ 0x90,     0x00,                       "Op3DataReg1"  },
	{ 0x94,     0x00,                       "Op4DataReg2"  },
};

/*
    (Verification code) Compare SPI register default value

    (Verification code) Compare SPI register default value.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL spiTest_compareRegDefaultValue(void)
{
	UINT32  uiIndex;
	BOOL    bReturn = TRUE;
	SPI_ID spiIdIdx;
	const static UINT32 vSpiBaseAddr[] = {
		IOADDR_SPI_REG_BASE,	IOADDR_SPI2_REG_BASE,
		IOADDR_SPI3_REG_BASE,	IOADDR_SPI4_REG_BASE,
		IOADDR_SPI5_REG_BASE,
	};

	// Compare register
	for (spiIdIdx = SPI_ID_1; spiIdIdx < SPI_ID_COUNT; spiIdIdx++) {
		for (uiIndex = 0; uiIndex < (sizeof(SPIRegDefault) / sizeof(SPIRegDefault[0])); uiIndex++) {
			if (INW(vSpiBaseAddr[spiIdIdx] + SPIRegDefault[uiIndex].uiOffset) != SPIRegDefault[uiIndex].uiValue) {
				DBG_ERR("SPI%d: %s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
						spiIdIdx + 1,
						SPIRegDefault[uiIndex].pName,
						SPIRegDefault[uiIndex].uiOffset,
						INW(vSpiBaseAddr[spiIdIdx] + SPIRegDefault[uiIndex].uiOffset),
						SPIRegDefault[uiIndex].uiValue);
				bReturn = FALSE;
			}
		}
	}

	return bReturn;
}

/*
    (Verification code) Setup SPI packet delay

    @param[in] uiDelay  delay between packet in DMA mode (unit: SPI clk)

    @return void
*/
void spiTest_setPktDelay(SPI_ID spiID, UINT32 uiDelay)
{
	T_SPI_CONFIG_REG configReg = {0};
	T_SPI_TIMING_REG timingReg = {0};

	configReg.reg = spi_getReg(spiID, SPI_CONFIG_REG_OFS);
	timingReg.reg = spi_getReg(spiID, SPI_TIMING_REG_OFS);

	if (uiDelay > 8191) {
		DBG_WRN("SPI%d: input delay %d exceed 8191, saturate it\r\n", spiID, uiDelay);
		uiDelay = 8191;
	}
	timingReg.bit.spi_post_cond_dly = uiDelay;
	if (uiDelay) {
		configReg.bit.spi_pkt_burst_post_cond = 1;
		configReg.bit.spi_pkt_burst_handshake_en = 1;
	} else {
		configReg.bit.spi_pkt_burst_post_cond = 0;
//        configReg.bit.spi_pkt_burst_handshake_en = 0;
	}
	spi_setReg(spiID, SPI_CONFIG_REG_OFS, configReg.reg);

	spi_setReg(spiID, SPI_TIMING_REG_OFS, timingReg.reg);
}

#endif
//@}

static void spi_drv_do_tasklet(unsigned long event)
{
    spi_ist3(event);
}

static irq_bh_handler_t spi_bh_ist(int irq, unsigned long event, void *data)
{
	spi_drv_do_tasklet(event);

	return (irq_bh_handler_t) IRQ_HANDLED;
}

void spi_platform_init(void)
{
	OS_CONFIG_FLAG(FLG_ID_SPI);
	SEM_CREATE(SEMID_SPI, 1);
	SEM_CREATE(SEMID_SPI2, 1);
	SEM_CREATE(SEMID_SPI3, 1);
	vSpiSemaphore[0] = SEMID_SPI;
	vSpiSemaphore[1] = SEMID_SPI2;
	vSpiSemaphore[2] = SEMID_SPI3;

	request_irq(INT_ID_SPI, spi_isr, IRQF_TRIGGER_HIGH, "SPI", 0);
	request_irq(INT_ID_SPI2, spi2_isr, IRQF_TRIGGER_HIGH, "SPI2", 0);
	request_irq(INT_ID_SPI3, spi3_isr, IRQF_TRIGGER_HIGH, "SPI3", 0);

	if (nvt_get_chip_id() != CHIP_NA51055) {
		P_HOST_CAP = &vSpiHostCapability[1][0];

		// not 520 => 528 has SPI4/5
		SEM_CREATE(SEMID_SPI4, 1);
		SEM_CREATE(SEMID_SPI5, 1);
		vSpiSemaphore[3] = SEMID_SPI4;
		vSpiSemaphore[4] = SEMID_SPI5;
		request_irq(INT_ID_SPI4, spi4_isr, IRQF_TRIGGER_HIGH, "SPI4", 0);
		request_irq(INT_ID_SPI5, spi5_isr, IRQF_TRIGGER_HIGH, "SPI5", 0);
	} else {
		// NT98520
		P_HOST_CAP = &vSpiHostCapability[0][0];
	}

	request_irq_bh(INT_ID_SPI3, (irq_bh_handler_t) spi_bh_ist, IRQF_BH_PRI_HIGH);
}

void spi_platform_uninit(void)
{
	free_irq(INT_ID_SPI, NULL);
	free_irq(INT_ID_SPI2, NULL);
	free_irq(INT_ID_SPI3, NULL);

	free_irq_bh(INT_ID_SPI3, NULL);

	rel_flg(FLG_ID_SPI);
	SEM_DESTROY(SEMID_SPI);
	SEM_DESTROY(SEMID_SPI2);
	SEM_DESTROY(SEMID_SPI3);

	if (nvt_get_chip_id() != CHIP_NA51055) {
		// not 520 => 528 has SPI4/5
		free_irq(INT_ID_SPI4, NULL);
		free_irq(INT_ID_SPI5, NULL);
		SEM_DESTROY(SEMID_SPI4);
		SEM_DESTROY(SEMID_SPI5);
	}

	memset(vSpiSemaphore, 0, sizeof(vSpiSemaphore));
}

//@}
