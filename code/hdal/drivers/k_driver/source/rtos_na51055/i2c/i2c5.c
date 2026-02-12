/*
    I2C5 module driver

    I2C5 module driver.

    @file       i2c5.c
    @ingroup    mIDrvIO_I2C
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#define __MODULE__    rtos_i2c
#include <kwrap/debug.h>
extern unsigned int rtos_i2c_debug_level;

#include "i2c_reg.h"
#include "i2c_int.h"
#if (I2C_HANDLE_PINMUX == ENABLE)
#include "top.h"
#endif

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

irqreturn_t i2c5_isr(int irq, void *devid);

static ID SEMID_I2C5;
static ID FLG_ID_I2C5;



// Module related definition -begin
// This is I2C5 moudle
#define MODULE_REG_BASE         IOADDR_I2C5_REG_BASE
#define MODULE_FLAG_ID          FLG_ID_I2C5
#define MODULE_FLAG_PTN         FLGPTN_I2C5
#define MODULE_SEM_ID           SEMID_I2C5



#define MODULE_CLK_ID           I2C5_CLK
#if (I2C_HANDLE_PINMUX == ENABLE)
#define MODULE_PIN1ST_ID        PINMUX_FUNC_ID_I2C5_1ST
#define MODULE_PIN2ND_ID        PINMUX_FUNC_ID_I2C5_2ND
#endif

#define module_unlock           i2c5_unlock
#define module_getConfig        i2c5_getConfig

#define MODULE_SUPPORT_SLAVE    ENABLE
#define MODULE_SUPPORT_DMA      ENABLE
// Module related definition -end

// Set register base address
REGDEF_SETIOBASE(MODULE_REG_BASE);

// Interrupt status (We don't polling the status, don't have to be volatile)
static UINT32       uiI2CIntSts;
// Session allocated status, bit value == 1 means allocated
static UINT32       uiI2CSesAllocated;
// Session locked status, bit value == 1 means locked
static UINT32       uiI2CSesLocked;
// Session modified status, bit value == 1 means modified
static UINT32       uiI2CSesModified;
// Active session
static I2C_SESSION  I2CActiveSes = I2C_TOTAL_SESSION;
// Check bus free or not (Master mode only)
static BOOL         bI2CCheckBusFree = TRUE;
// Remaining and current bytes number
static UINT32       uiI2CRemainingBytes, uiI2CCurrentBytes;
// Data byte
static I2C_BYTE    *pI2CByte;

// Configuration
// GNU extension
// Please refer to http://gcc.gnu.org/onlinedocs/gcc-4.4.7/gcc/Designated-Inits.html
static I2C_CONFIG   I2CConfig[I2C_TOTAL_SESSION] = {
	[0 ...(I2C_TOTAL_SESSION - 1)] =
	{
		I2C_MODE_MASTER,    // Master or Slave
		I2C_SAR_MODE_7BITS, // 7-bits or 10-bits
#if (I2C_HANDLE_PINMUX == ENABLE)
		I2C_PINMUX_1ST,     // 1st Pinmux
#endif
		I2C_DEFAULT_CLKLOW, // Clock low count
		I2C_DEFAULT_CLKHIGH,// Clock high count
		I2C_DEFAULT_TSR,    // TSR
		I2C_DEFAULT_GSR,    // GSR
		0x3FF,              // SCL timeout
		0x0E2,              // Bus free time
		0x000,              // SAR
		TRUE,               // Handle NACK
		FALSE,              // Response general call
		FALSE,				// uiSCLTimeoutCheckEn
		0x0, 				// uiVDSrc
		FALSE,              // bVDsync, Response VD sync mode
		FALSE,              // DMA Random Command Write mode
		0x0000,             // DMA_TRANS_SIZE2
		0x0000,             // DMA_TRANS_SIZE3
		0x00,               // DMA_TRANS_DB1
		0x00,               // DMA_TRANS_DB2
		0x00,               // DMA_TRANS_DB3
		0x0,                // DMA_VD_NUMBER
		0x000000,           // DMA_VD_DELAY
		0x00,               // DMA_VD_INTVAL
		0x00,               // DMA_VD_INTVAL2
		FALSE,				// bDescMode
#if I2C_EMU_TEST
		100000,				// timeout_ms
#else
		1000,				// timeout_ms
#endif
		FALSE,				// bus_clear
		FALSE,				// auto_rst
	}
};

/*
    Handle NACK condition when transmit data

    Handle NACK condition when transmit data. After calling this API,
    I2C controller will send STOP condition and back to idle state.

    @return void
*/
static void i2c_handleNACK(void)
{
	RCW_DEF(I2C_CONFIG_REG);
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_STS_REG);
	UINT32  uiTimeout;

	#if defined(_NVT_FPGA_) && defined(_NVT_EMULATION_)
	#if defined(_BSP_NA51000_)
	vos_task_delay_us_polling(100);
	#endif
	#endif

	// Configure controller to send STOP condition
	RCW_VAL(I2C_CONFIG_REG) = 0;
	RCW_OF(I2C_CONFIG_REG).AccessMode   = I2C_ACCESS_PIO;
	RCW_OF(I2C_CONFIG_REG).NACK_Gen0    = DISABLE;
	RCW_OF(I2C_CONFIG_REG).Start_Gen0   = DISABLE;
	RCW_OF(I2C_CONFIG_REG).Stop_Gen0    = ENABLE;
	RCW_OF(I2C_CONFIG_REG).PIODataSize  = I2C_BYTE_CNT_1;
	RCW_ST(I2C_CONFIG_REG);

	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).DMA_RCWRITE    = DISABLE;
	RCW_OF(I2C_CTRL_REG).TB_En          = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	// Wait for STOP condition sent, controller back to idle
	uiTimeout = I2C_POLLING_TIMEOUT;
	do {
		uiTimeout--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).Busy == 1) && uiTimeout);

	if (uiTimeout == 0) {
		DBG_ERR("Receive NACK, send STOP, wait for ready timeout!\r\n");
	}
}

/*
    Handle arbitration lost

    Handle arbitration lost.

    @return i2c status: I2C_STS_AL
*/
static I2C_STS i2c_handleAL(const CHAR *pFuncName)
{
	RCW_DEF(I2C_CTRL_REG);

	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).I2C_En = DISABLE;
	RCW_ST(I2C_CTRL_REG);

	RCW_OF(I2C_CTRL_REG).I2C_En = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	DBG_ERR("%s() - Arbitration lost!\r\n", pFuncName);

	return I2C_STS_AL;
}

/*
    Write transmit data & START/STOP condition to the register for transmitting operation

    Write transmit data & START/STOP condition to the register for transmitting operation.

    @return void
*/
_INLINE void i2c_setTxConfigAndData(void)
{
	RCW_DEF(I2C_CONFIG_REG);
	RCW_DEF(I2C_DATA_REG);
	UINT32      i;

	// Set parameter
	RCW_VAL(I2C_CONFIG_REG)             = 0;
	// Access mode
	RCW_OF(I2C_CONFIG_REG).AccessMode   = I2C_ACCESS_PIO;
	// PIO data size
	RCW_OF(I2C_CONFIG_REG).PIODataSize  = uiI2CCurrentBytes;

	// Data and START / STOP generator
	RCW_VAL(I2C_DATA_REG) = 0;
	for (i = 0; i < uiI2CCurrentBytes; i++) {
		RCW_VAL(I2C_DATA_REG) |= ((pI2CByte->uiValue & I2C_DATA_MASK) << (I2C_DATA_SHIFT * i));

		// Only master can send START and STOP condition
#if (MODULE_SUPPORT_SLAVE == ENABLE)
		if (I2CConfig[I2CActiveSes].Mode == I2C_MODE_MASTER)
#endif
		{
			// START condition
			if (pI2CByte->Param & I2C_BYTE_PARAM_START) {
				RCW_VAL(I2C_CONFIG_REG) |= (1 << (I2C_START_GEN_SHIFT + i));

				#if 0 //nt98520 support START and STOP at the same byte !
				// Check if START and STOP at the same byte
				if (pI2CByte->Param & I2C_BYTE_PARAM_STOP) {
					DBG_WRN("Generate START and STOP at the same byte!\r\n");
				}
				#endif
			}
			// STOP condition
			//else if (pI2CByte->Param & I2C_BYTE_PARAM_STOP) {
			if (pI2CByte->Param & I2C_BYTE_PARAM_STOP) {
				RCW_VAL(I2C_CONFIG_REG) |= (1 << (I2C_STOP_GEN_SHIFT + i));

				// Send STOP, we must check bus free for next operation
				bI2CCheckBusFree = TRUE;
			}
		}

		// Next byte
		pI2CByte++;
	}

	RCW_ST(I2C_DATA_REG);
	RCW_ST(I2C_CONFIG_REG);
}

/*
    Write START/STOP condition to the register for receiving operation

    Write START/STOP condition to the register for receiving operation.

    @return void
*/
_INLINE void i2c_setRxConfig(void)
{
	RCW_DEF(I2C_CONFIG_REG);
	I2C_BYTE   *pByte;
	UINT32      i;

	// Set parameter
	RCW_VAL(I2C_CONFIG_REG)             = 0;
	// Access mode
	RCW_OF(I2C_CONFIG_REG).AccessMode   = I2C_ACCESS_PIO;
	// PIO data size
	RCW_OF(I2C_CONFIG_REG).PIODataSize  = uiI2CCurrentBytes;

	// NACK and STOP generator
	// Don't modify global variable pI2CByte, becaure the variable
	// is required when data is received and store in FIFO.
	pByte = pI2CByte;
	for (i = 0; i < uiI2CCurrentBytes; i++) {
		// Response NACK condition
		if (pByte->Param & I2C_BYTE_PARAM_NACK) {
			RCW_VAL(I2C_CONFIG_REG) |= (1 << (I2C_NACK_GEN_SHIFT + i));
		}

		// Only master can send START condition
#if (MODULE_SUPPORT_SLAVE == ENABLE)
		if (I2CConfig[I2CActiveSes].Mode == I2C_MODE_MASTER)
#endif
		{
			// Send STOP condition
			if (pByte->Param & I2C_BYTE_PARAM_STOP) {
				RCW_VAL(I2C_CONFIG_REG) |= (1 << (I2C_STOP_GEN_SHIFT + i));
				// Send STOP, we must check bus free for next operation
				bI2CCheckBusFree = TRUE;
			}
		}

		// Next byte
		pByte++;
	}

	RCW_ST(I2C_CONFIG_REG);
}

/*
    Read received data from register to DRAM

    Read received data from register to DRAM.

    @return void
*/
_INLINE void i2c_getRxData(void)
{
	RCW_DEF(I2C_DATA_REG);
	UINT32  i;

	// Read data register
	RCW_LD(I2C_DATA_REG);
	for (i = 0; i < uiI2CCurrentBytes; i++) {
		pI2CByte->uiValue = (RCW_VAL(I2C_DATA_REG) >> (I2C_DATA_SHIFT * i)) & I2C_DATA_MASK;
		pI2CByte++;
	}
}

/*
    Interrupt Service Routine of I2C5

    Interrupt Service Routine of I2C5

    @return void
*/
irqreturn_t i2c5_isr(int irq, void *devid)
{
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_STS_REG);
	UINT32  uiNACKStatus;

	// Read status and control register
	RCW_LD(I2C_STS_REG);
	RCW_LD(I2C_CTRL_REG);

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Keep NACK_STS (bit1)
	uiNACKStatus = RCW_VAL(I2C_STS_REG) & I2C_STATUS_NACK;
#else
	uiNACKStatus = 0;
#endif

	// Handle enabled interrupt status
	RCW_VAL(I2C_STS_REG) &= (RCW_VAL(I2C_CTRL_REG) & I2C_CTRL_INTEN_MASK);

	// Keep NACK_STS (bit1)
	RCW_VAL(I2C_STS_REG) |= uiNACKStatus;

	if (RCW_VAL(I2C_STS_REG) != 0) {
		// Clear interrupt status
		RCW_ST(I2C_STS_REG);

		// Store interrupt status
		uiI2CIntSts |=  RCW_VAL(I2C_STS_REG);

		// Process remaining bytes (<= 4 bytes) if there is no error and transfer done
		// Some devices require the delay between two bytes can't exceed specific time.
		// If we don't start the next transfer ASAP, the communication might be failed
		// when task is low priority. Starting the dta transfer in ISR will make sure
		// the delay is minimum.
		if (
			(uiI2CRemainingBytes != 0) &&       // There are remaining bytes
			(
				(RCW_OF(I2C_STS_REG).DT == 1) ||   // Data transmitted
				(RCW_OF(I2C_STS_REG).DR == 1)      // Data received
			) &&
			(
#if (MODULE_SUPPORT_SLAVE == ENABLE)
				(RCW_OF(I2C_STS_REG).NACK == 0) &&     // Slave mode: receive NACK
#endif
				(RCW_OF(I2C_STS_REG).BERR == 0) &&     // Master mode: receive NACK
				(RCW_OF(I2C_STS_REG).AL == 0) &&       // Arbitration lost
				(RCW_OF(I2C_STS_REG).SCLTimeout == 0)  // SCL timeout out
			)
		) {
			uiI2CCurrentBytes   = uiI2CRemainingBytes;
			uiI2CRemainingBytes = 0;

			// Transmit, write data and configure START/STOP condition
			if (RCW_OF(I2C_STS_REG).DT == 1) {
				i2c_setTxConfigAndData();
			}
			// Receive, read data and configure START/STOP condition
			else {
				i2c_setRxConfig();
			}

			// Enable transfer
			RCW_OF(I2C_CTRL_REG).TB_En = ENABLE;
			RCW_ST(I2C_CTRL_REG);
		} else {
			// Set flag
			iset_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);
		}
	}

	return IRQ_HANDLED;
}

/**
    @addtogroup mIDrvIO_I2C
*/
//@{

/**
    Open I2C5 driver

    This function should be called before calling any other functions.

    @param[out] pSession    Available session number. If there is not available session, this will be I2C_TOTAL_SESSION.

    @return Open status
        - @b E_SYS      : There is no more free session
        - @b E_OK       : Open I2C5 driver OK, available session is returned in pSession.
*/
ER i2c5_open(PI2C_SESSION pSession)
{
	RCW_DEF(I2C_CTRL_REG);
	UINT32  i;
	unsigned long      flags;

	// Enter critical section
	loc_cpu(flags);

	// Check if any free session available
	// There is no free session
	if ((uiI2CSesAllocated & I2C_SES_ALLOCATED_MASK) == I2C_SES_ALLOCATED_MASK) {
		*pSession = I2C_TOTAL_SESSION;

		// Leave critical section
		unl_cpu(flags);

		DBG_ERR("There is no more free I2C session!\r\n");

		return E_SYS;
	}
	// All sessions are available
	else if (uiI2CSesAllocated == I2C_SES_NOT_ALLOCATED) {

		vos_flag_create(&MODULE_FLAG_ID, NULL, "FLG_ID_I2C5");
		vos_sem_create(&MODULE_SEM_ID, 1, "SEMID_I2C5");

		// clear the interrupt flag
		clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

		// Enable interrupt
		request_irq(INT_ID_I2C5, i2c5_isr ,IRQF_TRIGGER_HIGH, "i2c5", 0);

		// Enable specific interrupt event
		RCW_LD(I2C_CTRL_REG);
		RCW_OF(I2C_CTRL_REG).DT_IntEn           = ENABLE;
		RCW_OF(I2C_CTRL_REG).DR_IntEn           = ENABLE;
		RCW_OF(I2C_CTRL_REG).BERR_IntEn         = ENABLE;
#if (MODULE_SUPPORT_SLAVE == ENABLE)
		RCW_OF(I2C_CTRL_REG).STOP_IntEn         = ENABLE;
		RCW_OF(I2C_CTRL_REG).SAM_IntEn          = ENABLE;
		RCW_OF(I2C_CTRL_REG).GC_IntEn           = ENABLE;
#endif
		RCW_OF(I2C_CTRL_REG).AL_IntEn           = ENABLE;

#if (MODULE_SUPPORT_DMA == ENABLE)
		RCW_OF(I2C_CTRL_REG).DMAED_IntEn        = ENABLE;
		RCW_OF(I2C_CTRL_REG).DESCERR_IntEn      = ENABLE;
#endif

		RCW_OF(I2C_CTRL_REG).SCLTimeout_IntEn   = ENABLE;
		RCW_ST(I2C_CTRL_REG);
	}

	// Find available session
	for (i = I2C_SES0; i < I2C_TOTAL_SESSION; i++) {
		if ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << i)) == I2C_SES_NOT_ALLOCATED) {
			*pSession = i;
			uiI2CSesAllocated |= (I2C_SES_ALLOCATED << i);
			break;
		}
	}

    // PAD: disable pull up
	//pad_setPullUpDown(PAD_PIN_PGPIO22, PAD_NONE);
	//pad_setPullUpDown(PAD_PIN_PGPIO23, PAD_NONE);

	// Leave critical section
	unl_cpu(flags);

	return E_OK;
}

/**
    Close I2C5 driver for specific session

    Close I2C5 driver for specific session.

    @param[in] Session      I2C5 session

    @return Close status.
        - @b E_SYS      : The session is not opened yet
        - @b E_OK       : Session is closed
*/
ER i2c5_close(I2C_SESSION Session)
{
	unsigned long      flags;

	// Check if session is opened
	if ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) == I2C_SES_NOT_ALLOCATED) {
		DBG_ERR("I2C_SES%ld is not opened!\r\n", (UINT32)Session);
		return E_SYS;
	}

	// Check if session is unlocked
	if (uiI2CSesLocked & (I2C_SES_LOCKED << Session)) {
		module_unlock(Session);
	}

	// Enter critical section
	loc_cpu(flags);

	uiI2CSesAllocated &= ~(I2C_SES_ALLOCATED << Session);

	// No more session is opened
	if (uiI2CSesAllocated == I2C_SES_NOT_ALLOCATED) {
		// Disable interrupt
		free_irq(INT_ID_I2C5, 0);

		vos_flag_destroy(MODULE_FLAG_ID);
		vos_sem_destroy(MODULE_SEM_ID);
	}

	// Leave critical section
	unl_cpu(flags);

	return E_OK;
}

/**
    Check if I2C5 driver is opened or not for specific session

    Check if I2C5 driver is opened or not for specific session.

    @param[in] Session      I2C5 session

    @return
        - @b TRUE   : I2C5 session is opened
        - @b FALSE  : I2C5 session is not opened
*/

BOOL i2c5_isOpened(I2C_SESSION Session)
{
	return ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) != I2C_SES_NOT_ALLOCATED);
}

/**
    Lock I2C5 session

    Lock I2C5 session. You should lock I2C5 session before sending / receving any data,
    and unlock I2C5 session after data is transferred.

    @param[in] Session      I2C5 session

    @return Lock status.
        - @b E_ID       : Outside semaphore ID number range
        - @b E_NOEXS    : Semaphore does not yet exist
        - @b E_SYS      : I2C5 session is not opened
        - @b E_OK       : I2C5 session is locked
*/
ER i2c5_lock(I2C_SESSION Session)
{
	ER  erReturn;
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_BUSCLK_REG);
	RCW_DEF(I2C_TIMING_REG);
	RCW_DEF(I2C_BUSFREE_REG);
	RCW_DEF(I2C_DMA_VD_SIZE_REG);
	RCW_DEF(I2C_VD_CONTROL_REG);
	RCW_DEF(I2C_VD_DELAY_REG);

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	RCW_DEF(I2C_SAR_REG);
#endif

	// Check if session is opened
	if ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) == I2C_SES_NOT_ALLOCATED) {
		DBG_ERR("I2C_SES%ld is not opened!\r\n", (UINT32)Session);
		return E_SYS;
	}

	// Wait semaphore
	if ((erReturn = vos_sem_wait(MODULE_SEM_ID)) != E_OK) {
		return erReturn;
	}

	uiI2CSesLocked |= (I2C_SES_ALLOCATED << Session);

	// Clear interrupt status
	uiI2CIntSts = 0;

	// Enable module clock
	pll_enable_clock(MODULE_CLK_ID);

	// Enable I2C controller and slave or master mode
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).I2C_En = ENABLE;
	RCW_OF(I2C_CTRL_REG).SCLTimeout_IntEn = I2CConfig[Session].uiSCLTimeoutCheckEn;

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	RCW_OF(I2C_CTRL_REG).SCL_En = I2CConfig[Session].Mode;
	RCW_OF(I2C_CTRL_REG).GC_En  = I2CConfig[Session].bGeneralCall;
#endif

	RCW_ST(I2C_CTRL_REG);

	// Config I2C controller if previous session is not matched current session
	// or session configuration is modified
	if ((Session != I2CActiveSes) ||
		((uiI2CSesModified & (I2C_SES_MODIFIED << Session)) != 0)) {
		// Save active session
		I2CActiveSes = Session;

		// Remove modified attribute
		uiI2CSesModified &= ~(I2C_SES_MODIFIED << Session);

#if 0//(I2C_HANDLE_PINMUX == ENABLE)
		if (I2CConfig[Session].PinMux == I2C_PINMUX_1ST) {
			pinmux_setPinmux(MODULE_PIN1ST_ID, PINMUX_I2C_SEL_ACTIVE);
			pinmux_setPinmux(MODULE_PIN2ND_ID, PINMUX_I2C_SEL_INACTIVE);
		} else {
			pinmux_setPinmux(MODULE_PIN1ST_ID, PINMUX_I2C_SEL_INACTIVE);
			pinmux_setPinmux(MODULE_PIN2ND_ID, PINMUX_I2C_SEL_ACTIVE);
		}
#endif

		// Bus clock
		RCW_OF(I2C_BUSCLK_REG).LowCounter   = I2CConfig[Session].uiClkLowCnt;
		RCW_OF(I2C_BUSCLK_REG).HighCounter  = I2CConfig[Session].uiClkHighCnt;
		RCW_ST(I2C_BUSCLK_REG);

#if (MODULE_SUPPORT_SLAVE == ENABLE)
		// Slave address
		if (I2CConfig[Session].Mode == I2C_MODE_SLAVE) {
			RCW_OF(I2C_SAR_REG).SAR         = I2CConfig[Session].uiSAR;
			RCW_OF(I2C_SAR_REG).TenBits     = I2CConfig[Session].SARMode;
			RCW_ST(I2C_SAR_REG);
		}
#endif

		// TSR, GSR, SCL timeout
		RCW_LD(I2C_TIMING_REG);
		RCW_OF(I2C_TIMING_REG).TSR          = I2CConfig[Session].uiTSR;
		RCW_OF(I2C_TIMING_REG).GSR          = I2CConfig[Session].uiGSR;
		RCW_OF(I2C_TIMING_REG).SCLTimeout   = I2CConfig[Session].uiSCLTimeout;
		RCW_ST(I2C_TIMING_REG);

		// Bus Free time
		RCW_OF(I2C_BUSFREE_REG).BusFree     = I2CConfig[Session].uiBusFree;
		RCW_ST(I2C_BUSFREE_REG);

		// VD Source
		if (nvt_get_chip_id() == CHIP_NA51055) {
			RCW_OF(I2C_CTRL_REG).VD_SRC         = (BOOL)I2CConfig[Session].uiVDSrc;
		} else if (nvt_get_chip_id() == CHIP_NA51084) {
			RCW_OF(I2C_CTRL_REG).VD_SRC         = I2CConfig[Session].uiVDSrc;
		} else {
			RCW_OF(I2C_CTRL_REG).VD_SRC         = (BOOL)I2CConfig[Session].uiVDSrc;
			DBG_ERR("get chip id %d out of range\r\n", nvt_get_chip_id());
		}

		// VD Sync mode
		RCW_OF(I2C_CTRL_REG).VD_SYNC        = I2CConfig[Session].bVDsync;

		// DMA Randmo Write mode
		RCW_OF(I2C_CTRL_REG).DMA_RCWRITE    = I2CConfig[Session].bRCWrite;

		// DMA Desc mode
		RCW_OF(I2C_CTRL_REG).DMA_DESC       = I2CConfig[Session].bDescMode;
		RCW_ST(I2C_CTRL_REG);

		// I2C DMA random command write mode for the size of Transfer Group2
		RCW_OF(I2C_DMA_VD_SIZE_REG).DMA_TRANS_SIZE2  = I2CConfig[Session].uiSize2;
		RCW_ST(I2C_DMA_VD_SIZE_REG);

		// I2C DMA random command write mode for size of Transfer Group3
		RCW_OF(I2C_DMA_VD_SIZE_REG).DMA_TRANS_SIZE3  = I2CConfig[Session].uiSize3;
		RCW_ST(I2C_DMA_VD_SIZE_REG);


		// I2C DMA random command write mode for Data Byte 1
		RCW_OF(I2C_VD_CONTROL_REG).DMA_TRANS_DB1  = I2CConfig[Session].uiDB1;
		RCW_ST(I2C_VD_CONTROL_REG);


		// I2C DMA random command write mode for Data Byte 2
		RCW_OF(I2C_VD_CONTROL_REG).DMA_TRANS_DB2  = I2CConfig[Session].uiDB2;
		RCW_ST(I2C_VD_CONTROL_REG);

		// I2C DMA random command write mode for Data Byte 3
		RCW_OF(I2C_VD_CONTROL_REG).DMA_TRANS_DB3  = I2CConfig[Session].uiDB3;
		RCW_ST(I2C_VD_CONTROL_REG);

		// I2C VD sync mode frame number
		RCW_OF(I2C_VD_CONTROL_REG).DMA_VD_NUMBER  = I2CConfig[Session].uiVDNumber;
		RCW_ST(I2C_VD_CONTROL_REG);


		// I2C VD sync Delay
		RCW_OF(I2C_VD_DELAY_REG).DMA_VD_DELAY  = I2CConfig[Session].uiVDDelay;
		RCW_ST(I2C_VD_DELAY_REG);

		// I2C VD sync interval between DMA_TRANS_G1 and DMA_TRANS_G2
		RCW_OF(I2C_VD_DELAY_REG).DMA_VD_INTVAL  = I2CConfig[Session].uiVDIntval;
		RCW_ST(I2C_VD_DELAY_REG);

		// I2C VD sync interval between DMA_TRANS_G2 and DMA_TRANS_G3
		RCW_OF(I2C_VD_DELAY_REG).DMA_VD_INTVAL2  = I2CConfig[Session].uiVDIntval2;
		RCW_ST(I2C_VD_DELAY_REG);

	}

	// We must check bus free for next operation
	// Only check bus free at master mode
#if (MODULE_SUPPORT_SLAVE == ENABLE)
	if (I2CConfig[Session].Mode == I2C_MODE_MASTER) {
		bI2CCheckBusFree = TRUE;
	} else {
		bI2CCheckBusFree = FALSE;
	}
#else
	bI2CCheckBusFree = TRUE;
#endif

	// Pre-set flag
	set_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	return E_OK;
}

/**
    Unlock I2C5 session

    Unlock I2C5 session. You should unlock I2C5 session and let other task can use I2C5.

    @param[in] Session      I2C5 session

    @return Unlock status.
        - @b E_ID       : Outside semaphore ID number range
        - @b E_NOEXS    : Semaphore does not yet exist
        - @b E_QOVR     : Semaphore's counter error, maximum counter < counter
        - @b E_SYS      : I2C5 session is not opened or not locked
        - @b E_OK       : I2C5 session is unlocked
*/
ER i2c5_unlock(I2C_SESSION Session)
{
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_STS_REG);
	FLGPTN  FlagPtn;
	UINT32  uiTimeout;
	ER rt = E_OK;

	// Check if session is opened
	if (
		((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) == I2C_SES_NOT_ALLOCATED) ||
		((uiI2CSesLocked & (I2C_SES_LOCKED << Session)) == I2C_SES_NOT_LOCKED)
	) {
		DBG_ERR("I2C_SES%d is not opened or locked!\r\n", (INT)Session);
		return E_SYS;
	}

	// Wait for i2c controller is not busy
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[Session].timeout_ms)) != E_OK) {
		DBG_ERR("Session %d timeout %d ms\r\n", Session, I2CConfig[Session].timeout_ms);
		rt = E_TMOUT;
		goto timeout;
	}

	uiTimeout = I2C_POLLING_TIMEOUT;
	do {
		uiTimeout--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).Busy == 1) && uiTimeout);

	if (uiTimeout == 0) {
		DBG_ERR("Wait for ready timeout!\r\n");
	}

timeout :
	uiI2CSesLocked &= ~(I2C_SES_ALLOCATED << Session);

	// Disable I2C controller
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).I2C_En = DISABLE;
	RCW_ST(I2C_CTRL_REG);

	if (I2CConfig[Session].uiSCLTimeoutCheckEn == DISABLE) {
		RCW_VAL(I2C_STS_REG) = 0;
		RCW_OF(I2C_STS_REG).SCLTimeout = 1;
		RCW_ST(I2C_STS_REG);
	}

	// Disable module clock
	pll_disable_clock(MODULE_CLK_ID);

	vos_sem_sig(MODULE_SEM_ID);

	return rt;
}

/**
    Check if I2C5 Session is locked or not.

    Check if I2C5 Session is locked or not.

    @param[in] Session      I2C5 session

    @return
        - @b TRUE       : Specified Session is locked.
        - @b FALSE      : Specified Session is unlocked.
*/
BOOL i2c5_islocked(I2C_SESSION Session)
{
	return (uiI2CSesLocked & (I2C_SES_ALLOCATED << Session)) > 0;
}

/**
    Configure I2C5

    Configuration for specific I2C5 session.

    @param[in] Session      I2C5 session that will be configured
    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value

    @return void
*/
void i2c5_setConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	// Check session
	if (Session >= I2C_TOTAL_SESSION) {
		DBG_ERR("Invalid session!\r\n");
		return;
	}

	// Check if session is opened
	if ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) == I2C_SES_NOT_ALLOCATED) {
		DBG_ERR("I2C_SES%d is not opened!\r\n", (INT)Session);
		return;
	}

	// Configuration of this session is modified
	uiI2CSesModified |= (I2C_SES_MODIFIED << Session);

	switch (ConfigID) {
	// Master or Slave
	case I2C_CONFIG_ID_MODE:
		if (uiConfig > I2C_MODE_MASTER) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].Mode = uiConfig;
		}
		break;

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Slave address
	case I2C_CONFIG_ID_SAR:
		I2CConfig[Session].uiSAR = uiConfig & I2C_SAR_REG_MASK;
		break;

	// 7-bits or 10 bits slave address mode
	case I2C_CONFIG_ID_SAR_MODE:
		if (uiConfig > I2C_SAR_MODE_10BITS) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].SARMode = uiConfig;
		}
		break;

	// Response general call
	case I2C_CONFIG_ID_GC:
		if (uiConfig > TRUE) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].bGeneralCall = (BOOL)uiConfig;
		}
		break;
#endif

	// Bus clock
	case I2C_CONFIG_ID_BUSCLOCK:
#ifndef _NVT_EMULATION_
		if ((uiConfig < I2C_BUS_CLOCK_50KHZ) || (uiConfig > I2C_BUS_CLOCK_1MHZ)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else
#endif
		{
			UINT32  uiMinLowCnt, uiMinHighCnt;

			// uiGSR is fixed
//			I2CConfig[Session].uiGSR         = I2C_DEFAULT_GSR;

			// uiBusClock = (I2C_SOURCE_CLOCK / (uiClkLowCnt + uiClkHighCnt + uiGSR)
			// Assume uiClkLowCnt = uiClkHighCnt

			I2CConfig[Session].uiClkLowCnt  =
				I2CConfig[Session].uiClkHighCnt = ((I2C_SOURCE_CLOCK / uiConfig) - I2CConfig[Session].uiGSR) >> 1;

			while (module_getConfig(Session, I2C_CONFIG_ID_BUSCLOCK) > uiConfig) {
				I2CConfig[Session].uiClkLowCnt++;
			}

			// Standard mode (bus clock <= 100 KHz)
			if (uiConfig <= I2C_BUS_CLOCK_100KHZ) {
				// Clock low period must >= 4.7 us, clock high period must >= 4 us
				uiMinLowCnt  = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)4700) + 1;
				uiMinHighCnt = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)4000) + 1;
			}
			// Fast mode (bus clock <= 400 KHz)
			else if (uiConfig <= I2C_BUS_CLOCK_400KHZ) {
				// Clock low period must >= 1.3 us, clock high period must >= 0.6 us
				uiMinLowCnt  = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)1300) + 1;
				uiMinHighCnt = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)600) + 1;
			}
			// Fast mode plus (bus clock <= 1 MHz)
			else {
				// Clock low period must >= 0.5 us, clock high period must >= 0.26 us
				uiMinLowCnt  = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)500) + 1;
				uiMinHighCnt = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)260) + 1;
			}

			// Find correct clock low period
			if (I2CConfig[Session].uiClkLowCnt < uiMinLowCnt) {
				I2CConfig[Session].uiClkHighCnt -= (uiMinLowCnt - I2CConfig[Session].uiClkLowCnt);
				I2CConfig[Session].uiClkLowCnt  = uiMinLowCnt;
			}

			if (I2CConfig[Session].uiClkLowCnt > I2C_CLKLOW_MAX) {
				I2CConfig[Session].uiClkLowCnt = I2C_CLKLOW_MAX;
			}

			// Find correct clock high period
			if (I2CConfig[Session].uiClkHighCnt < uiMinHighCnt) {
				I2CConfig[Session].uiClkHighCnt = uiMinHighCnt;
			}

			if (I2CConfig[Session].uiClkHighCnt > I2C_CLKHIGH_MAX) {
				I2CConfig[Session].uiClkHighCnt = I2C_CLKHIGH_MAX;
			}

			// Adjust to real register settings
			I2CConfig[Session].uiClkLowCnt -= 1;
			if (I2CConfig[Session].uiClkLowCnt < I2C_CLKLOW_MIN) {
				I2CConfig[Session].uiClkLowCnt = I2C_CLKLOW_MIN;
			}
			I2CConfig[Session].uiClkHighCnt -= 3;
			if (I2CConfig[Session].uiClkHighCnt < I2C_CLKHIGH_MIN) {
				I2CConfig[Session].uiClkHighCnt = I2C_CLKHIGH_MIN;
			}

			while (module_getConfig(Session, I2C_CONFIG_ID_BUSCLOCK) > uiConfig) {
				I2CConfig[Session].uiClkLowCnt++;
			}

#if defined(_NVT_FPGA_)
			// In FPGA, source clock is slower than real chip, our formula might
			// cause variable changed from positive value to negative value.
			if (I2CConfig[Session].uiGSR <= 4) {
				if (I2CConfig[Session].uiClkLowCnt < 8) {
					I2CConfig[Session].uiClkLowCnt = 8;
				}
			} else {
				if (I2CConfig[Session].uiClkLowCnt < (4 + I2CConfig[Session].uiGSR)) {
					I2CConfig[Session].uiClkLowCnt = (4 + I2CConfig[Session].uiGSR);
				}
			}
#endif

#if 0
			// Set setup / hold time to clock * 1/4
			// Controller will drive the data bus after "uiTSR + 4" clock cycles when SCL goes low.
			// uiTSR + 4 = uiClkLowCnt / 2;
			I2CConfig[Session].uiTSR = (I2CConfig[Session].uiClkLowCnt >> 1) - 4;
#else
			// Fix TSR as 0x1. About 0.1us.
			I2CConfig[Session].uiTSR = 1;
#endif

			// Clock low counter must > (4 + GSR + TSR)
			if (I2CConfig[Session].uiTSR >= (I2CConfig[Session].uiClkLowCnt - 4 - I2CConfig[Session].uiGSR)) {
				I2CConfig[Session].uiTSR = I2CConfig[Session].uiClkLowCnt - 5 - I2CConfig[Session].uiGSR;
			}

			// TSR: 1 ~ 1023
			if (I2CConfig[Session].uiTSR < I2C_TSR_MIN) {
				I2CConfig[Session].uiTSR = I2C_TSR_MIN;
			} else if (I2CConfig[Session].uiTSR > I2C_TSR_MAX) {
				I2CConfig[Session].uiTSR = I2C_TSR_MAX;
			}
		}
		break;

	// SCL timeout
	case I2C_CONFIG_ID_SCL_TIMEOUT:
		if ((uiConfig < I2C_SCL_TIMEOUT_MIN) || (uiConfig > I2C_SCL_TIMEOUT_MAX)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiSCLTimeout = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)uiConfig) >> 16;
			if (I2CConfig[Session].uiSCLTimeout == 0) {
				I2CConfig[Session].uiSCLTimeout = 1;
			}
		}
		break;

	case I2C_CONFIG_ID_SCLTIMEOUT_CHECK:
		I2CConfig[Session].uiSCLTimeoutCheckEn = uiConfig > 0;
		break;

	// Bus free time
	case I2C_CONFIG_ID_BUSFREE_TIME:
		if ((uiConfig < I2C_BUSFREE_TIME_MIN) || (uiConfig > I2C_BUSFREE_TIME_MAX)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiBusFree = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)uiConfig) & I2C_BUSFREE_REG_MASK;
			if (I2CConfig[Session].uiBusFree == 0) {
				I2CConfig[Session].uiBusFree = 1;
			}
		}
		break;

	// Handle NACK or not
	case I2C_CONFIG_ID_HANDLE_NACK:
		if (uiConfig > TRUE) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].bHandleNACK = (BOOL)uiConfig;
		}
		break;

		// Pinmux
#if (I2C_HANDLE_PINMUX == ENABLE)
	case I2C_CONFIG_ID_PINMUX:
		if (uiConfig > I2C_PINMUX_1ST) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].PinMux = uiConfig;
		}
		break;
#endif

#ifdef _NVT_EMULATION_
	// Clock low counter
	case I2C_CONFIG_ID_CLKCNT_L:
		if ((uiConfig > I2C_CLKLOW_MAX) || (uiConfig < I2C_CLKLOW_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiClkLowCnt = uiConfig;
		}
		break;

	// Clock high counter
	case I2C_CONFIG_ID_CLKCNT_H:
		if ((uiConfig > I2C_CLKHIGH_MAX) || (uiConfig < I2C_CLKHIGH_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiClkHighCnt = uiConfig;
		}
		break;

#endif

	// TSR
	case I2C_CONFIG_ID_TSR:
		if ((uiConfig > I2C_TSR_MAX) || (uiConfig < I2C_TSR_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiTSR = uiConfig;
		}
		break;
	// GSR
	case I2C_CONFIG_ID_GSR:
		if (uiConfig > I2C_GSR_MAX) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiGSR = uiConfig;
		}
		break;

	//VD Sync
	case I2C_CONFIG_ID_VD:
		if (uiConfig > TRUE) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].bVDsync = uiConfig;
		}
		break;

	//DMA_RCWrite
	case I2C_CONFIG_ID_DMA_RCWrite:
		if (uiConfig > TRUE) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].bRCWrite = uiConfig;
		}
		break;

	//DMA_TRANS_SIZE2
	case I2C_CONFIG_ID_DMA_TRANS_SIZE2:
		if ((uiConfig > I2C_DMA_SIZE_MAX) || (uiConfig < I2C_DMA_SIZE_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiSize2 = uiConfig;
		}
		break;

	//DMA_TRANS_SIZE3
	case I2C_CONFIG_ID_DMA_TRANS_SIZE3:
		if ((uiConfig > I2C_DMA_SIZE_MAX) || (uiConfig < I2C_DMA_SIZE_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiSize3 = uiConfig;
		}
		break;

	// I2C DMA random command write mode for Data Byte 1
	case I2C_CONFIG_ID_DMA_TRANS_DB1:
		if ((uiConfig > I2C_DMA_DB_MAX) || (uiConfig < I2C_DMA_DB_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiDB1 = uiConfig;
		}
		break;

	// I2C DMA random command write mode for Data Byte 2
	case I2C_CONFIG_ID_DMA_TRANS_DB2:
		if ((uiConfig > I2C_DMA_DB_MAX) || (uiConfig < I2C_DMA_DB_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiDB2 = uiConfig;
		}
		break;

	// I2C DMA random command write mode for Data Byte 3
	case I2C_CONFIG_ID_DMA_TRANS_DB3:
		if ((uiConfig > I2C_DMA_DB_MAX) || (uiConfig < I2C_DMA_DB_MIN)) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiDB3 = uiConfig;
		}
		break;

	// I2C VD sync mode source
	case I2C_CONFIG_ID_DMA_VD_SRC:
		if (nvt_get_chip_id() == CHIP_NA51055) {
			if (uiConfig > I2C_VD_SIE2) {
				DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
			} else {
				I2CConfig[Session].uiVDSrc = uiConfig;
			}
		} else if  (nvt_get_chip_id() == CHIP_NA51084) {
			if (uiConfig > I2C_VD_SIE5) {
				DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
			} else {
				I2CConfig[Session].uiVDSrc = uiConfig;
			}
		} else {
			DBG_ERR("get chip id %d out of range\r\n", nvt_get_chip_id());
			if (uiConfig > I2C_VD_SIE2) {
				DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
			} else {
				I2CConfig[Session].uiVDSrc = uiConfig;
			}
		}
		break;

	// I2C VD sync mode frame number
	case I2C_CONFIG_ID_DMA_VD_NUMBER:
		if (uiConfig > I2C_VD_NUMBER_3) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiVDNumber = uiConfig;
		}
		break;

	// I2C VD sync Delay
	case I2C_CONFIG_ID_DMA_VD_DELAY:
		if (uiConfig > I2C_VD_DELAY_NS_MAX) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiVDDelay = (UINT32)(((FLOAT)I2C_SOURCE_CLOCK / (FLOAT)1000000000) * (FLOAT)uiConfig);
			if (I2CConfig[Session].uiVDDelay == 0) {
				I2CConfig[Session].uiVDDelay = 1;
			}
		}
		break;

	// I2C VD sync interval between DMA_TRANS_G1 and DMA_TRANS_G2
	case I2C_CONFIG_ID_DMA_VD_INTVAL:
		if (uiConfig > I2C_VD_INTVAL_7) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiVDIntval = uiConfig;
		}
		break;

	// I2C VD sync interval between DMA_TRANS_G2 and DMA_TRANS_G3
	case I2C_CONFIG_ID_DMA_VD_INTVAL2:
		if (uiConfig > I2C_VD_INTVAL_7) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].uiVDIntval2 = uiConfig;
		}
		break;

	//DMA_DESC
	case I2C_CONFIG_ID_DMA_DESC:
		if (uiConfig > TRUE) {
			DBG_ERR("Invalid value (%d) for ID %d!\r\n", (INT)uiConfig, (INT)ConfigID);
		} else {
			I2CConfig[Session].bDescMode= uiConfig;
		}
		break;

	//timeout
	case I2C_CONFIG_ID_TIMEOUT_MS:
		I2CConfig[Session].timeout_ms = uiConfig;
		break;

	case I2C_CONFIG_ID_BUS_CLEAR:
		I2CConfig[Session].bus_clear = uiConfig;
		break;

	case I2C_CONFIG_ID_AUTO_RST:
		I2CConfig[Session].auto_rst = uiConfig;
		break;

	default:
		DBG_ERR("Not supported ID (%d)!\r\n", (INT)ConfigID);
		break;
	}
}

/**
    Get I2C5 configuration

    Get I2C5 configuration for specific session.

    @param[in] Session      I2C5 session that will be probed
    @param[in] ConfigID     Configuration ID

    @return Configuration value.
*/
UINT32 i2c5_getConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID)
{
	// Check session
	if (Session >= I2C_TOTAL_SESSION) {
		DBG_ERR("Invalid session!\r\n");
		return 0;
	}

	// Check if session is opened
	if ((uiI2CSesAllocated & (I2C_SES_ALLOCATED << Session)) == I2C_SES_NOT_ALLOCATED) {
		DBG_ERR("I2C_SES%d is not opened!\r\n", (INT)Session);
		return 0;
	}

	switch (ConfigID) {
#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Master or Slave
	case I2C_CONFIG_ID_MODE:
		return I2CConfig[Session].Mode;

	// Slave address
	case I2C_CONFIG_ID_SAR:
		return I2CConfig[Session].uiSAR;

	// 7-bits or 10 bits slave address mode
	case I2C_CONFIG_ID_SAR_MODE:
		return I2CConfig[Session].SARMode;

	// Response general call
	case I2C_CONFIG_ID_GC:
		return I2CConfig[Session].bGeneralCall;
#endif

	// Bus clock
	case I2C_CONFIG_ID_BUSCLOCK:
		return (I2C_SOURCE_CLOCK / (I2CConfig[Session].uiClkLowCnt + I2CConfig[Session].uiClkHighCnt + I2CConfig[Session].uiGSR + 4));

	// SCL timeout
	case I2C_CONFIG_ID_SCL_TIMEOUT:
		return (UINT32)(((FLOAT)1000000000 / (FLOAT)I2C_SOURCE_CLOCK) * (FLOAT)(I2CConfig[Session].uiSCLTimeout << 16));

	// Bus free time
	case I2C_CONFIG_ID_BUSFREE_TIME:
		return (UINT32)(((FLOAT)1000000000 / (FLOAT)I2C_SOURCE_CLOCK) * (FLOAT)(I2CConfig[Session].uiBusFree << 16));

	// Handle NACK or not
	case I2C_CONFIG_ID_HANDLE_NACK:
		return (UINT32)I2CConfig[Session].bHandleNACK;

#if (I2C_HANDLE_PINMUX == ENABLE)
	// Pinmux
	case I2C_CONFIG_ID_PINMUX:
		return I2CConfig[Session].PinMux;
#endif

#ifdef _NVT_EMULATION_
	// Clock
	case I2C_CONFIG_ID_CLKCNT_L:
		return I2CConfig[Session].uiClkLowCnt;

	case I2C_CONFIG_ID_CLKCNT_H:
		return I2CConfig[Session].uiClkHighCnt;

#endif

	// TSR
	case I2C_CONFIG_ID_TSR:
		return I2CConfig[Session].uiTSR;
	// GSR
	case I2C_CONFIG_ID_GSR:
		return I2CConfig[Session].uiGSR;

	// VD sync mode
	case I2C_CONFIG_ID_VD:
		return I2CConfig[Session].bVDsync;

	// DMA Random Command Write mode
	case I2C_CONFIG_ID_DMA_RCWrite:
		return I2CConfig[Session].bRCWrite;

	// I2C DMA random command write mode for the size of Transfer Group2
	case I2C_CONFIG_ID_DMA_TRANS_SIZE2:
		return I2CConfig[Session].uiSize2;

	// I2C DMA random command write mode for size of Transfer Group3
	case I2C_CONFIG_ID_DMA_TRANS_SIZE3:
		return I2CConfig[Session].uiSize3;

	// I2C DMA random command write mode for Data Byte 1
	case I2C_CONFIG_ID_DMA_TRANS_DB1:
		return I2CConfig[Session].uiDB1;

	// I2C DMA random command write mode for Data Byte 2
	case I2C_CONFIG_ID_DMA_TRANS_DB2:
		return I2CConfig[Session].uiDB2;

	// I2C DMA random command write mode for Data Byte 3
	case I2C_CONFIG_ID_DMA_TRANS_DB3:
		return I2CConfig[Session].uiDB3;

	// I2C VD sync mode frame number
	case I2C_CONFIG_ID_DMA_VD_NUMBER:
		return I2CConfig[Session].uiVDNumber;

	// I2C VD sync Delay
	case I2C_CONFIG_ID_DMA_VD_DELAY: {
			return (UINT32)(((FLOAT)1000000000 / (FLOAT)I2C_SOURCE_CLOCK) * (FLOAT)(I2CConfig[Session].uiVDDelay));
		}

	// I2C VD sync interval between DMA_TRANS_G1 and DMA_TRANS_G2
	case I2C_CONFIG_ID_DMA_VD_INTVAL:
		return I2CConfig[Session].uiVDIntval;

	// I2C VD sync interval between DMA_TRANS_G2 and DMA_TRANS_G3
	case I2C_CONFIG_ID_DMA_VD_INTVAL2:
		return I2CConfig[Session].uiVDIntval2;

	// DMA Desc mode
	case I2C_CONFIG_ID_DMA_DESC:
		return I2CConfig[Session].bDescMode;

	// DMA DESC information
	case I2C_CONFIG_ID_DMA_DESC_CNT:
	case I2C_CONFIG_ID_DMA_DESC_POLL1:
	case I2C_CONFIG_ID_DMA_DESC_POLL2:
		{
			UINT32 ret = 0;

			if (!I2CConfig[Session].bDescMode) {
				DBG_WRN("ConfigID %d only support in desc mode\r\n", (INT)ConfigID);
			}
			RCW_DEF(DMA_DESC_INFO_REG);
			RCW_LD(DMA_DESC_INFO_REG);
			if (ConfigID == I2C_CONFIG_ID_DMA_DESC_CNT) {
				ret = RCW_OF(DMA_DESC_INFO_REG).DMA_DESC_COUNTER;
			}
			if (ConfigID == I2C_CONFIG_ID_DMA_DESC_POLL1) {
				ret = RCW_OF(DMA_DESC_INFO_REG).DMA_POLL_DATA1;
			}
			if (ConfigID == I2C_CONFIG_ID_DMA_DESC_POLL2) {
				ret = RCW_OF(DMA_DESC_INFO_REG).DMA_POLL_DATA2;
			}
			return ret;

		}

	// timeout
	case I2C_CONFIG_ID_TIMEOUT_MS:
		return I2CConfig[Session].timeout_ms;

	case I2C_CONFIG_ID_BUS_CLEAR:
		return I2CConfig[Session].bus_clear;

	case I2C_CONFIG_ID_AUTO_RST:
		return I2CConfig[Session].auto_rst;

	default:
		DBG_ERR("Not supported ID (%ld)!\r\n", (UINT32)ConfigID);
		return 0;
	}
}

/**
    Transmit I2C5 data in PIO mode

    Transmit I2C5 data in PIO mode for I2C_SESx that are locked.

    @param[in] pData    PIO data to transmit, including START / STOP condition

    @note               START and STOP can't be generated at the same byte. And STOP can't
                        be followed by START in one function call.

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Byte count is not 1 ~ 8, no START or I2C5 is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_NACK               : Receive NACK, could be OR with I2C_STS_STOP.
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is transmitted, could be OR with I2C_STS_STOP.
        - @b I2C_STS_STOP               : Slave mode only, receive STOP condition from master.
*/
I2C_STS i2c5_transmit(PI2C_DATA pData)
{
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CTRL_REG);
	UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;

	// Check parameter
	if ((pData->ByteCount > I2C_BYTE_CNT_8) ||
		(pData->ByteCount < I2C_BYTE_CNT_1)) {
		DBG_ERR("Invalid byte count!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// uiI2CIntSts will be cleared after transmitted.
	// If the value is not 0, it means there are some interrupts issued after transmitted.
	if (uiI2CIntSts != 0) {
		// Get interrupt status
		RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

		// STOP interrupt is issued after DT is received
		if (RCW_OF(I2C_STS_REG).STOP == 1) {
			return (I2C_STS_OK | I2C_STS_STOP);
		}
	}
#endif

	// Wait for bus free
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((((bI2CCheckBusFree == TRUE) &&
			   (RCW_OF(I2C_STS_REG).BusFree == 0)) ||       // STOP is issued and not available to issue START
			  (RCW_OF(I2C_STS_REG).BusBusy == 1)) && i);    // Bus is busy (START to STOP) and controller is not involved in

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// If 1st data without START condition, I2C controller must be in busy state
	if (((pData->pByte->Param & I2C_BYTE_PARAM_START) == 0) &&
		(RCW_OF(I2C_STS_REG).Busy == 0)) {
		DBG_ERR("No START condition!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Don't check bus free for next operation
	bI2CCheckBusFree = FALSE;

	// Store the pointer point to I2C_BYTE array
	pI2CByte            = pData->pByte;
	// Store current transmit byte count
	uiI2CCurrentBytes   = (pData->ByteCount > I2C_BYTE_CNT_4) ? (pData->ByteCount - I2C_BYTE_CNT_4) : pData->ByteCount;
	// Store not transmitted byte count.
	// (FW support up to 8 bytes, Controller support up to 4 bytes, we can transmit all remaining data in ISR)
	uiI2CRemainingBytes = pData->ByteCount - uiI2CCurrentBytes;

	// Write data and configure START/STOP condition
	i2c_setTxConfigAndData();

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).TB_En = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	// Wait for data transmitted
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
		DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// If controller detect STOP or NACK in slave mode before pData->ByteCount are transmitted,
	// the controller will only issue STOP or NACK interrupt.
	if ((I2CConfig[I2CActiveSes].Mode == I2C_MODE_SLAVE) &&
		(RCW_OF(I2C_STS_REG).DT == 0)) {
		DBG_DUMP("%s: Detect %s %s before %d bytes transmitted!\r\n",
				 __func__,
				 (INT)RCW_OF(I2C_STS_REG).NACK == 1 ? "NACK" : "",
				 (INT)RCW_OF(I2C_STS_REG).STOP == 1 ? "STOP" : "",
				 (INT)pData->ByteCount);
	}
#endif

	// NACK
#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Slave mode, check NACK bit
	if (I2CConfig[I2CActiveSes].Mode == I2C_MODE_SLAVE) {
		if (RCW_OF(I2C_STS_REG).NACK == 1) {
			// Check STOP condition
			if (RCW_OF(I2C_STS_REG).STOP == 1) {
				return (I2C_STS_NACK | I2C_STS_STOP);
			} else {
				return I2C_STS_NACK;
			}
		}
	} else
#endif
		// Master mode, check BERR bit
	{
		if (RCW_OF(I2C_STS_REG).BERR == 1) {
			if (I2CConfig[I2CActiveSes].bHandleNACK == TRUE) {
				i2c_handleNACK();
			}
			return I2C_STS_NACK;
		}
	}

	// Data transmitted
#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Only slave mode will issue STOP interrupt
	if (RCW_OF(I2C_STS_REG).STOP == 1) {
		return (I2C_STS_OK | I2C_STS_STOP);
	}
#endif

	return I2C_STS_OK;
}

/**
    Receive I2C5 data in PIO mode

    Receive I2C5 data in PIO mode for I2C_SESx that are locked.

    @param[in] pData    PIO data to received, including response NACK condition

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Byte count is not 1 ~ 8 or I2C5 is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is received, could be OR with I2C_STS_STOP.
        - @b I2C_STS_STOP               : Slave mode only, receive STOP condition from master.
*/
I2C_STS i2c5_receive(PI2C_DATA pData)
{
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CTRL_REG);
	UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;

	// Check parameter
	if ((pData->ByteCount > I2C_BYTE_CNT_8) ||
		(pData->ByteCount < I2C_BYTE_CNT_1)) {
		DBG_ERR("Invalid byte count!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Wait for bus free
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((((bI2CCheckBusFree == TRUE) &&
			   (RCW_OF(I2C_STS_REG).BusFree == 0)) ||   // STOP is issued and not available to issue START
			  (RCW_OF(I2C_STS_REG).BusBusy == 1)) &&    // Bus is busy (START to STOP) and controller is not involved in
			 i);

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Don't check bus free for next operation
	bI2CCheckBusFree = FALSE;

	// Store the pointer point to I2C_BYTE array
	pI2CByte            = pData->pByte;
	// Store current transmit byte count
	uiI2CCurrentBytes   = (pData->ByteCount > I2C_BYTE_CNT_4) ? (pData->ByteCount - I2C_BYTE_CNT_4) : pData->ByteCount;
	// Store not transmitted byte count.
	// (FW support up to 8 bytes, Controller support up to 4 bytes, we can transmit all remaining data in ISR)
	uiI2CRemainingBytes = pData->ByteCount - uiI2CCurrentBytes;

	i2c_setRxConfig();

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).TB_En = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	// Wait for data received
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
		DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Data received
	i2c_getRxData();

#if (MODULE_SUPPORT_SLAVE == ENABLE)
	// Only slave mode will issue STOP interrupt
	// If the controller detect STOP in slave mode before pData->ByteCount are received,
	// the controller will only issue STOP interrupt.
	if (RCW_OF(I2C_STS_REG).STOP == 1) {
#ifdef _NVT_EMULATION_
		if (RCW_OF(I2C_STS_REG).DR == 0)
#else
		// In slave receive mode, master won't send STOP unless slave
		// write TB_EN = 1 (SCL will hold low before TB_EN = 1),
		// that means slave must receive one extra byte to get STOP
		// interrupt.
		if ((RCW_OF(I2C_STS_REG).DR == 0) && (pData->ByteCount != I2C_BYTE_CNT_1))
#endif
		{
			DBG_DUMP("%s: Detect STOP before %d bytes received!\r\n", __func__, (INT)pData->ByteCount);
		}
		return (I2C_STS_OK | I2C_STS_STOP);
	}
#endif

	return I2C_STS_OK;
}

/**
    Get current I2C R/W bit status

    Get current I2C R/W bit status.

    @return R/W bit status
        - @b I2C_RWBIT_WRITE    : Write
        - @b I2C_RWBIT_READ     : Read
*/
I2C_RWBIT i2c5_getRWBit(void)
{
	RCW_DEF(I2C_STS_REG);

	RCW_LD(I2C_STS_REG);

	return (I2C_RWBIT)(RCW_OF(I2C_STS_REG).RW);
}

#if (MODULE_SUPPORT_DMA == ENABLE)
/**
    Transmit I2C5 data in DMA mode

    Transmit I2C5 data in DMA mode for I2C_SESx that are locked. Please note that DMA mode
    can't send START or STOP condition.

    @param[in] uiAddr   DMA buffer address for transmitted data (Unit: byte, must be word alignment)
    @param[in] uiSize   DMA buffer size for transmitted data (Unit: byte, must be word alignment).
                        Valid value: 0x4 ~ 0xFFFC

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Address or size is not word alignment, size over limitation or I2C5 is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_NACK               : Receive NACK
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is transmitted
*/
I2C_STS i2c5_transmitDMA(UINT32 uiAddr, UINT32 uiSize)
{
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CONFIG_REG);
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_DMASIZE_REG);
	RCW_DEF(I2C_DMAADDR_REG);
	UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;

	// Check parameter
	if ((uiAddr & 0x3) ||
		//(uiSize & 0x3) ||
		(uiSize > I2C_DMA_SIZE_MAX)) {
		DBG_ERR("Invalid parameter!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Flush cache
//	dma_flushWriteCache(uiAddr, uiSize);
	vos_cpu_dcache_sync((VOS_ADDR)uiAddr, uiSize, VOS_DMA_TO_DEVICE_NON_ALIGN);

	// Wait for bus not busy
	// (Don't have to wait for RCW_OF(I2C_STS_REG).BusFree, since DMA mode can't send START or STOP condition)
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).BusBusy == 1) && i);  // Bus is busy (START to STOP) and controller is not involved in

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Configure I2C to DMA mode
	RCW_VAL(I2C_CONFIG_REG) = 0;
	RCW_OF(I2C_CONFIG_REG).AccessMode = I2C_ACCESS_DMA;
	RCW_ST(I2C_CONFIG_REG);

	// Configure DMA buffer address and size
	RCW_OF(I2C_DMAADDR_REG).DMA_StartAddr   = dma_getPhyAddr(uiAddr);
	RCW_ST(I2C_DMAADDR_REG);
	RCW_OF(I2C_DMASIZE_REG).DMA_BufSize     = uiSize; // desc mode not need to set this register, but need to dma_flushWriteCache
	RCW_ST(I2C_DMASIZE_REG);

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable DMA transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).DMA_Dir    = I2C_DMA_DIR_WRITE;
	RCW_OF(I2C_CTRL_REG).DMA_En     = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	// Wait for data transmitted
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms))) {
		DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// NACK
	if (RCW_OF(I2C_STS_REG).BERR == 1) {
		if (I2CConfig[I2CActiveSes].bHandleNACK == TRUE) {
			i2c_handleNACK();
		}
		return I2C_STS_NACK;
	}

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Desc poll timeout
	if (RCW_OF(I2C_STS_REG).DESCERR == 1) {
		DBG_ERR("polling timeout!\r\n");
		return I2C_STS_DESCERR;
	}

	// Data transmitted
#ifdef _NVT_EMULATION_
	// This should not happen, but we still check this in emulation environment.
	// Don't check this in release environment to reduce code size.
	if (RCW_OF(I2C_STS_REG).DMAED == 0) {
		DBG_ERR("Invalid interrupt status 0x%.8X\r\n", (int)RCW_VAL(I2C_STS_REG));
		while (1) {
			;
		}
	}
#endif

	return I2C_STS_OK;
}

/**
    Transmit I2C data in DMA mode

    Transmit I2C data in DMA mode for I2C_SESx that are locked. Please note that DMA mode
    can't send START or STOP condition.

    @param[in] uiAddr   DMA buffer address for transmitted data (Unit: byte, must be word alignment)
    @param[in] uiSize   DMA buffer size for transmitted data (Unit: byte, must be word alignment).
                        Valid value: 0x4 ~ 0xFFFC

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Address or size is not word alignment, size over limitation or I2C is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_NACK               : Receive NACK
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is transmitted
*/
I2C_STS i2c5_transmitDMA_En(UINT32 uiAddr, UINT32 uiSize)
{
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CONFIG_REG);
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_DMASIZE_REG);
	RCW_DEF(I2C_DMAADDR_REG);
	UINT32      i;
//    FLGPTN      FlagPtn;

	// Check parameter
	if ((uiAddr & 0x3) ||
		(uiSize & 0x3) ||
		(uiSize > I2C_DMA_SIZE_MAX)) {
		DBG_ERR("Invalid parameter!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Flush cache
//	dma_flushWriteCache(uiAddr, uiSize);
	vos_cpu_dcache_sync((VOS_ADDR)uiAddr, uiSize, VOS_DMA_TO_DEVICE_NON_ALIGN);

	// Wait for bus not busy
	// (Don't have to wait for RCW_OF(I2C_STS_REG).BusFree, since DMA mode can't send START or STOP condition)
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).BusBusy == 1) && i);  // Bus is busy (START to STOP) and controller is not involved in

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Configure I2C to DMA mode
	RCW_VAL(I2C_CONFIG_REG) = 0;
	RCW_OF(I2C_CONFIG_REG).AccessMode = I2C_ACCESS_DMA;
	RCW_ST(I2C_CONFIG_REG);

	// Configure DMA buffer address and size
	RCW_OF(I2C_DMAADDR_REG).DMA_StartAddr   = dma_getPhyAddr(uiAddr);
	RCW_ST(I2C_DMAADDR_REG);
	RCW_OF(I2C_DMASIZE_REG).DMA_BufSize     = uiSize;
	RCW_ST(I2C_DMASIZE_REG);

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable DMA transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).DMA_Dir    = I2C_DMA_DIR_WRITE;
	RCW_OF(I2C_CTRL_REG).DMA_En     = ENABLE;
	RCW_ST(I2C_CTRL_REG);
#if 0
	// Wait for data transmitted
	wai_flg(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW);

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// NACK
	if (RCW_OF(I2C_STS_REG).BERR == 1) {
		if (I2CConfig[I2CActiveSes].bHandleNACK == TRUE) {
			i2c_handleNACK();
		}
		return I2C_STS_NACK;
	}

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Data transmitted
#ifdef _NVT_EMULATION_
	// This should not happen, but we still check this in emulation environment.
	// Don't check this in release environment to reduce code size.
	if (RCW_OF(I2C_STS_REG).DMAED == 0) {
		DBG_ERR("Invalid interrupt status 0x%.8X\r\n", RCW_VAL(I2C_STS_REG));
		while (1) {
			;
		}
	}
#endif
#endif
	return I2C_STS_OK;
}

/**
    Transmit I2C data in DMA mode

    Transmit I2C data in DMA mode for I2C_SESx that are locked. Please note that DMA mode
    can't send START or STOP condition.

    @param[in] uiAddr   DMA buffer address for transmitted data (Unit: byte, must be word alignment)
    @param[in] uiSize   DMA buffer size for transmitted data (Unit: byte, must be word alignment).
                        Valid value: 0x4 ~ 0xFFFC

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Address or size is not word alignment, size over limitation or I2C is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_NACK               : Receive NACK
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is transmitted
*/
I2C_STS i2c5_transmitDMA_Wait(UINT32 uiAddr, UINT32 uiSize)
{
	RCW_DEF(I2C_STS_REG);
//    RCW_DEF(I2C_CONFIG_REG);
//    RCW_DEF(I2C_CTRL_REG);
//    RCW_DEF(I2C_DMASIZE_REG);
//    RCW_DEF(I2C_DMAADDR_REG);
//    UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;
#if 0
	// Check parameter
	if ((uiAddr & 0x3) ||
		(uiSize & 0x3) ||
		(uiSize > I2C_DMA_SIZE_MAX)) {
		DBG_ERR("Invalid parameter!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Flush cache
	dma_flushWriteCache(uiAddr, uiSize);

	// Wait for bus not busy
	// (Don't have to wait for RCW_OF(I2C_STS_REG).BusFree, since DMA mode can't send START or STOP condition)
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).BusBusy == 1) && i);  // Bus is busy (START to STOP) and controller is not involved in

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Configure I2C to DMA mode
	RCW_VAL(I2C_CONFIG_REG) = 0;
	RCW_OF(I2C_CONFIG_REG).AccessMode = I2C_ACCESS_DMA;
	RCW_ST(I2C_CONFIG_REG);

	// Configure DMA buffer address and size
	RCW_OF(I2C_DMAADDR_REG).DMA_StartAddr   = dma_getPhyAddr(uiAddr);
	RCW_ST(I2C_DMAADDR_REG);
	RCW_OF(I2C_DMASIZE_REG).DMA_BufSize     = uiSize;
	RCW_ST(I2C_DMASIZE_REG);

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable DMA transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).DMA_Dir    = I2C_DMA_DIR_WRITE;
	RCW_OF(I2C_CTRL_REG).DMA_En     = ENABLE;
	RCW_ST(I2C_CTRL_REG);
#endif

	// Wait for data transmitted
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
		DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// NACK
	if (RCW_OF(I2C_STS_REG).BERR == 1) {
		if (I2CConfig[I2CActiveSes].bHandleNACK == TRUE) {
			i2c_handleNACK();
		}
		return I2C_STS_NACK;
	}

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Data transmitted
#ifdef _NVT_EMULATION_
	// This should not happen, but we still check this in emulation environment.
	// Don't check this in release environment to reduce code size.
	if (RCW_OF(I2C_STS_REG).DMAED == 0) {
		DBG_ERR("Invalid interrupt status 0x%.8X\r\n", (int)RCW_VAL(I2C_STS_REG));
		while (1) {
			;
		}
	}
#endif

	return I2C_STS_OK;
}

/**
    Receive I2C5 data in DMA mode

    Receive I2C5 data in DMA mode for I2C_SESx that are locked. Please note that DMA mode
    can't send STOP condition.

    @param[in] uiAddr   DMA buffer address for received data (Unit: byte, must be word alignment)
    @param[in] uiSize   DMA buffer size for received data (Unit: byte, must be word alignment)
                        Valid value: 0x4 ~ 0xFFFC

    @return Transmit status, one of the following:
        - @b I2C_STS_INVALID_PARAM      : Address or size is not word alignment, size over limitation or I2C5 is not locked
        - @b I2C_STS_BUS_NOT_AVAILABLE  : Bus is busy
        - @b I2C_STS_AL                 : Arbitration lost
        - @b I2C_STS_OK                 : Data is received
*/
I2C_STS i2c5_receiveDMA(UINT32 uiAddr, UINT32 uiSize)
{
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CONFIG_REG);
	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_DMASIZE_REG);
	RCW_DEF(I2C_DMAADDR_REG);
	UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;

	// Check parameter
	if ((uiAddr & 0x3) ||
		(uiSize & 0x3) ||
		(uiSize > I2C_DMA_SIZE_MAX)) {
		DBG_ERR("Invalid parameter!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Flush cache
//	dma_flushReadCache(uiAddr, uiSize);
	vos_cpu_dcache_sync((VOS_ADDR)uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL_NON_ALIGN);

	// Wait for bus not busy
	// (Don't have to wait for RCW_OF(I2C_STS_REG).BusFree, since DMA mode can't send START or STOP condition)
	i = I2C_POLLING_TIMEOUT;
	do {
		i--;
		RCW_LD(I2C_STS_REG);
	} while ((RCW_OF(I2C_STS_REG).BusBusy == 1) && i);  // Bus is busy (START to STOP) and controller is not involved in

	if (i == 0) {
		DBG_ERR("Wait for bus free timeout!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Configure I2C to DMA mode
	RCW_VAL(I2C_CONFIG_REG) = 0;
	RCW_OF(I2C_CONFIG_REG).AccessMode = I2C_ACCESS_DMA;
	RCW_ST(I2C_CONFIG_REG);

	// Configure DMA buffer address and size
	RCW_OF(I2C_DMAADDR_REG).DMA_StartAddr   = dma_getPhyAddr(uiAddr);
	RCW_ST(I2C_DMAADDR_REG);
	RCW_OF(I2C_DMASIZE_REG).DMA_BufSize     = uiSize;
	RCW_ST(I2C_DMASIZE_REG);

	// Clear flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable DMA transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).DMA_Dir    = I2C_DMA_DIR_READ;
	RCW_OF(I2C_CTRL_REG).DMA_En     = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	// Wait for data transmitted
	if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
		DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	vos_cpu_dcache_sync((VOS_ADDR)uiAddr, uiSize, VOS_DMA_FROM_DEVICE_NON_ALIGN);

	loc_cpu(flags);

	// Get interrupt status
	RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

	// Clear interrupt status
	uiI2CIntSts = 0;

	unl_cpu(flags);

	// Arbitration lost
	if (RCW_OF(I2C_STS_REG).AL == 1) {
		return i2c_handleAL(__func__);
	}

	// Bus not available
	if (RCW_OF(I2C_STS_REG).SCLTimeout == 1) {
		DBG_ERR("Clock pin is LOW too long!\r\n");
		return I2C_STS_BUS_NOT_AVAILABLE;
	}

	// Data received
#ifdef _NVT_EMULATION_
	// This should not happen, but we still check this in emulation environment.
	// Don't check this in release environment to reduce code size.
	if (RCW_OF(I2C_STS_REG).DMAED == 0) {
		DBG_ERR("Invalid interrupt status 0x%.8X\r\n", (int)RCW_VAL(I2C_STS_REG));
		while (1) {
			;
		}
	}
#endif

	return I2C_STS_OK;
}
#endif

#if (MODULE_SUPPORT_SLAVE == ENABLE)

/**
    Wait for slave address matched

    Wait for slave address matched.

    @param[in] SARLen       Slave address length

    @return Slave address matched or general call
        - @b I2C_STS_SAM    : Slave address matched
        - @b I2C_STS_GC     : General call
*/
I2C_STS i2c5_waitSARMatch(I2C_SAR_LEN SARLen)
{
	UINT32      i;
	FLGPTN      FlagPtn;
	unsigned long      flags;

	RCW_DEF(I2C_CTRL_REG);
	RCW_DEF(I2C_STS_REG);
	RCW_DEF(I2C_CONFIG_REG);
#ifdef _NVT_EMULATION_
	RCW_DEF(I2C_DATA_REG);
#endif

	// Set parameter
	RCW_VAL(I2C_CONFIG_REG)             = 0;

	// Access mode
	RCW_OF(I2C_CONFIG_REG).AccessMode   = I2C_ACCESS_PIO;

	// PIO data size
	RCW_OF(I2C_CONFIG_REG).PIODataSize  = SARLen;

	RCW_ST(I2C_CONFIG_REG);

	while (1) {
		// Wait for bus not busy
		i = I2C_POLLING_TIMEOUT;
		do {
			i--;
			RCW_LD(I2C_STS_REG);
		} while ((RCW_OF(I2C_STS_REG).BusBusy == 1) && i);  // STOP is issued and not available to issue START

		// Clear flag
		clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

		// Enable transfer
		RCW_LD(I2C_CTRL_REG);
		RCW_OF(I2C_CTRL_REG).TB_En = ENABLE;
		RCW_ST(I2C_CTRL_REG);

		// Wait for SAR matched
		if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
			DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
			return I2C_STS_BUS_NOT_AVAILABLE;
		}

		loc_cpu(flags);

		// Get interrupt status
		RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

		// Clear interrupt status
		uiI2CIntSts = 0;

		unl_cpu(flags);

		// General call
		// When GC is issued, SAM will be issued too. We have to check GC before SAM.
		if (RCW_OF(I2C_STS_REG).GC == 1) {
			// Wait for DR
			while (RCW_OF(I2C_STS_REG).DR == 0) {
				if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
					DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
					return I2C_STS_BUS_NOT_AVAILABLE;
				}

				loc_cpu(flags);

				// Get interrupt status
				RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

				// Clear interrupt status
				uiI2CIntSts = 0;

				unl_cpu(flags);
			}

			set_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

#ifdef _NVT_EMULATION_			// Read data from data register
			RCW_LD(I2C_DATA_REG);

			// Check received slave address (data register)
			// Data register LSB must be 0
			if (RCW_VAL(I2C_DATA_REG) & 0xFF) {
				DBG_ERR("Received SAR is not GC(0)!\r\n");
			}
#endif
			return I2C_STS_GC;
		}

		// Slave address matched
		if (RCW_OF(I2C_STS_REG).SAM == 1) {
			// Wait for DR
			while (RCW_OF(I2C_STS_REG).DR == 0) {
				if (vos_flag_wait_timeout(&FlagPtn, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(I2CConfig[I2CActiveSes].timeout_ms)) != E_OK) {
					DBG_ERR("Session %d timeout %d ms\r\n", I2CActiveSes, I2CConfig[I2CActiveSes].timeout_ms);
					return I2C_STS_BUS_NOT_AVAILABLE;
				}

				loc_cpu(flags);

				// Get interrupt status
				RCW_VAL(I2C_STS_REG) = uiI2CIntSts;

				// Clear interrupt status
				uiI2CIntSts = 0;

				unl_cpu(flags);
			}

			set_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

#ifdef _NVT_EMULATION_
			// Read data from data register
			RCW_LD(I2C_DATA_REG);

			// Check data (SAR) recevied and SAR set by upper layer
			if (SARLen == I2C_SAR_LEN_1BYTE) {
				// bit 0 is R/W bit of I2C protocol
				RCW_VAL(I2C_DATA_REG) &= 0xFE;

				if (I2CConfig[I2CActiveSes].SARMode == I2C_SAR_MODE_7BITS) {
					if (RCW_VAL(I2C_DATA_REG) != i2c_conv7bSARToData(I2CConfig[I2CActiveSes].uiSAR & I2C_SAR_7BIT_MASK)) {
						RCW_LD(I2C_DATA_REG);
						DBG_ERR("SAR[1byte] is not matched! Data: 0x%X EXPT : 0x%X\r\n", (int)RCW_VAL(I2C_DATA_REG), (int)i2c_conv7bSARToData(I2CConfig[I2CActiveSes].uiSAR & I2C_SAR_7BIT_MASK));
					}
				} else {
					if (RCW_VAL(I2C_DATA_REG) != i2c_conv10bSARTo1stData(I2CConfig[I2CActiveSes].uiSAR)) {
						RCW_LD(I2C_DATA_REG);
						DBG_ERR("SAR[1byte, 10-bits] is not matched! Data: 0x%X EXPT : 0x%X\r\n", (int)RCW_VAL(I2C_DATA_REG), (int)i2c_conv10bSARTo1stData(I2CConfig[I2CActiveSes].uiSAR));
					}
				}
			} else {
				// bit 0 is R/W bit of I2C protocol
				RCW_VAL(I2C_DATA_REG) &= 0xFFFE;

				if (RCW_VAL(I2C_DATA_REG) != (i2c_conv10bSARTo1stData(I2CConfig[I2CActiveSes].uiSAR) | (i2c_conv10bSARTo2ndData(I2CConfig[I2CActiveSes].uiSAR) << 8))) {
					RCW_LD(I2C_DATA_REG);
					DBG_ERR("SAR[2bytes] is not matched! Data: 0x%.8X\r\n", (int)RCW_VAL(I2C_DATA_REG));
				}
			}
#endif
			return I2C_STS_SAM;
		}
	}
}

#endif

I2C_STS i2c5_clearBus_En(void)
{
	RCW_DEF(I2C_CTRL_REG);

    // Check if i2c locked
	if (uiI2CSesLocked == 0) {
		DBG_ERR("I2C isn't locked!\r\n");
		return I2C_STS_INVALID_PARAM;
	}

	// Enable transfer
	RCW_LD(I2C_CTRL_REG);
	RCW_OF(I2C_CTRL_REG).BC_En = ENABLE;
	RCW_ST(I2C_CTRL_REG);

	do {
		RCW_LD(I2C_CTRL_REG);
	} while ( (RCW_OF(I2C_CTRL_REG).BC_En == 1) );
	return I2C_STS_OK;
}

void i2c5_autoRst_En(I2C_SESSION *i2c_session)
{
	int i, reg_val[reg_sz] = {0};

	/* read ori register, for I2C_RSTN(DO_RESET) */
	for (i = 0; i < reg_sz; i++) {
		reg_val[i] = (int)EMU_I2C_READ_ENG_REG(MODULE_REG_BASE + (i << 2));
	}

	pll_enable_system_reset(I2C5_RSTN);
	pll_disable_system_reset(I2C5_RSTN);

	i2c5_close(*i2c_session);
	i2c5_open(i2c_session);

	/* write ori register, for I2C_RSTN(DO_RESET) */
	for (i = 0; i < reg_sz; i++) {
		EMU_I2C_WRITE_ENG_REG(MODULE_REG_BASE + (i << 2), reg_val[i]);
	}
}

//@}

// Emulation only code
#if 1//#ifdef _NVT_EMULATION_

BOOL    i2c5Test_compareRegDefaultValue(void);
void    i2c5Test_getClockPeriod(I2C_SESSION Session, double *dHigh, double *dLow);

static I2C_REG_DEFAULT  I2CRegDefault[] = {
	{ 0x00,     I2C_CTRL_REG_DEFAULT,       "Control"       },
	{ 0x04,     I2C_STS_REG_DEFAULT,        "Status"        },
	{ 0x08,     I2C_CONFIG_REG_DEFAULT,     "Config"        },
	{ 0x0C,     I2C_BUSCLK_REG_DEFAULT,     "BusClock"      },
	{ 0x10,     I2C_DATA_REG_DEFAULT,       "Data"          },
	{ 0x14,     I2C_SAR_REG_DEFAULT,        "SlaveAddress"  },
	{ 0x18,     I2C_TIMING_REG_DEFAULT,     "Timing"        },
	{ 0x1C,     I2C_BUS_REG_DEFAULT,        "BusMonitor"    },
	{ 0x20,     I2C_DMASIZE_REG_DEFAULT,    "DMASize"       },
	{ 0x24,     I2C_DMAADDR_REG_DEFAULT,    "DMAAddress"    },
	{ 0x28,     I2C_BUSFREE_REG_DEFAULT,    "BusFree"       },
	{ 0x2C,     I2C_VD_SIZE_REG_DEFAULT,    "GroupSize"     },
	{ 0x34,     I2C_VD_CONTROL_REG_DEFAULT, "VDContorl"     },
	{ 0x38,     I2C_VD_DELAY_REG_DEFAULT,   "VDDelay"       },
	{ 0x3C,     I2C_DESC_INFO_DEFAULT,      "DescInfo"      }
};

/*
    (Verification code) Compare I2C5 register default value

    (Verification code) Compare I2C5 register default value.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL i2c5Test_compareRegDefaultValue(void)
{
	UINT32  uiIndex;
	BOOL    bReturn = TRUE;

	for (uiIndex = 0; uiIndex < (sizeof(I2CRegDefault) / sizeof(I2C_REG_DEFAULT)); uiIndex++) {
		if (INW(_REGIOBASE + I2CRegDefault[uiIndex].uiOffset) != I2CRegDefault[uiIndex].uiValue) {
			DBG_ERR("%s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
					I2CRegDefault[uiIndex].pName,
					(INT)I2CRegDefault[uiIndex].uiOffset,
					(INT)INW(_REGIOBASE + I2CRegDefault[uiIndex].uiOffset),
					(INT)I2CRegDefault[uiIndex].uiValue);
			bReturn = FALSE;
		}
	}

	return bReturn;
}

/*
    (Verification code) Get I2C5 bus clock high / low period

    (Verification code) Get I2C5 bus clock high / low period.

    @param[in]  Session : Which session
    @param[out] dHigh   : High period in ns
    @param[out] dLow    : Low period in ns
    @return void
*/
void i2c5Test_getClockPeriod(I2C_SESSION Session, double *dHigh, double *dLow)
{
	double oneBaseClock;

	oneBaseClock = (double)(1000000000) / I2C_SOURCE_CLOCK;
	*dHigh  = oneBaseClock * (I2CConfig[Session].uiClkHighCnt + 3 + I2CConfig[Session].uiGSR);
	*dLow   = oneBaseClock * (I2CConfig[Session].uiClkLowCnt + 1);
}

/*
    (Verification code) Get I2C5 bus VD Delay

    (Verification code) Get I2C5 bus VD Delay

    @param[in]  Session : Which session
    @param[in]  uiDealy : VD Delay period in cycle count
    @return void
*/
void i2c5Test_setVDDelay(I2C_SESSION Session, UINT32 uiDelay)
{
	// Configuration of this session is modified
	uiI2CSesModified |= (I2C_SES_MODIFIED << Session);

	I2CConfig[Session].uiVDDelay = uiDelay;
}

#endif
