/*
    MIPI-CSI module driver

    @file       csi.c
    @ingroup    mIDrvIO_CSI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifdef __KERNEL__
#include <kwrap/util.h>
#include <kwrap/spinlock.h>
//#include <linux/clk.h>
#include "csi_dbg.h"
#include "../csi.h"
#include "csi_int.h"
#else
#if defined(__FREERTOS)
#define __MODULE__    rtos_csi
#include <kwrap/debug.h>
#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
unsigned int rtos_csi_debug_level = NVT_DBG_WRN;
#include "../csi.h"
#include "include/csi_int.h"
#else
#include "DrvCommon.h"
#include "csi.h"
#include "Utility.h"
#include "csi_int.h"
#endif
#endif

#define DEBUG_MSG 0
#define MODULE_ID  CSI_ID_CSI

#ifdef __KERNEL__
//#define TODO

UINT32 _CSI_REG_BASE_ADDR;
UINT32 _SIE_CSI_REG_BASE_ADDR;


//#define Delay_DelayMs(ms) mdelay(ms)
//#define Delay_DelayUs(us) ndelay(1000*us)

//static struct completion csi_completion;
static SEM_HANDLE SEMID_CSI;
static FLGPTN     FLG_ID_CSI;
#define FLGPTN_CSI				FLGPTN_BIT(0)


//#define drv_enableInt(x)
//#define drv_disableInt(x)
//#define pll_disableClock(x)
//#define pll_enableClock(x)
//#define pll_setClockRate(x, y)
//#define pll_isClockEnabled(x) 1
//#define pll_getClockRate(x) 0

//#define MIPI_LVDS_CLK 0
//#define PLL_CLKSEL_MIPI_LVDS_60 0
//#define PLL_CLKSEL_MIPI_LVDS_120 1
#define MODULE_CLK_ID				0
#else
#if defined(__FREERTOS)
static ID SEMID_CSI;
static ID FLG_ID_CSI;
#define MODULE_CLK_ID				MIPI_LVDS_CLK
#define MODULE_DRV_INT				INT_ID_LVDS
#else
#define MODULE_CLK_ID				MIPI_LVDS_CLK
#endif
#endif

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

/**
    @addtogroup mIDrvIO_CSI
*/
//@{
#ifndef __KERNEL
#define CSI_TIMEOUTCALLBACK         csi_timeout_timer_cb
#endif
#define MODULE_FLAG_ID              FLG_ID_CSI
#define MODULE_FLAG_PTN             FLGPTN_CSI
#define MODULE_SEM_ID               SEMID_CSI

static BOOL     csi_opened = FALSE;
static UINT32   csi_int_sts;
static UINT32   csi_err_monitor[1] = {0};
static UINT32   csi_err_cnt[1]     = {0};
static UINT32   csi_err_sts[2]    = {0, 0};
static UINT32   csi_input_swap = 0x32103210;
static UINT32   csi_4_lanes_input_swap = 0x32103210;
static BOOL     csi_clk_switch     = FALSE;
#ifndef __KERNEL__
static SWTIMER_ID       csi_timeout_timer_id;
#endif
static UINT32           csi_timeout_period;
static BOOL     csi_force_dis;
static UINT32   csi_sensor_target_mclk;
static UINT32   csi_sensor_real_mclk;
static UINT32   ecc_cnt = 0;
static UINT32   fast_boot = 0;
static UINT32   fs_cnt = 0;
static UINT8    phy_hso_delay = 0;



//static void csi_resetPhysicalLayer(BOOL bRst);
static void csi_set_int_disable(BOOL bint1, CSI_INTERRUPT interr_dis);


void csi_get_escape_command(UINT32 *pcmd0, UINT32 *pcmd1, UINT32 *pcmd2, UINT32 *pcmd3);
#ifndef __KERNEL__
#if defined(__FREERTOS)
irqreturn_t csi_isr(int irq, void *devid);
#else
void csi_isr(void);
#endif
#endif

#ifndef __KERNEL__//#if 0//ndef __KERNEL__
static void csi_timeout_timer_cb(UINT32 event)
{
	unsigned long	   flags;

	loc_cpu(flags);
	csi_int_sts |= CSI_INTERRUPT_ABORT;
	unl_cpu(flags);

	set_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);
}
#endif

#ifdef __KERNEL__
UINT32 csi_isr_check(void)
{
	T_CSI_INTSTS0_REG  reg_sts0;
	T_CSI_INTSTS1_REG  reg_sts1;

	reg_sts0.reg  = CSI_GETREG(CSI_INTSTS0_REG_OFS);
	reg_sts1.reg  = CSI_GETREG(CSI_INTSTS1_REG_OFS);
	reg_sts0.reg &= CSI_GETREG(CSI_INTEN0_REG_OFS);
	reg_sts1.reg &= CSI_GETREG(CSI_INTEN1_REG_OFS);

	if ((reg_sts0.reg) || (reg_sts1.reg)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
#endif

#ifndef __KERNEL__
#if defined(__FREERTOS)
irqreturn_t csi_isr(int irq, void *devid)
#else
void csi_isr(void)
#endif
#else
void csi_isr(void)
#endif
{
	T_CSI_INTSTS0_REG  reg_sts0;
	T_CSI_INTSTS1_REG  reg_sts1;
	//T_CSI_CTRL0_REG reg_ctrl0;
	//T_CSI_PHYCTRL1_REG reg_phy1;
	//BOOL clk_is_forcehs;

	// Get the interrupt status which the enable bits are enabled.
	reg_sts0.reg  = CSI_GETREG(CSI_INTSTS0_REG_OFS);
	reg_sts1.reg  = CSI_GETREG(CSI_INTSTS1_REG_OFS);
	reg_sts0.reg &= CSI_GETREG(CSI_INTEN0_REG_OFS);
	reg_sts1.reg &= CSI_GETREG(CSI_INTEN1_REG_OFS);

	// Check if this ISR event is triggered by the CSI module because of the share interrupt design.
	if ((reg_sts0.reg) || (reg_sts1.reg)) {
		// Clear the interrupt status and this status event would be handled later.
		CSI_SETREG(CSI_INTSTS0_REG_OFS, reg_sts0.reg);
		CSI_SETREG(CSI_INTSTS1_REG_OFS, reg_sts1.reg);

		// Save the status bits to global variable and this is used in the wait_interrupt API.
		csi_int_sts |= (reg_sts0.reg & CSI_INTERRUP_ALL & (~(CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4)));
		csi_int_sts &= ~CSI_DEFAULT_INT0_FS;
		csi_int_sts |= (reg_sts1.reg & (CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4));

#if _FPGA_EMULATION_
		/* FPGA Usage */
		if ((reg_sts0.bit.SOT_SYNC_ERR0)
			|| (reg_sts0.bit.SOT_SYNC_ERR1) || (reg_sts0.bit.SOT_SYNC_ERR2)
			|| (reg_sts0.bit.SOT_SYNC_ERR3) || (reg_sts0.bit.STATE_ERR0)
			|| (reg_sts0.bit.STATE_ERR1) || (reg_sts0.bit.STATE_ERR2) || (reg_sts0.bit.STATE_ERR3) || (reg_sts0.bit.STATE_ERRC)) {
			csi_err_monitor[0]++;
			csi_err_cnt[0]++;
			if (csi_err_monitor[0] >= 10000) {
				csi_err_monitor[0] = 0x0;
				DBG_ERR("CSI ERROR!\r\n");
			}

			//#if _FPGA_EMULATION_
			//csi_resetPhysicalLayer(TRUE);
			//Delay_DelayUsPolling(10);
			//csi_resetPhysicalLayer(FALSE);
			//#endif
		}
#else
		/* Real Chip Case */

		// These are all of the Error Events that should not be appeared in normal operations.
		// In current driver design, we show the error warning msg only if the 10000 error event are triggered.
		// This is because in the normal operation, such as sensor power-on or mode-change.
		// The sensor output LowPower pin may not follow the normal CSI rules.
		// This may make the user think that the error events occured if we show these msg.
		// So we only show the warning msg for the errors which happens continuously.
#if 1
		//patch for disable at last line issue - begin
		if (reg_sts0.bit.ECC_ERR) {
			ecc_cnt ++;
			if ((ecc_cnt % 100) == 10) {

				if (ecc_cnt >= 5000) {
					csi_set_int_disable(CSI_INT_BANK0, 0x1);
				}
#if 0
				reg_ctrl0.reg = CSI_GETREG(CSI_CTRL0_REG_OFS);
				if (reg_ctrl0.bit.EN_SEL) {
					reg_ctrl0.bit.CSI_EN = 0;
					CSI_SETREG(CSI_CTRL0_REG_OFS, reg_ctrl0.reg);
					//csi_set_enable(DISABLE);

					/*do {
						reg_ctrl0.reg = CSI_GETREG(CSI_CTRL0_REG_OFS);
					} while(reg_ctrl0.bit.CSI_EN);*/

					reg_phy1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
					clk_is_forcehs = reg_phy1.bit.CLK_FORCE_HS;
					if (clk_is_forcehs == 0) {
						reg_phy1.bit.CLK_FORCE_HS = 1;
						CSI_SETREG(CSI_PHYCTRL1_REG_OFS, reg_phy1.reg);
					}

					//senphy_enable(SENPHY_SEL_MIPILVDS, DISABLE);
					//senphy_enable(SENPHY_SEL_MIPILVDS, ENABLE);

					reg_ctrl0.bit.CSI_EN = 1;
					CSI_SETREG(CSI_CTRL0_REG_OFS, reg_ctrl0.reg);
					//csi_set_enable(TRUE);

					/*do {
						reg_ctrl0.reg = CSI_GETREG(CSI_CTRL0_REG_OFS);
					} while(!reg_ctrl0.bit.CSI_EN);*/

					if (clk_is_forcehs == 0) {
						reg_phy1.bit.CLK_FORCE_HS = 0;
						CSI_SETREG(CSI_PHYCTRL1_REG_OFS, reg_phy1.reg);
					}
					DBG_DUMP("ecc_cnt 1.\r\n");

					CSI_SETREG(CSI_INTSTS0_REG_OFS, 0x1);

					/*if (ecc_cnt >= 1000) {
						//DBG_DUMP("ecc_cnt 2.\r\n");
						csi_err_sts[0] |= 1;
					}*/
				}
#else
					//DBG_DUMP("ecc_cnt 0.\r\n");
#endif
			}
		}
		//patch for disable at last line issue - end
#endif

		if ((reg_sts0.reg & CSI_DEFAULT_INT0) || (reg_sts1.reg & CSI_DEFAULT_INT1)) {
			csi_err_monitor[0]++;
			csi_err_cnt[0]++;
			if (csi_err_monitor[0] >= 10000) {
				csi_err_monitor[0] = 0x0;
				//DBG_ERR("CSI ERROR! Sts0=0x%08X  Sts1=0x%08X\r\n", csi_err_sts[0], csi_err_sts[1]);
				csi_error_parser(MODULE_ID, csi_err_sts[0], csi_err_sts[1], CSI_GETREG(CSI_LANESTATE1_REG_OFS));

				csi_err_sts[0] = 0;
				csi_err_sts[1] = 0;
			} else {
				csi_err_sts[0] |= (reg_sts0.reg & CSI_DEFAULT_INT0);
				csi_err_sts[1] |= (reg_sts1.reg & CSI_DEFAULT_INT1);

				reg_sts0.reg &= ~CSI_DEFAULT_INT0;
				reg_sts1.reg &= ~CSI_DEFAULT_INT1;
			}
		}

		if ((reg_sts0.reg & CSI_DEFAULT_INT0_FS)) {
			fs_cnt++;
			//DBG_ERR("FS = %d\r\n", fs_cnt);
#ifdef __KERNEL__
			csi_platform_complete(MODULE_ID);
			return;
#else
			iset_flg(FLG_ID_CSI, FLGPTN_CSI);
			reg_sts0.reg  = CSI_GETREG(CSI_INTSTS0_REG_OFS); //dummy read for dummy interrupt
			return IRQ_HANDLED;
#endif
		}
#endif



		// Below are the error messages for the debug mode usages only.
		// And these msg would not be shown in the real application usages.
#if CSI_DEBUG
#if CSI_OPTION_MSG_PUT
		if ((reg_sts0.bit.ESC_RDY0) || (reg_sts0.bit.ESC_RDY1) || (reg_sts0.bit.ESC_RDY2) || (reg_sts0.bit.ESC_RDY3)) {
			DBG_DUMP("Valid = 0x%X   ESC Cmd = 0x%08X   \r\n", ((reg_sts0.reg >> 16) & 0xF), CSI_GETREG(CSI_ESCCMD_REG_OFS));
		}


		if (reg_sts0.bit.ECC_ERR) {
			DBG_DUMP(("ECC Err\r\n"));
		}
		if (reg_sts0.bit.CRC_ERR) {
			DBG_DUMP(("CRC Err\r\n"));
		}
		if (reg_sts0.bit.ECC_CORRECTED) {
			DBG_DUMP(("ECC CORRECTED\r\n"));
		}
		if (reg_sts0.bit.SOT_SYNC_ERR0) {
			DBG_DUMP(("SOT_SYNC_ERR0\r\n"));
		}
		if (reg_sts0.bit.SOT_SYNC_ERR1) {
			DBG_DUMP(("SOT_SYNC_ERR1\r\n"));
		}
		if (reg_sts0.bit.SOT_SYNC_ERR2) {
			DBG_DUMP(("SOT_SYNC_ERR2\r\n"));
		}
		if (reg_sts0.bit.SOT_SYNC_ERR3) {
			DBG_DUMP(("SOT_SYNC_ERR3\r\n"));
		}
		if (reg_sts0.bit.STATE_ERR0) {
			DBG_DUMP(("STATE_ERR0\r\n"));
		}
		if (reg_sts0.bit.STATE_ERR1) {
			DBG_DUMP(("STATE_ERR1\r\n"));
		}
		if (reg_sts0.bit.STATE_ERR2) {
			DBG_DUMP(("STATE_ERR2\r\n"));
		}
		if (reg_sts0.bit.STATE_ERR3) {
			DBG_DUMP(("STATE_ERR3\r\n"));
		}
		if (reg_sts0.bit.STATE_ERRC) {
			DBG_DUMP(("STATE_ERRC\r\n"));
		}
		if (reg_sts0.bit.DATA_LT_WC) {
			DBG_DUMP(("DATA_LT_WC\r\n"));
		}
		if (reg_sts0.bit.INVALID_DI) {
			T_CSI_RXDI_REG reg_rx_di;

			reg_rx_di.reg = CSI_GETREG(CSI_RXDI_REG_OFS);
			DBG_DUMP("CSI Invalid DI 0x%02X\r\n", reg_rx_di.bit.PKT_INVALID_DI);
		}
		if (reg_sts0.bit.GEN_GOT) {
			DBG_DUMP("GEN GOT 0x%06X\r\n", CSI_GETREG(CSI_RXSPKT1_REG_OFS));
		}
		if ((reg_sts1.bit.SOT_SYNC_CORRECTED0) || (reg_sts1.bit.SOT_SYNC_CORRECTED1) || (reg_sts1.bit.SOT_SYNC_CORRECTED2) || (reg_sts1.bit.SOT_SYNC_CORRECTED3)) {
			DBG_DUMP("CSI SOT_SYNC_CORRECTED 0x%X\r\n", ((reg_sts1.reg >> 8) & 0xF));
		}
#endif


		if (reg_sts0.bit.FS_SYNC_ERR) {
			DBG_DUMP(("FS Sync Err\r\n"));
		}
		if (reg_sts0.bit.LS_SYNC_ERR) {
			DBG_DUMP(("LS Sync Err\r\n"));
		}
		if (reg_sts0.bit.ULPS_ENTERC) {
			DBG_DUMP(("CK LANE ULPS ENTER\r\n"));
		}
		if (reg_sts0.bit.ULPS_LEAVEC) {
			DBG_DUMP(("CK LANE ULPS LEAVE\r\n"));
		}
		if ((reg_sts0.bit.ULPS_LEAVE0) || (reg_sts0.bit.ULPS_LEAVE1) || (reg_sts0.bit.ULPS_LEAVE2) || (reg_sts0.bit.ULPS_LEAVE3)) {
			DBG_DUMP("DAT LABE ULPS LEAVE 0x%X\r\n", ((reg_sts0.reg >> 24) & 0xF));
		}

		if ((reg_sts1.bit.ESC0_DATA_LT) || (reg_sts1.bit.ESC1_DATA_LT) || (reg_sts1.bit.ESC2_DATA_LT) || (reg_sts1.bit.ESC3_DATA_LT)) {
			DBG_DUMP("ESC LT 0x%X\r\n", reg_sts1.reg & 0xF);
		}
		if ((reg_sts1.bit.HS_EOT_TIMEOUT0) || (reg_sts1.bit.HS_EOT_TIMEOUT1) || (reg_sts1.bit.HS_EOT_TIMEOUT2) || (reg_sts1.bit.HS_EOT_TIMEOUT3)) {
			DBG_DUMP("EOT Timeout 0x%X\r\n", (reg_sts1.reg >> 4) & 0xF);
		}
		if (reg_sts1.bit.FRMEND) {
			DBG_DUMP(("CSI FRM END\r\n"));
		}
		if (reg_sts0.bit.FS_GOT) {
			DBG_DUMP(("CSI FS_GOT\r\n"));
		}
		if (reg_sts0.bit.FE_GOT) {
			DBG_DUMP(("CSI FE_GOT\r\n"));
		}
		if (reg_sts0.bit.LS_GOT) {
			DBG_DUMP(("CSI LS_GOT\r\n"));
		}
		if (reg_sts1.bit.FIFO_OV) {
			DBG_DUMP(("CSI FIFO_OV\r\n"));
		}
		if (reg_sts1.bit.FIFO2_OV) {
			DBG_DUMP(("CSI FIFO2_OV\r\n"));
		}
		if (reg_sts1.bit.FIFO3_OV) {
			DBG_DUMP(("CSI FIFO3_OV\r\n"));
		}
		if (reg_sts1.bit.FIFO4_OV) {
			DBG_DUMP(("CSI FIFO4_OV\r\n"));
		}
		if (reg_sts1.bit.FRMEND2) {
			DBG_DUMP(("CSI FRM END2\r\n"));
		}
#endif

#ifndef __KERNEL__
		iset_flg(FLG_ID_CSI, FLGPTN_CSI);
#else
		//complete(&csi_completion);
		csi_platform_complete(MODULE_ID);
#endif

	}

#if defined(__FREERTOS)
	return IRQ_HANDLED;
#endif
}

/*
    Lock CSI

    This api is used in the wait interrupt api to the multi-task access protection.
*/
static ER csi_lock(void)
{
#ifdef __KERNEL__
	return SEM_WAIT(MODULE_SEM_ID);
#else
#if defined(__FREERTOS)
	vos_sem_wait(MODULE_SEM_ID);
	return E_OK;
#else
	return wai_sem(MODULE_SEM_ID);
#endif
#endif
}

/*
    UnLock CSI

    This api is used in the wait interrupt api to the multi-task access protection.
*/
static ER csi_unlock(void)
{
#ifdef __KERNEL__
	SEM_SIGNAL(MODULE_SEM_ID);
	return E_OK;
#else
#if defined(__FREERTOS)
	vos_sem_sig(MODULE_SEM_ID);
	return E_OK;
#else
	return sig_sem(MODULE_SEM_ID);
#endif
#endif
}

static void csi_set_hsodly(UINT8 delay)
{
	T_CSI_DLY0_REG reg_dly0;

	reg_dly0.reg = CSI_GETREG(CSI_DLY0_REG_OFS);
	reg_dly0.bit.EN_HSDATA0_DLY = delay;
	CSI_SETREG(CSI_DLY0_REG_OFS, reg_dly0.reg);
}

/*
    Set CSI Interrupt Enable

    Set CSI Interrupt Enable

    @param[in] bint1
     - @b FALSE:    Set interrupt enable to interrupt Bank-0
     - @b TRUE:     Set interrupt enable to interrupt Bank-1
    @param[in] interr_en Bit-wise of the interrupt enable bits.

    @return void
*/
static void csi_set_int_enable(BOOL bint1, CSI_INTERRUPT interr_en)
{
	T_CSI_INTEN0_REG   reg_int_en0;
	T_CSI_INTEN1_REG   reg_int_en1;
	unsigned long      flags;

	// Enable Interrupt for bank 0 or 1.

	loc_cpu(flags);
	if (bint1) {
		reg_int_en1.reg  = CSI_GETREG(CSI_INTEN1_REG_OFS);
		reg_int_en1.reg |= interr_en;
		CSI_SETREG(CSI_INTEN1_REG_OFS, reg_int_en1.reg);
	} else {
		reg_int_en0.reg  = CSI_GETREG(CSI_INTEN0_REG_OFS);
		reg_int_en0.reg |= interr_en;
		CSI_SETREG(CSI_INTEN0_REG_OFS, reg_int_en0.reg);
	}
	unl_cpu(flags);

}

/*
    Set CSI Interrupt Disable

    Set CSI Interrupt Disable

    @param[in] bint1
     - @b FALSE:    Set interrupt Disable to interrupt Bank-0
     - @b TRUE:     Set interrupt Disable to interrupt Bank-1
    @param[in] interr_dis Bit-wise of the interrupt Disable bits.

    @return void
*/
static void csi_set_int_disable(BOOL bint1, CSI_INTERRUPT interr_dis)
{
	T_CSI_INTEN0_REG   reg_int_en0;
	T_CSI_INTEN1_REG   reg_int_en1;
	unsigned long      flags;

	loc_cpu(flags);
	if (bint1) {
		reg_int_en1.reg  = CSI_GETREG(CSI_INTEN1_REG_OFS);
		reg_int_en1.reg &= ~interr_dis;
		CSI_SETREG(CSI_INTEN1_REG_OFS, reg_int_en1.reg);
	} else {
		reg_int_en0.reg  = CSI_GETREG(CSI_INTEN0_REG_OFS);
		reg_int_en0.reg &= ~interr_dis;
		CSI_SETREG(CSI_INTEN0_REG_OFS, reg_int_en0.reg);
	}
	unl_cpu(flags);

}

/*
    Before CSI enabling, this would check the input lane swap configuration is valid or not.
    If not valid, the CSI controller would not be enabled.
*/
static BOOL csi_config_input_swap(void)
{
	UINT32              mask = 0;
	T_CSI_CTRL1_REG     reg_ctrl1;
	UINT32              number_of_lane, target_pad_no, lane_no, pad_no;
	//T_CSI_PHYRST_REG    reg_phyrst;
	//T_CSI_PHYDONE_REG   reg_phydone;
	//T_CSI_INSWAP_REG    inswap;

	number_of_lane = 4;
	reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);

    if (reg_ctrl1.bit.DLANE_NUM == CSI_DATLANE_NUM_1) {
		number_of_lane = 1;
    } else if (reg_ctrl1.bit.DLANE_NUM == CSI_DATLANE_NUM_2) {
		number_of_lane = 2;
    }

	csi_input_swap = csi_4_lanes_input_swap;
    for (lane_no = 0; lane_no < 4; lane_no+=1)
    {
        if (lane_no >= number_of_lane)
        {
            csi_input_swap &= ~(CSI_SWAP_MSK7 << (lane_no << 2));
            csi_input_swap |= (4 << (lane_no << 2));
        }
        target_pad_no = (csi_input_swap >> (lane_no << 2)) & CSI_SWAP_MSK7;

        for (pad_no = 0; pad_no < 4; pad_no++)
        {
            if (((csi_input_swap >> (CSI_PINSWAP_OFS + (pad_no << 2))) & CSI_SWAP_MSK7) == lane_no)
            {
                if (target_pad_no != pad_no)
                {
                    csi_input_swap &= ~(CSI_SWAP_MSK7 << (CSI_PINSWAP_OFS + (pad_no << 2)));
                    csi_input_swap |= (0x04 << (CSI_PINSWAP_OFS + (pad_no << 2)));
                } else if (((csi_input_swap >> (CSI_PINSWAP_OFS + (pad_no << 2))) & CSI_SWAP_MSK7) >= number_of_lane) {
					csi_input_swap &= ~(CSI_SWAP_MSK7 << (CSI_PINSWAP_OFS + (pad_no << 2)));
					csi_input_swap |= (4 << (CSI_PINSWAP_OFS + (lane_no << 2)));
                }
            }
        }
    }

	//DBG_ERR("csi_input_swap = 0x%x\r\n", (unsigned int) csi_input_swap);
	// The input swap configurations must not be overlapped.
	// This would check this and if violate the configuration rule,
	// This api would output the error messages.

	/*mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE0_OFS)&CSI_SWAP_MSK7));
	mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE1_OFS)&CSI_SWAP_MSK7));
	mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE2_OFS)&CSI_SWAP_MSK7));
	mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE3_OFS)&CSI_SWAP_MSK7));

	if (mask == 0xF) {
		// Valid Config
		CSI_SETREG(CSI_INSWAP_REG_OFS, csi_input_swap);
	} else {
		DBG_ERR("CSI Data Swap Config Overlaped\r\n");
		return TRUE;
	}*/
	CSI_SETREG(CSI_INSWAP_REG_OFS, csi_input_swap);

	mask = 0;
	reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
	if (reg_ctrl1.bit.DLANE_NUM == CSI_DATLANE_NUM_1) {
		mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE0_OFS)&CSI_SWAP_MSK));
		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD0_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET));
	} else if (reg_ctrl1.bit.DLANE_NUM == CSI_DATLANE_NUM_2) {
		mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE0_OFS)&CSI_SWAP_MSK));
		mask |= (0x1 << ((csi_input_swap >> CSI_SWAP_DATALANE1_OFS)&CSI_SWAP_MSK));

		CSI_SETREG(CSI_PHYRST_REG_OFS, 0x01);
		CSI_SETREG(CSI_PHYDONE_REG_OFS, 0x03);

		//DBG_ERR("\r\n config 0x48, 0x4C\r\n");

		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD0_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET));
		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD1_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS + CSI_SWAP_DATALANE1_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET));
	} else {
		mask = 0xF;

		CSI_SETREG(CSI_PHYRST_REG_OFS, 0x03);
		CSI_SETREG(CSI_PHYDONE_REG_OFS, 0x0F);

		//DBG_ERR("\r\n config 0x48, 0x4C\r\n");

		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD0_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD0_LPCLK - PLL_CLKSEL_R24_OFFSET));
		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD1_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS + CSI_SWAP_DATALANE1_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD1_LPCLK - PLL_CLKSEL_R24_OFFSET));
		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD2_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS + CSI_SWAP_DATALANE2_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD2_LPCLK - PLL_CLKSEL_R24_OFFSET));
		//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LPD3_LPCLK, (((csi_input_swap >> (CSI_LPSWAP_OFS + CSI_SWAP_DATALANE3_OFS))&CSI_SWAP_MSK) + 0) << (PLL_CLKSEL_LPD3_LPCLK - PLL_CLKSEL_R24_OFFSET));
	}

	senphy_set_valid_lanes(SENPHY_SEL_MIPILVDS, mask);

	return FALSE;
}

/*
    Set/Clear the Reset of the Analog PHY
static void csi_resetPhysicalLayer(BOOL bRst)
{
    T_CSI_PHYCTRL0_REG  RegPhy0;

    // Set/Clear Analog PHY Reset.
    // This would not be auto cleared, and the user must clear the reset for normal working.

    RegPhy0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
    RegPhy0.bit.RESET = bRst;
    CSI_SETREG(CSI_PHYCTRL0_REG_OFS, RegPhy0.reg);
}
*/

/*
    Enable/Disable the Analog PHY
static void csi_setAnalogEn(BOOL ben)
{
    T_CSI_PHYCTRL0_REG  RegPhy0;

    RegPhy0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
    RegPhy0.bit.EN_BIAS = ben;
    CSI_SETREG(CSI_PHYCTRL0_REG_OFS, RegPhy0.reg);
}
*/

/*
    Get the Sensor sent Escape Command

    This api can be used after the escape command ready interrupt.
    Currently, this is reserved api for internal usage and not the public api.
*/
void csi_get_escape_command(UINT32 *pcmd0, UINT32 *pcmd1, UINT32 *pcmd2, UINT32 *pcmd3)
{
	T_CSI_ESCCMD_REG   reg_esc;

	reg_esc.reg = CSI_GETREG(CSI_ESCCMD_REG_OFS);
	*pcmd0   = reg_esc.bit.ESC_CMD0;
	*pcmd1   = reg_esc.bit.ESC_CMD1;
	*pcmd2   = reg_esc.bit.ESC_CMD2;
	*pcmd3   = reg_esc.bit.ESC_CMD3;
}
#ifdef __KERNEL__
void csi_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_CSI);
	SEM_CREATE(SEMID_CSI, 1);
	//init_completion(&csi_completion);
	csi_platform_init_completion(MODULE_ID);
}
//EXPORT_SYMBOL(csi_create_resource);

void csi_release_resource(void)
{
	del_flg(FLG_ID_CSI);
	SEM_DESTROY(SEMID_CSI);
}

void csi_set_base_addr(UINT32 addr)
{
	_CSI_REG_BASE_ADDR = addr;

}
//EXPORT_SYMBOL(csi_set_base_addr);

void siecsi_set_base_addr(UINT32 addr)
{
	_SIE_CSI_REG_BASE_ADDR = addr;

}
//EXPORT_SYMBOL(siecsi_set_base_addr);

void csi_set_fastboot(void) {
	fast_boot = 1;
}

#endif

#ifndef __KERNEL__
void csi_init(void)
{
	vos_flag_create(&MODULE_FLAG_ID, NULL, "FLG_ID_CSI");
	vos_sem_create(&MODULE_SEM_ID, 1, "SEMID_CSI");
}

void csi_uninit(void)
{
	vos_sem_destroy(MODULE_SEM_ID);
	vos_flag_destroy(MODULE_FLAG_ID);
}
#endif

#if 1

/**
    Open the CSI module driver

    This api would initialize the CSI module needed resources, such as source clock, power,
    and OS interrupt. The user must open driver first before enabling CSI module.
    The user can use csi_is_opened() to check the driver is opened or not.

    @return
     - @b E_OK: Driver open done and success.
*/
ER csi_open(void)
{
#if defined(__KERNEL__) && (!_FPGA_EMULATION_)
	//struct clk *parent;
	//struct clk *csi_clk;
	unsigned long parent_rate = 0;
#endif
	//T_CSI_CTRL0_REG reg_ctrl0;
	//PIN_GROUP_CONFIG pinmux_config[1];


#ifndef __KERNEL__
	static BOOL rtos_init = 0;

	if (!rtos_init) {
		csi_init();
		rtos_init = 1;
	}
#endif

	if (csi_opened) {
		return E_OK;
	}

#if (DEBUG_MSG)
	DBG_DUMP("[%d]\r\n", __LINE__);
#endif

	csi_opened = TRUE;

	csi_timeout_period = 0;
	csi_force_dis = 0;
	csi_sensor_target_mclk = 0;
	csi_sensor_real_mclk = 0;
	//csi_input_swap = 0x32103210;

	//csi_installCmd();

#ifndef __KERNEL__
#if defined(_NVT_FPGA_) && defined(__FREERTOS)
#ifndef _NVT_EMULATION_
	pll_set_clock_rate(PLL_CLKSEL_MIPI_LVDS, PLL_CLKSEL_MIPI_LVDS_120);
#endif
#endif
#endif

	if (fast_boot == 0) {
		// Turn on power
		//pmc_turnonPower(PMC_MODULE_MIPI_CSI);
		senphy_init();
	}

#if !_FPGA_EMULATION_
	// Set PHY to Digital HSCLK Invert
	//CSI_SET_PLLCLKRATE(PLL_CLKSEL_LVDS_CLKPHASE, PLL_LVDS_CLK_PHASE_INVERT);
#endif

	//CSI_ENABLE_PLLCLK(MODULE_CLK_ID);
	csi_platform_clk_enable(MODULE_ID);

	// Turn on power
#ifndef __KERNEL__
	pll_enable_system_reset(MIPI_LVDS_RSTN);
	pll_disable_system_reset(MIPI_LVDS_RSTN);
#else
	if (fast_boot == 0) {
		iowrite32(ioread32((void *)(0xFD020088)) & ~(0x1 << 1), (void *)(0xFD020088));
		iowrite32(ioread32((void *)(0xFD020088)) | (0x1 << 1), (void *)(0xFD020088));
		DBG_DUMP("CSI HW reset!\r\n");
	}
#endif


	/* Clear Interrupt Status */
	CSI_SETREG(CSI_INTSTS0_REG_OFS, 0xFFFFFFFF);
	CSI_SETREG(CSI_INTSTS1_REG_OFS, 0xFFFFFFFF);

	/* Set Interrupt Enable */
	csi_set_int_enable(CSI_INT_BANK0, CSI_DEFAULT_INT0 | CSI_DEFAULT_INT0_FS);
	csi_set_int_enable(CSI_INT_BANK1, CSI_DEFAULT_INT1);

	clr_flg(FLG_ID_CSI, FLGPTN_CSI);

#ifndef __KERNEL__
#if defined(__FREERTOS)
	// clear the interrupt flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable interrupt
	request_irq(MODULE_DRV_INT, csi_isr ,IRQF_TRIGGER_HIGH, "csi1", 0);
#else
	// Enable interrupt
	CSI_ENABLE_DRVINT(DRV_INT_CSI);
#endif
#else
	// Enable interrupt
	CSI_ENABLE_DRVINT(DRV_INT_CSI);
#endif

	// Enable Analog PHY power for PHY.
	senphy_set_power(SENPHY_SEL_MIPILVDS, ENABLE);
	senphy_set_config(SENPHY_CONFIG_CSI_MODE, ENABLE);

	csi_err_monitor[0] = 0;
	csi_err_cnt[0]     = 0;
	csi_int_sts        = 0;

#ifndef __KERNEL__
#if !_FPGA_EMULATION_
	// Real Chip Usage: This is default timing according to the CSI standard.
	// If the sensor is not output the standard timing, we must fine tuning this parameter by our own.
	if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
		csi_set_hsodly(0x5);
	} /*else if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_80) {
		csi_set_hsodly(0x8);
	} */else if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
		csi_set_hsodly(0xa);
	}
#endif
#if defined(_NVT_FPGA_) && defined(__FREERTOS)
	csi_set_hsodly(0x5);
#endif
#else
	#if _FPGA_EMULATION_
		csi_set_hsodly(0x5);
	#else
	//csi_clk = clk_get(NULL, "f0280000.csi");
	//parent = clk_get_parent(csi_clk);
	//parent_rate = clk_get_rate(parent);
	csi_platform_clk_get_freq(MODULE_ID, &parent_rate);
	//nvt_dbg(ERR, "csi parent_clk = %ld\r\n", parent_rate);
	if (parent_rate == 60000000) {
		csi_set_hsodly(0x5);
	} else if (parent_rate == 80000000) {
		csi_set_hsodly(0x8);
	} else if (parent_rate == 120000000) {
		csi_set_hsodly(0xa);
	}
	//clk_put(csi_clk);
	#endif
#endif

	// The synchronous reset function must be disabled for CSI.
	// This function is designed for LVDS usages.
	csi_set_config(CSI_CONFIG_ID_A_SYNC_RST_EN, DISABLE);

	csi_set_config(CSI_CONFIG_ID_DEPACK_BIT_ORDER, ENABLE);

	csi_set_config(CSI_CONFIG_ID_A_FORCE_CLK_FSM, TRUE);

	//csi_set_config(CSI_CONFIG_ID_A_FORCE_CLK_HS, ENABLE); //for test

	return E_OK;
}

/**
    Close the CSI module driver

    This api would close the CSI module needed resources, such as source clock, power,
    and OS interrupt. The user should disable csi controller before closing it.

    @return
     - @b E_OK: Driver close done and success.
*/
ER csi_close(void)
{
	T_CSI_CTRL2_REG reg_ctr2;

	if (!csi_opened) {
		return E_OK;
	}

#if (DEBUG_MSG)
	DBG_DUMP("[%d]\r\n", __LINE__);
#endif

	// Check if the csi is disabled first. If not, output the warning messages.
	if (csi_get_enable()) {
		reg_ctr2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
		csi_set_enable(FALSE);
		DBG_WRN("CSI has not disable first\r\n");
		if (reg_ctr2.bit.DISABLE_SRC == 0x0) {
			csi_wait_interrupt(CSI_INTERRUPT_FRAME_END);
		} else if (reg_ctr2.bit.DISABLE_SRC == 0x1) {
			csi_wait_interrupt(CSI_INTERRUPT_FRAME_END2);
		} else if (reg_ctr2.bit.DISABLE_SRC == 0x2) {
			csi_wait_interrupt(CSI_INTERRUPT_FRAME_END); //no frame3
		} else {
			csi_wait_interrupt(CSI_INTERRUPT_FRAME_END); //no frame4
		}
	}

	// Close the MIPI clock source
	//CSI_DISABLE_PLLCLK(MODULE_CLK_ID);
	csi_platform_clk_disable(MODULE_ID);

#ifndef __KERNEL__
#if defined(__FREERTOS)
	// Disable interrupt
	free_irq(MODULE_DRV_INT, 0);
#else
	// Disable interrupt
	CSI_DISABLE_DRVINT(DRV_INT_CSI);
#endif
#else
	// Disable interrupt
	CSI_DISABLE_DRVINT(DRV_INT_CSI);
#endif

	csi_set_int_disable(CSI_INT_BANK0, CSI_DEFAULT_INT0);
	csi_set_int_disable(CSI_INT_BANK1, CSI_DEFAULT_INT1);

	// Close analog PHY powerl.
	senphy_set_power(SENPHY_SEL_MIPILVDS, DISABLE);

	csi_opened = FALSE;
	fast_boot = 0;
	return E_OK;
}

/**
    Check if the CSI Driver is opened or not

    Check if the CSI Driver is opened or not.

    @return
     - @b TRUE:  CSI driver is already opened.
     - @b FALSE: CSI driver has not be opened.
*/
BOOL csi_is_opened(void)
{
	return csi_opened;
}

/**
    Set the CSI Engine Enable/Disable

    This api is used to enable/disable the CSI engine. This api can only be used after CSI driver is opened.
    After set enabling of the CSI engine, the CSI controller would start working immediately.
    After set disabling of the CSI engine, the CSI controller would be stopped at the current receiving frame end.
    The user can use csi_wait_interrupt(CSI_INTERRUPT_FRAME_END) and also csi_get_enable() to ensure the CSI engine is disabled.

    @param[in] ben
     - @b TRUE:  Enable the CSI engine.
     - @b FALSE: Disable the CSI engine.

    @return
     - @b E_OK:  Configuration is done and success.
     - @b E_SYS: CSI Driver has not be opened. Please use csi_open() first.
     - @b E_PAR: The CSI input lane swap configurations invalid. Please check the csi_set_config(CSI_CONFIG_ID_DATALANE*_PIN) config.
*/
ER csi_set_enable(BOOL ben)
{
	T_CSI_CTRL0_REG reg_ctrl0;
	//BOOL            bclken;
#if (CSI_ECO_ENABLE == ENABLE)
	T_CSI_SIE_REG   reg_csi_sie;
#endif
#if defined(__KERNEL__) && (!_FPGA_EMULATION_)
	//struct clk *parent;
	//struct clk *csi_clk;
	unsigned long parent_rate = 0;
#endif

	if (!csi_opened) {
		DBG_ERR("CSI must open first\r\n");
		return E_SYS;
	}

#if (DEBUG_MSG)
	DBG_DUMP("[%d]: ben = %d\r\n", __LINE__, ben);
#endif

	if (!ben) {
		csi_set_int_disable(CSI_INT_BANK0, CSI_DEFAULT_INT0_FS); //disable FS interrupt
		CSI_SETREG(CSI_INTSTS0_REG_OFS, CSI_DEFAULT_INT0_FS); //clear FS interrupt status
	}

	if (ben) {
		if (phy_hso_delay == 0) {
		#ifndef __KERNEL__

		#if defined(_NVT_FPGA_) && defined(__FREERTOS)
		//csi_set_hsodly(0x5 * csi_sensor_target_mclk / csi_sensor_real_mclk);
		if (csi_sensor_real_mclk) {
			if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
				csi_set_hsodly(0x5 * csi_sensor_target_mclk / csi_sensor_real_mclk);
			} else if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
				csi_set_hsodly(0xA * csi_sensor_target_mclk / csi_sensor_real_mclk);
			}
		}
		#else
		if (csi_sensor_real_mclk) {
			if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
				csi_set_hsodly(0x5 * csi_sensor_target_mclk / csi_sensor_real_mclk);
			} else if (CSI_GET_PLLCLKRATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
				csi_set_hsodly(0xA * csi_sensor_target_mclk / csi_sensor_real_mclk);
			}
		}
		#endif
		#else

		#if _FPGA_EMULATION_
			csi_set_hsodly(0x5 * csi_sensor_target_mclk / csi_sensor_real_mclk);
		#else
		if (csi_sensor_real_mclk) {
			//csi_clk = clk_get(NULL, "f0280000.csi");
			//parent = clk_get_parent(csi_clk);
			//parent_rate = clk_get_rate(parent);
			csi_platform_clk_get_freq(MODULE_ID, &parent_rate);
			//nvt_dbg(ERR, "csi parent_clk = %ld\r\n", parent_rate);
			if (parent_rate == 60000000) {
				csi_set_hsodly(0x5 * csi_sensor_target_mclk / csi_sensor_real_mclk);
			} else if (parent_rate == 120000000) {
				csi_set_hsodly(0xA * csi_sensor_target_mclk / csi_sensor_real_mclk);
			}
			//clk_put(csi_clk);
		}
		#endif
		#endif
		}else {
			csi_set_hsodly(phy_hso_delay);
		}

		// If setting enable, check the input swap setting is valid or not.
		// If not valid, Reject the enable request.
		csi_err_monitor[0] = 0;
		csi_err_cnt[0]    = 0;
		csi_err_sts[0]    = 0;
		csi_err_sts[1]    = 0;
		ecc_cnt = 0;

		if (csi_config_input_swap()) {
			return E_PAR;
		}
	}

	if (csi_clk_switch == TRUE) {
		senphy_set_clock_map(SENPHY_SEL_MIPILVDS, SENPHY_CLKMAP_CK2);
	} else {
		senphy_set_clock_map(SENPHY_SEL_MIPILVDS, SENPHY_CLKMAP_CK0);    // Or SENPHY_CLKMAP_CK2
	}

	// If setting enable, assert the reset of the analog PHY.
	//if(ben && (csi2_get_enable() == FALSE))
	if (fast_boot == 0) {
		if (ben) {
#if (CSI_ECO_ENABLE == ENABLE)
			reg_csi_sie.reg = CSI_SIE_GETREG(CSI_SIE_REG_OFS);
			if (reg_csi_sie.bit.CSI_HWRESET == 0) {
				reg_csi_sie.bit.CSI_HWRESET = 1;
				CSI_SIE_SETREG(CSI_SIE_REG_OFS, reg_csi_sie.reg);
			}
#endif
			// Set PHY Enable/Disable
			senphy_enable(SENPHY_SEL_MIPILVDS, DISABLE);
	//#if _FPGA_EMULATION_
			CSI_DELAY_US(1);
	//#endif
			senphy_enable(SENPHY_SEL_MIPILVDS, ENABLE);

#if (CSI_ECO_ENABLE == ENABLE)
#if (CSI_ECO_HWRST == DISABLE)
			reg_csi_sie.reg = CSI_SIE_GETREG(CSI_SIE_REG_OFS);
			if (reg_csi_sie.bit.CSI_HWRESET != 0) {
				reg_csi_sie.bit.CSI_HWRESET = 0;
				CSI_SIE_SETREG(CSI_SIE_REG_OFS, reg_csi_sie.reg);
			}
#endif
#endif

		}
	} else {
		if (ben) {
			fast_boot = 0;
		}
	}
	// Set PHY Enable/Disable
	//senphy_enable(SENPHY_SEL_MIPILVDS, ben>0);
	//senphy_setConfig(SENPHY_CONFIG_ID_IADJ, 0x2);

	/*if (ben) {
		bclken = CSI_ISENABLE_PLLCLK(MIPI_LVDS_CLK);
		if (bclken == TRUE) {
			//CSI_DISABLE_PLLCLK(MIPI_LVDS_CLK);
			csi_platform_clk_disable(MODULE_ID);
			//CHKPNT;
			CSI_DELAY_US(100);
			//CHKPNT;
		}
	}*/

	// Set csi controller enable/disable
	reg_ctrl0.reg = CSI_GETREG(CSI_CTRL0_REG_OFS);
	reg_ctrl0.bit.CSI_EN         = (ben > 0);
	reg_ctrl0.bit.CSI_DBGMUX_EN  = 1;
	reg_ctrl0.bit.DBG_SEL      = 0x02;
	//reg_ctrl0.bit.TEST28 = 1;
	//reg_ctrl0.bit.TEST29 = 1;
	//reg_ctrl0.bit.TEST30 = 1;
	/*#if CSI_FORCE_OFF
	    // Force OFF or FrmEndOff
	    reg_ctrl0.bit.EN_SEL         = 1;
	#else
	    reg_ctrl0.bit.EN_SEL         = 0;
	#endif*/
	reg_ctrl0.bit.EN_SEL         = csi_force_dis;

#if (CSI_ENABLE_DESKEW == ENABLE)
	reg_ctrl0.bit.EN_DESKEW      = 1;
#else
	reg_ctrl0.bit.EN_DESKEW      = 0;
#endif

	CSI_SETREG(CSI_CTRL0_REG_OFS, reg_ctrl0.reg);

	/*if (ben) {
		if (bclken == TRUE) {
			//CSI_ENABLE_PLLCLK(MODULE_CLK_ID);
			csi_platform_clk_enable(MODULE_ID);
		}
	}*/

	// If setting enable, assert the reset of the analog PHY.
	//if(ben && (csi2_get_enable() == FALSE))
	//{
	//    csi_resetPhysicalLayer(TRUE);
	//    #if _FPGA_EMULATION_
	//    CSI_DELAY_US(10);
	//    #endif
	//    csi_resetPhysicalLayer(FALSE);
	//}
	if (ben) {
		CSI_SETREG(CSI_INTSTS0_REG_OFS, CSI_DEFAULT_INT0_FS); //clear FS interrupt status
		csi_set_int_enable(CSI_INT_BANK0, CSI_DEFAULT_INT0_FS); //enable FS interrupt
	}
	return E_OK;
}

/**
    Check if the CSI engine is enabled or not

    Check if the CSI engine is enabled or not.

    @return
     - @b TRUE:  CSI engine is already opened.
     - @b FALSE: CSI engine has not be opened.
*/
BOOL csi_get_enable(void)
{
	T_CSI_CTRL0_REG reg_ctrl0;

	reg_ctrl0.reg = CSI_GETREG(CSI_CTRL0_REG_OFS);
	return reg_ctrl0.bit.CSI_EN;
}

/**
    Set CSI engine general configurations

    This api is used to configure the general function of the CSI engine,
    such as DataLane number, Receiving Mode, Lane Swap,... etc.

    @param[in] config_id     The configuration selection ID. Please refer to CSI_CONFIG_ID for details.
    @param[in] cfg_value   The configuration parameter according to the config_id.

    @return void
*/
void csi_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value)
{
	// This API is used to configure the CSI settings.
	// In current design, this would not be blocked by the driver open/close status.
	// Because the csi enable bit would be blocked if the driver no opened.
	unsigned long      flags;

	loc_cpu(flags);
	switch (config_id) {
	case CSI_CONFIG_ID_DIS_METHOD: {
			//csi_force_dis = cfg_value > 0;
		}
		break;

	case CSI_CONFIG_ID_LPKT_AUTO: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Auto-Mode enable    TRUE: Auto-Mode disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.LPKT_AUTO_EN = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode1 enable    TRUE: Manual-Mode1 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.LPKT_MANUAL_EN = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL2: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode2 enable    TRUE: Manual-Mode2 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.LPKT_MANUAL2_EN = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL3: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode3 enable    TRUE: Manual-Mode3 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.LPKT_MANUAL3_EN = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_PHECC_BITORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_BIT_ORD_LSB / CSI_PH_ECC_BIT_ORD_MSB
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			reg_debug.bit.PH_ECC_BIT_ORD     = cfg_value;
			CSI_SETREG(CSI_DEBUG_REG_OFS, reg_debug.reg);
		}
		break;

	case CSI_CONFIG_ID_PHECC_BYTEORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_BYTE_ORD_LSB / CSI_PH_ECC_BYTE_ORD_MSB
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			reg_debug.bit.PH_ECC_WC_BYTE_ORD = cfg_value;
			CSI_SETREG(CSI_DEBUG_REG_OFS, reg_debug.reg);
		}
		break;

	case CSI_CONFIG_ID_PHECC_FIELDORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_FIED_ORD_DIWC / CSI_PH_ECC_FIED_ORD_WCDI
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			reg_debug.bit.PH_ECC_FIELD_ORD   = cfg_value;
			CSI_SETREG(CSI_DEBUG_REG_OFS, reg_debug.reg);
		}
		break;

	case CSI_CONFIG_ID_HD_GEN_METHOD: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_LINESYNC_SPKT / CSI_LINESYNC_PKTHEAD
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.LINE_SYNC_METHOD = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_ROUND_ENABLE: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// ENABLE / DISABLE
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.ROUND_EN = (cfg_value > 0);
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_HD_GATING_EN: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// ENABLE / DISABLE
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.HD_GATING = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;


	case CSI_CONFIG_ID_DLANE_NUMBER: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_DATLANE_NUM_1 / CSI_DATLANE_NUM_2 / CSI_DATLANE_NUM_4
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.DLANE_NUM = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL_FORMAT: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.MAN_DEPACK = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL_DATA_ID: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// 8Bits Value
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.MANUAL_DI = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL2_FORMAT: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.MAN_DEPACK2 = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL2_DATA_ID: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// 8Bits Value
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.MANUAL_DI2 = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL3_FORMAT: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.MAN_DEPACK3 = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_MANUAL3_DATA_ID: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// 8Bits Value
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.MANUAL_DI3 = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_CHID_MODE: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_CHID_MIPIDI / CSI_CHID_SONYLI
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.CH_ID_MODE = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.VIRTUAL_ID = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID2: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.VIRTUAL_ID2 = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID3: {
			T_CSI_CTRL3_REG    reg_ctrl3;

			reg_ctrl3.reg = CSI_GETREG(CSI_CTRL3_REG_OFS);
			reg_ctrl3.bit.VIRTUAL_ID3 = cfg_value;
			CSI_SETREG(CSI_CTRL3_REG_OFS, reg_ctrl3.reg);
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID4: {
			T_CSI_CTRL3_REG    reg_ctrl3;

			reg_ctrl3.reg = CSI_GETREG(CSI_CTRL3_REG_OFS);
			reg_ctrl3.bit.VIRTUAL_ID4 = cfg_value;
			CSI_SETREG(CSI_CTRL3_REG_OFS, reg_ctrl3.reg);
		}
		break;


	case CSI_CONFIG_ID_VALID_HEIGHT: {
			if (cfg_value > 65535) {
				cfg_value = 65535;
			}

			CSI_SETREG(CSI_COUNT_REG_OFS, cfg_value);
		}
		break;

	case CSI_CONFIG_ID_VALID_HEIGHT2: {
			if (cfg_value > 65535) {
				cfg_value = 65535;
			}

			CSI_SETREG(CSI_COUNT2_REG_OFS, cfg_value);
		}
		break;

	case CSI_CONFIG_ID_VALID_HEIGHT3: {
			T_CSI_COUNT3_REG    reg_count3;

			if (cfg_value > 65535) {
				cfg_value = 65535;
			}

			reg_count3.reg = CSI_GETREG(CSI_COUNT3_REG_OFS);
			reg_count3.bit.FRMEND3_LINE_CNT = cfg_value;

			CSI_SETREG(CSI_COUNT3_REG_OFS, reg_count3.reg);
		}
		break;

	case CSI_CONFIG_ID_VALID_HEIGHT4: {
			T_CSI_COUNT3_REG    reg_count3;

			if (cfg_value > 65535) {
				cfg_value = 65535;
			}

			reg_count3.reg = CSI_GETREG(CSI_COUNT3_REG_OFS);
			reg_count3.bit.FRMEND4_LINE_CNT = cfg_value;

			CSI_SETREG(CSI_COUNT3_REG_OFS, reg_count3.reg);
		}
		break;


	case CSI_CONFIG_ID_VD_ISSUE_TWOSIE: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl1.bit.VD_ISSUE_TWO = cfg_value;
			CSI_SETREG(CSI_CTRL1_REG_OFS, reg_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_EOT_TIMEOUT: {
			if (cfg_value > 65535) {
				cfg_value = 65535;
			}

			CSI_SETREG(CSI_HSTIMEOUT_REG_OFS, cfg_value);
		}
		break;

	case CSI_CONFIG_ID_DEPACK_BYTE_ORDER: {
			T_CSI_DEBUG_REG reg_dbg;

			reg_dbg.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			reg_dbg.bit.PIX_DEPACK_BYTE_ORD = cfg_value;
			CSI_SETREG(CSI_DEBUG_REG_OFS, reg_dbg.reg);
		}
		break;

	case CSI_CONFIG_ID_DEPACK_BIT_ORDER: {
			T_CSI_DEBUG_REG reg_dbg;

			reg_dbg.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			reg_dbg.bit.PIX_DEPACK_BIT_ORD = 1;
			CSI_SETREG(CSI_DEBUG_REG_OFS, reg_dbg.reg);
		}
		break;

	case CSI_CONFIG_ID_DATALANE0_PIN:
	case CSI_CONFIG_ID_DATALANE1_PIN:
	case CSI_CONFIG_ID_DATALANE2_PIN:
	case CSI_CONFIG_ID_DATALANE3_PIN: {
			if (cfg_value > CSI_INPUT_PIN_D3PN) {
				break;
			}



			config_id = config_id - CSI_CONFIG_ID_DATALANE0_PIN;

			csi_4_lanes_input_swap &= ~(CSI_SWAP_MSK << (config_id << 2));
			csi_4_lanes_input_swap |= (cfg_value << (config_id << 2));

			csi_4_lanes_input_swap &= ~(CSI_SWAP_MSK << (CSI_PINSWAP_OFS + (cfg_value << 2)));
			csi_4_lanes_input_swap |= (config_id << (CSI_PINSWAP_OFS + (cfg_value << 2)));
			//DBG_ERR("csi_4_lanes_input_swap = 0x%x\r\n", (unsigned int)(csi_4_lanes_input_swap));
		}
		break;

	case CSI_CONFIG_ID_IMGDUPLICATE_EN: {
			T_CSI_CTRL1_REG    reg_ctrl1;
			T_CSI_CTRL2_REG    reg_ctrl2;
			T_CSI_CTRL3_REG    reg_ctrl3;

			//read VCHID and VCHID2
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl3.reg = CSI_GETREG(CSI_CTRL3_REG_OFS);

			if (cfg_value != 0) {
				//write VCHID2 with VCHID
				reg_ctrl2.bit.VIRTUAL_ID2 = reg_ctrl1.bit.VIRTUAL_ID;
				CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
				reg_ctrl3.bit.VIRTUAL_ID3 = reg_ctrl1.bit.VIRTUAL_ID;
				reg_ctrl3.bit.VIRTUAL_ID4 = reg_ctrl1.bit.VIRTUAL_ID;
				CSI_SETREG(CSI_CTRL3_REG_OFS, reg_ctrl3.reg);
			} else {
				//write VCHID2 with VCHID
				if (reg_ctrl1.bit.VIRTUAL_ID == reg_ctrl2.bit.VIRTUAL_ID2) {
					reg_ctrl2.bit.VIRTUAL_ID2 = reg_ctrl1.bit.VIRTUAL_ID + 1;
					CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
					DBG_WRN("CSI: The original VCHID == VCHID2, driver set the VCHID2 = VCHID + 1\r\n");
					reg_ctrl3.bit.VIRTUAL_ID3 = reg_ctrl1.bit.VIRTUAL_ID + 2;
					reg_ctrl3.bit.VIRTUAL_ID4 = reg_ctrl1.bit.VIRTUAL_ID + 3;
					CSI_SETREG(CSI_CTRL3_REG_OFS, reg_ctrl3.reg);
					DBG_WRN("CSI: The original VCHID == VCHID3, driver set the VCHID3 = VCHID + 2\r\n");
					DBG_WRN("CSI: The original VCHID == VCHID4, driver set the VCHID4 = VCHID + 3\r\n");
				}
			}
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT0: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			reg_li.bit.CHID_BIT0_OFS = cfg_value;
			CSI_SETREG(CSI_LI_REG_OFS, reg_li.reg);
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT1: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			reg_li.bit.CHID_BIT1_OFS = cfg_value;
			CSI_SETREG(CSI_LI_REG_OFS, reg_li.reg);
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT2: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			reg_li.bit.CHID_BIT2_OFS = cfg_value;
			CSI_SETREG(CSI_LI_REG_OFS, reg_li.reg);
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT3: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			reg_li.bit.CHID_BIT3_OFS = cfg_value;
			CSI_SETREG(CSI_LI_REG_OFS, reg_li.reg);
		}
		break;

	case CSI_CONFIG_ID_DIS_SRC: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			reg_ctrl2.bit.DISABLE_SRC = cfg_value;
			CSI_SETREG(CSI_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case CSI_CONFIG_ID_PATGEN_EN: {
			T_CSI_PATGEN_REG reg_pat;

			reg_pat.reg = CSI_GETREG(CSI_PATGEN_REG_OFS);
			reg_pat.bit.PATGEN_EN = cfg_value;
			CSI_SETREG(CSI_PATGEN_REG_OFS, reg_pat.reg);
		}
		break;

	case CSI_CONFIG_ID_PATGEN_MODE: {
			T_CSI_PATGEN_REG reg_pat;

			reg_pat.reg = CSI_GETREG(CSI_PATGEN_REG_OFS);
			reg_pat.bit.PATGEN_MODE = cfg_value;
			CSI_SETREG(CSI_PATGEN_REG_OFS, reg_pat.reg);
		}
		break;

	case CSI_CONFIG_ID_PATGEN_VAL: {
			T_CSI_PATGEN_REG reg_pat;

			reg_pat.reg = CSI_GETREG(CSI_PATGEN_REG_OFS);
			reg_pat.bit.PATGEN_VAL = cfg_value;
			CSI_SETREG(CSI_PATGEN_REG_OFS, reg_pat.reg);
		}
		break;

	/*
	    Analog Block Control
	*/
	case CSI_CONFIG_ID_HSDATAO_DELAY: {
			//T_CSI_DLY0_REG reg_dly0;

			if (cfg_value > 127) {
				cfg_value = 127;
			}
			phy_hso_delay = (UINT8)cfg_value;
			/*reg_dly0.reg = CSI_GETREG(CSI_DLY0_REG_OFS);
			reg_dly0.bit.EN_HSDATA0_DLY = cfg_value;
			CSI_SETREG(CSI_DLY0_REG_OFS, reg_dly0.reg);*/
		}
		break;

	case CSI_CONFIG_ID_HSDATAO_EDGE: {
			T_CSI_DLY0_REG reg_dly0;

			reg_dly0.reg = CSI_GETREG(CSI_DLY0_REG_OFS);
			reg_dly0.bit.OCLK_EDGE = cfg_value;
			CSI_SETREG(CSI_DLY0_REG_OFS, reg_dly0.reg);
		}
		break;

	case CSI_CONFIG_ID_HSDONE_DLY_CNT: {
			T_CSI_DLY1_REG reg_dly1;

			reg_dly1.reg = CSI_GETREG(CSI_DLY1_REG_OFS);
			reg_dly1.bit.DLY_CNT = cfg_value;
			CSI_SETREG(CSI_DLY1_REG_OFS, reg_dly1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_FORCE_CLK_HS: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.CLK_FORCE_HS = cfg_value > 0;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_FORCE_CLK_FSM: {
		T_CSI_PHYCTRL1_REG phy_ctrl1;

		phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
		phy_ctrl1.bit.CLK_FORCE_FSM = cfg_value > 0;
		CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
	}
	break;

	case CSI_CONFIG_ID_A_DLY_ENABLE: {
			T_CSI_PHYCTRL0_REG phy_ctrl0;

			phy_ctrl0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
			phy_ctrl0.bit.DLY_EN = cfg_value > 0;
			CSI_SETREG(CSI_PHYCTRL0_REG_OFS, phy_ctrl0.reg);
		}
		break;

	case CSI_CONFIG_ID_A_DLY_CLK: {
			T_CSI_PHYCTRL0_REG phy_ctrl0;

			phy_ctrl0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
			phy_ctrl0.bit.CLK_DESKEW = cfg_value;
			CSI_SETREG(CSI_PHYCTRL0_REG_OFS, phy_ctrl0.reg);
		}
		break;

	case CSI_CONFIG_ID_A_DLY_DAT0: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.DL0_DESKEW = cfg_value;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_DLY_DAT1: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.DL1_DESKEW = cfg_value;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_DLY_DAT2: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.DL2_DESKEW = cfg_value;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_DLY_DAT3: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.DL3_DESKEW = cfg_value;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	case CSI_CONFIG_ID_A_IADJ: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			phy_ctrl1.bit.IADJ = cfg_value > 0;
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;

	/*case CSI_CONFIG_ID_A_SYNC_RST_EN: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			if (cfg_value) {
				phy_ctrl1.bit.HSRX_OPT = 0;
			} else {
				phy_ctrl1.bit.HSRX_OPT = 2;
			}
			CSI_SETREG(CSI_PHYCTRL1_REG_OFS, phy_ctrl1.reg);
		}
		break;*/

	case CSI_CONFIG_ID_CLANE_SWITCH: {
			csi_clk_switch = cfg_value > 0;
		}
		break;

	case CSI_CONFIG_ID_TIMEOUT_PERIOD: {
			csi_timeout_period = cfg_value;
		}
		break;

	case CSI_CONFIG_ID_SENSOR_TARGET_MCLK: {
			csi_sensor_target_mclk = cfg_value;
		}
		break;
	case CSI_CONFIG_ID_SENSOR_REAL_MCLK: {
			csi_sensor_real_mclk = cfg_value;
		}
		break;

	default:
		break;

	}
	unl_cpu(flags);

}

/**
    Get CSI engine general configurations

    This api is used to retrieve the general function configuration value of the CSI engine,
    such as DataLane number, Receiving Mode, ... etc.

    @param[in] config_id     The configuration selection ID. Please refer to CSI_CONFIG_ID for details.

    @return The configuration parameter according to the config_id. Please refer to CSI_CONFIG_ID for details.
*/
UINT32 csi_get_config(CSI_CONFIG_ID config_id)
{
	UINT32 ret = 0x0;

	switch (config_id) {
	case CSI_CONFIG_ID_DIS_METHOD: {
			ret = (csi_force_dis > 0);
		}
		break;

	case CSI_CONFIG_ID_LPKT_AUTO: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.LPKT_AUTO_EN;
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode1 enable    TRUE: Manual-Mode1 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.LPKT_MANUAL_EN;
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL2: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode2 enable    TRUE: Manual-Mode2 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.LPKT_MANUAL2_EN;
		}
		break;

	case CSI_CONFIG_ID_LPKT_MANUAL3: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// TRUE: Manual-Mode3 enable    TRUE: Manual-Mode3 disable
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.LPKT_MANUAL3_EN;
		}
		break;

	case CSI_CONFIG_ID_PHECC_BITORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_BIT_ORD_LSB / CSI_PH_ECC_BIT_ORD_MSB
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			ret = reg_debug.bit.PH_ECC_BIT_ORD;
		}
		break;

	case CSI_CONFIG_ID_PHECC_BYTEORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_BYTE_ORD_LSB / CSI_PH_ECC_BYTE_ORD_MSB
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			ret = reg_debug.bit.PH_ECC_WC_BYTE_ORD;
		}
		break;

	case CSI_CONFIG_ID_PHECC_FIELDORDER: {
			T_CSI_DEBUG_REG    reg_debug;

			// CSI_PH_ECC_FIED_ORD_DIWC / CSI_PH_ECC_FIED_ORD_WCDI
			reg_debug.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			ret = reg_debug.bit.PH_ECC_FIELD_ORD;
		}
		break;

	case CSI_CONFIG_ID_HD_GEN_METHOD: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_LINESYNC_SPKT / CSI_LINESYNC_PKTHEAD
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.LINE_SYNC_METHOD;
		}
		break;

	case CSI_CONFIG_ID_ROUND_ENABLE: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// ENABLE / DISABLE
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.ROUND_EN;
		}
		break;

	case CSI_CONFIG_ID_DLANE_NUMBER: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_DATLANE_NUM_1 / CSI_DATLANE_NUM_2 / CSI_DATLANE_NUM_4
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.DLANE_NUM;
		}
		break;

	case CSI_CONFIG_ID_MANUAL_FORMAT: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.MAN_DEPACK;
		}
		break;

	case CSI_CONFIG_ID_MANUAL_DATA_ID: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// 8Bits Value
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.MANUAL_DI;
		}
		break;

	case CSI_CONFIG_ID_MANUAL2_FORMAT: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.MAN_DEPACK2;
		}
		break;

	case CSI_CONFIG_ID_MANUAL2_DATA_ID: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// 8Bits Value
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.MANUAL_DI2;
		}
		break;

	case CSI_CONFIG_ID_MANUAL3_FORMAT: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// CSI_MANDEPACK_8 / CSI_MANDEPACK_10 / CSI_MANDEPACK_12 / CSI_MANDEPACK_14
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.MAN_DEPACK3;
		}
		break;

	case CSI_CONFIG_ID_MANUAL3_DATA_ID: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			// 8Bits Value
			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.MANUAL_DI3;
		}
		break;

	case CSI_CONFIG_ID_CHID_MODE: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			// CSI_CHID_MIPIDI / CSI_CHID_SONYLI
			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.CH_ID_MODE;
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID: {
			T_CSI_CTRL1_REG    reg_ctrl1;

			reg_ctrl1.reg = CSI_GETREG(CSI_CTRL1_REG_OFS);
			ret = reg_ctrl1.bit.VIRTUAL_ID;
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID2: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.VIRTUAL_ID2;
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID3: {
			T_CSI_CTRL3_REG    reg_ctrl3;

			reg_ctrl3.reg = CSI_GETREG(CSI_CTRL3_REG_OFS);
			ret = reg_ctrl3.bit.VIRTUAL_ID3;
		}
		break;

	case CSI_CONFIG_ID_VIRTUAL_ID4: {
			T_CSI_CTRL3_REG    reg_ctrl3;

			reg_ctrl3.reg = CSI_GETREG(CSI_CTRL3_REG_OFS);
			ret = reg_ctrl3.bit.VIRTUAL_ID4;
		}
		break;

	case CSI_CONFIG_ID_VALID_HEIGHT: {
			ret = CSI_GETREG(CSI_COUNT_REG_OFS);
		}
		break;

	case CSI_CONFIG_ID_EOT_TIMEOUT: {
			ret = CSI_GETREG(CSI_HSTIMEOUT_REG_OFS);
		}
		break;


	case CSI_CONFIG_ID_HSDATAO_DELAY: {
			T_CSI_DLY0_REG reg_dly0;

			reg_dly0.reg = CSI_GETREG(CSI_DLY0_REG_OFS);
			ret = reg_dly0.bit.EN_HSDATA0_DLY;
		}
		break;

	case CSI_CONFIG_ID_DEPACK_BYTE_ORDER: {
			T_CSI_DEBUG_REG reg_dbg;

			reg_dbg.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			ret = reg_dbg.bit.PIX_DEPACK_BYTE_ORD;
		}
		break;

	case CSI_CONFIG_ID_DEPACK_BIT_ORDER: {
			T_CSI_DEBUG_REG reg_dbg;

			reg_dbg.reg = CSI_GETREG(CSI_DEBUG_REG_OFS);
			ret = reg_dbg.bit.PIX_DEPACK_BIT_ORD;
		}
		break;

	case CSI_CONFIG_ID_DATALANE0_PIN:
	case CSI_CONFIG_ID_DATALANE1_PIN:
	case CSI_CONFIG_ID_DATALANE2_PIN:
	case CSI_CONFIG_ID_DATALANE3_PIN: {
			T_CSI_INSWAP_REG    in_swap;

			config_id = config_id - CSI_CONFIG_ID_DATALANE0_PIN;

			in_swap.reg = CSI_GETREG(CSI_INSWAP_REG_OFS);
			ret = (in_swap.reg >> (config_id << 2)) & CSI_SWAP_MSK;

		}
		break;

	case CSI_CONFIG_ID_DIS_SRC: {
			T_CSI_CTRL2_REG    reg_ctrl2;

			reg_ctrl2.reg = CSI_GETREG(CSI_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.DISABLE_SRC;
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT0: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			ret = reg_li.bit.CHID_BIT0_OFS;
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT1: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			ret = reg_li.bit.CHID_BIT1_OFS;
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT2: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			ret = reg_li.bit.CHID_BIT2_OFS;
		}
		break;

	case CSI_CONFIG_ID_LI_CHID_BIT3: {
			T_CSI_LI_REG reg_li;

			reg_li.reg = CSI_GETREG(CSI_LI_REG_OFS);
			ret = reg_li.bit.CHID_BIT3_OFS;
		}
		break;

	case CSI_CONFIG_ID_FRAME_NUMBER: {
			T_CSI_RXSPKT0_REG reg_spkt;

			reg_spkt.reg = CSI_GETREG(CSI_RXSPKT0_REG_OFS);
			ret = reg_spkt.bit.FRM_NUMBER;
		}
		break;

	/*
	    Analog Block Control
	*/
	case CSI_CONFIG_ID_A_DLY_ENABLE: {
			T_CSI_PHYCTRL0_REG phy_ctrl0;

			phy_ctrl0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
			ret = phy_ctrl0.bit.DLY_EN;
		}
		break;

	case CSI_CONFIG_ID_A_DLY_CLK: {
			T_CSI_PHYCTRL0_REG phy_ctrl0;

			phy_ctrl0.reg = CSI_GETREG(CSI_PHYCTRL0_REG_OFS);
			ret = phy_ctrl0.bit.CLK_DESKEW;
		}
		break;

	case CSI_CONFIG_ID_A_DLY_DAT0:
	case CSI_CONFIG_ID_A_DLY_DAT1:
	case CSI_CONFIG_ID_A_DLY_DAT2:
	case CSI_CONFIG_ID_A_DLY_DAT3: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			config_id = config_id - CSI_CONFIG_ID_A_DLY_DAT0;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			ret = (phy_ctrl1.reg >> (config_id << 2)) & CSI_ANALOG_DLY_MSK;
		}
		break;

	case CSI_CONFIG_ID_A_IADJ: {
			T_CSI_PHYCTRL1_REG phy_ctrl1;

			phy_ctrl1.reg = CSI_GETREG(CSI_PHYCTRL1_REG_OFS);
			ret = phy_ctrl1.bit.IADJ;
		}
		break;

	case CSI_CONFIG_ID_CLANE_SWITCH: {
			ret = csi_clk_switch;
		}
		break;
	case CSI_CONFIG_ID_TIMEOUT_PERIOD: {
			ret = csi_timeout_period;
		}
		break;
	case CSI_CONFIG_ID_SENSOR_TARGET_MCLK: {
			ret = csi_sensor_target_mclk;
		}
		break;
	case CSI_CONFIG_ID_SENSOR_REAL_MCLK: {
			ret = csi_sensor_real_mclk;
		}
		break;
	case CSI_CONFIG_ID_FS_CNT: {
			ret  = fs_cnt;
		}
		break;

	case CSI_CONFIG_ID_ERR_CNT: {
			ret = csi_err_cnt[0];
		}
		break;

	default:
		DBG_WRN("CSI get Cfg ID Err\r\n");
		break;
	}

	return ret;
}

/**
    Wait the CSI interrupt event

    Wait the CSI specified interrupt event. When the interrupt event is triggered and match the wanted events,
    the waited event flag would be returned.

    @param[in] waited_flag  The bit-wise OR of the waited interrupt events.

    @return The waited interrupt events.
*/
CSI_INTERRUPT csi_wait_interrupt(CSI_INTERRUPT waited_flag)
{
#ifndef __KERNEL__
	FLGPTN              flag;
#endif
	UINT32              temp;
	T_CSI_INTEN0_REG  int0en;
#ifndef __KERNEL__
	ER                  tm_ret = E_SYS;
#else
	unsigned long       flags;
#endif

	if (csi_get_enable()) {
		// Check Supporting
		waited_flag = waited_flag & CSI_INTERRUP_ALL;
		if (!(waited_flag & CSI_INTERRUP_ALL)) {
			return 0x0;
		}

		// Lock
		if (csi_lock() != E_OK) {
			return 0x0;
		}

		int0en.reg = CSI_GETREG(CSI_INTEN0_REG_OFS);
		if ((waited_flag &  (CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4))) {
			if (int0en.bit.FS_GOT) {
				csi_set_int_disable(CSI_INT_BANK0, CSI_INTERRUPT_FS_GOT);
			}
		}

		// Clear Interrupt Status if the waited interrupt NOT enabled
		CSI_SETREG(CSI_INTSTS0_REG_OFS, ((waited_flag & 0xFFFF) & (~(CSI_GETREG(CSI_INTEN0_REG_OFS)))));
		CSI_SETREG(CSI_INTSTS1_REG_OFS, ((waited_flag & 0xF0000) & (~(CSI_GETREG(CSI_INTEN1_REG_OFS)))));

		// Clear waited flag
		csi_int_sts &= (~(waited_flag | CSI_INTERRUPT_ABORT));

		// This is race condition protection to prevent CSI disable activate just after status clear
		// and this may cause the user hang at frame end waiting.
		if (((waited_flag & (CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4))) && (csi_get_enable() == FALSE)) {
			csi_unlock();
			return 0x0;
		}

		// Set Interrupt Enable
		csi_set_int_enable(CSI_INT_BANK0, (waited_flag & (~(CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4))));
		if (waited_flag & CSI_INTERRUPT_FRAME_END) {
			csi_set_int_enable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END);
		}
		if (waited_flag & CSI_INTERRUPT_FRAME_END2) {
			csi_set_int_enable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END2);
		}
		if (waited_flag & CSI_INTERRUPT_FRAME_END3) {
			csi_set_int_enable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END3);
		}
		if (waited_flag & CSI_INTERRUPT_FRAME_END4) {
			csi_set_int_enable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END4);
		}

		waited_flag |= CSI_INTERRUPT_ABORT;

#if (DEBUG_MSG)
		DBG_DUMP("[%d], timeout = %d, waited_flag = 0x%x\r\n", __LINE__, csi_timeout_period, waited_flag);
#endif

		while (1) {
#ifndef __KERNEL__
			if (csi_timeout_period) {
				tm_ret = SW_TIMER_OPEN(&csi_timeout_timer_id, CSI_TIMEOUTCALLBACK);
				if (tm_ret == E_OK) {
					SW_TIMER_CFG(csi_timeout_timer_id, csi_timeout_period, SWTIMER_MODE_ONE_SHOT);
					SW_TIMER_START(csi_timeout_timer_id);
				}
			}
			wai_flg(&flag, FLG_ID_CSI, FLGPTN_CSI, TWF_ORW | TWF_CLR);


			if (csi_timeout_period) {
				if (tm_ret == E_OK) {
					SW_TIMER_STOP(csi_timeout_timer_id);
					SW_TIMER_CLOSE(csi_timeout_timer_id);
				}
			}
#else
			if (csi_timeout_period) {
				//init_completion(&csi_completion);
				//csi_platform_init_completion(MODULE_ID);
				csi_platform_reinit_completion(MODULE_ID);
				//if (!wait_for_completion_timeout(&csi_completion, msecs_to_jiffies(csi_timeout_period))) {
				if (!csi_platform_wait_completion_timeout(MODULE_ID, csi_timeout_period)) {
					loc_cpu(flags);
					csi_int_sts |= CSI_INTERRUPT_ABORT;
					unl_cpu(flags);
#if (DEBUG_MSG)
					DBG_DUMP("[%d]\r\n", __LINE__);
#endif
				}
			} else {
				//wait_for_completion(&csi_completion);
				csi_platform_wait_completion(MODULE_ID);
#if (DEBUG_MSG)
				DBG_DUMP("[%d]\r\n", __LINE__);
#endif
			}
#endif
#if (DEBUG_MSG)
			DBG_DUMP("[%d], csi_int_sts = 0x%x, waited_flag = 0x%x\r\n", __LINE__, csi_int_sts, waited_flag);
#endif

			temp = csi_int_sts;
			if (temp & waited_flag) {
				csi_int_sts &= (~waited_flag);

				csi_set_int_disable(CSI_INT_BANK0, (waited_flag & (~(CSI_INTERRUPT_FRAME_END | CSI_INTERRUPT_FRAME_END2 | CSI_INTERRUPT_FRAME_END3 | CSI_INTERRUPT_FRAME_END4))));
				if (waited_flag & CSI_INTERRUPT_FRAME_END) {
					csi_set_int_disable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END);
				}
				if (waited_flag & CSI_INTERRUPT_FRAME_END2) {
					csi_set_int_disable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END2);
				}
				if (waited_flag & CSI_INTERRUPT_FRAME_END3) {
					csi_set_int_disable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END3);
				}
				if (waited_flag & CSI_INTERRUPT_FRAME_END4) {
					csi_set_int_disable(CSI_INT_BANK1, CSI_INTERRUPT_FRAME_END4);
				}
				// Unlock
				csi_unlock();

				if (temp & CSI_INTERRUPT_ABORT) {
					DBG_ERR("TIMEOUT ABORT\r\n");
				}

				if (int0en.bit.FS_GOT) {
					csi_set_int_enable(CSI_INT_BANK0, CSI_INTERRUPT_FS_GOT);
				}
				//DBG_DUMP("[%d], int0en = 0x%x\r\n", __LINE__, CSI_GETREG(CSI_INTEN0_REG_OFS));

				return (temp & waited_flag);
			} else {
				//DBG_ERR("not temp and waited flag\r\n");
				//DBG_ERR("%d:  csi_int_sts = %d\r\n", __LINE__, csi_int_sts);
				// Unlock
				csi_unlock();
#if (DEBUG_MSG)
				DBG_DUMP("[%d], ~(temp & waited_flag)\r\n", __LINE__);
#endif
				if (int0en.bit.FS_GOT) {
					csi_set_int_enable(CSI_INT_BANK0, CSI_INTERRUPT_FS_GOT);
				}
				//DBG_DUMP("[%d], int0en = 0x%x\r\n", __LINE__, CSI_GETREG(CSI_INTEN0_REG_OFS));

				return 0;
			}
		}

	}

	DBG_ERR("Not EN\r\n");
	return 0x0;
}
#endif

//#ifndef __KERNEL__
BOOL csi_print_info_to_uart(CHAR *pstrcmd)
{
	T_CSI_RXDI_REG      reg_rxdi;
	T_CSI_RXSPKT0_REG   reg_rx_spkt0;
	T_CSI_COUNT_REG     reg_height;
	UINT32              temp1 = 0, temp2 = 0, /*temp3 = 0, temp4 = 0, temp5 = 0, temp6 = 0, temp7 = 0, temp8 = 0,*/ tempm1 = 0, tempm2 = 0;

	CSI_SETREG(CSI_INTSTS0_REG_OFS, CSI_INTERRUPT_FS_GOT | CSI_INTERRUPT_FE_GOT);

	reg_rxdi.reg = CSI_GETREG(CSI_RXDI_REG_OFS);

	temp1 = csi_get_config(CSI_CONFIG_ID_DLANE_NUMBER);
	if (temp1 == CSI_DATLANE_NUM_1) {
		temp1 = 1;
	} else if (temp1 == CSI_DATLANE_NUM_2) {
		temp1 = 2;
	} else if (temp1 == CSI_DATLANE_NUM_4) {
		temp1 = 4;
	}

	DBG_DUMP("CSI is %s. %d lanes\r\n", csi_get_enable() ? "ENABLE" : "DISABLE", (int)temp1);

	CSI_GET_PLLCLKFREQ(SIECLK_FREQ, &temp1);
	CSI_GET_PLLCLKFREQ(SIE2CLK_FREQ, &temp2);
	CSI_GET_PLLCLKFREQ(SIEMCLK_FREQ, &tempm1);
	CSI_GET_PLLCLKFREQ(SIEMCLK2_FREQ, &tempm2);
	DBG_DUMP("SIE1CLK=%d SIE2CLK=%d\r\n", (int)(temp1), (int)(temp2));
	//DBG_DUMP("SIE1CLK=%d SIE2CLK=%d SIE3CLK=%d SIE4CLK=%d\r\n", (int)(temp1), (int)(temp2), (int)(temp3), (int)(temp4));
	//DBG_DUMP("SIE5CLK=%d SIE6CLK=%d SIE7CLK=%d SIE8CLK=%d\r\n", (int)(temp5), (int)(temp6), (int)(temp7), (int)(temp8));
	DBG_DUMP("SIEMCLK=%d SIEMCLK2=%d\r\n", (int)(tempm1), (int)(tempm2));

	DBG_DUMP("AUTOMODE(EN%d)(T0x%02X) MANUAL1(EN%d)(T0x%02X)(F%d) MANUAL2(EN%d)(T0x%02X)(F%d) MANUAL3(EN%d)(T0x%02X)(F%d)\r\n",
			 (int)csi_get_config(CSI_CONFIG_ID_LPKT_AUTO), (unsigned int)reg_rxdi.bit.PKT_VALID_DI,
			 (int)csi_get_config(CSI_CONFIG_ID_LPKT_MANUAL), (unsigned int)csi_get_config(CSI_CONFIG_ID_MANUAL_DATA_ID), (int)((csi_get_config(CSI_CONFIG_ID_MANUAL_FORMAT) << 1) + 8),
			 (int)csi_get_config(CSI_CONFIG_ID_LPKT_MANUAL2), (unsigned int)csi_get_config(CSI_CONFIG_ID_MANUAL2_DATA_ID), (int)((csi_get_config(CSI_CONFIG_ID_MANUAL2_FORMAT) << 1) + 8),
			 (int)csi_get_config(CSI_CONFIG_ID_LPKT_MANUAL3), (unsigned int)csi_get_config(CSI_CONFIG_ID_MANUAL3_DATA_ID), (int)((csi_get_config(CSI_CONFIG_ID_MANUAL3_FORMAT) << 1) + 8));

	reg_height.reg  = CSI_GETREG(CSI_COUNT_REG_OFS);
	reg_rx_spkt0.reg = CSI_GETREG(CSI_RXSPKT0_REG_OFS);
	DBG_DUMP("WIDTH=%d-BYTES.. HEIGHT=%d\r\n", (int)(reg_rx_spkt0.bit.LINE_NUMBER), (int)(reg_height.bit.FRMEND_LINE_CNT));
	DBG_DUMP("DISABLE_SRC = FrmEnd%d\r\n", (int)(csi_get_config(CSI_CONFIG_ID_DISABLE_SOURCE) + 1));

	DBG_DUMP("HDR MODE is %s\r\n", csi_get_config(CSI_CONFIG_ID_CHID_MODE) ? "SONY_DOL" : "VIRTUAL-CH");

	DBG_DUMP("CHID1=%d, CHID2=%d, CHID3=%d, CHID4=%d\r\n", (int)(csi_get_config(CSI_CONFIG_ID_VIRTUAL_ID)), (int)(csi_get_config(CSI_CONFIG_ID_VIRTUAL_ID2)), (int)(csi_get_config(CSI_CONFIG_ID_VIRTUAL_ID3)), (int)(csi_get_config(CSI_CONFIG_ID_VIRTUAL_ID4)));
	DBG_DUMP("LI0=bit[%d] LI1=bit[%d] LI2=bit[%d] LI3=bit[%d]\r\n", (int)(csi_get_config(CSI_CONFIG_ID_LI_CHID_BIT0)), (int)(csi_get_config(CSI_CONFIG_ID_LI_CHID_BIT1)), (int)(csi_get_config(CSI_CONFIG_ID_LI_CHID_BIT2)), (int)(csi_get_config(CSI_CONFIG_ID_LI_CHID_BIT3)));
	DBG_DUMP("DL0 is D-%d-PN. DL1 is D-%d-PN. DL2 is D-%d-PN. DL3 is D-%d-PN\r\n\n", (int)(csi_get_config(CSI_CONFIG_ID_DATALANE0_PIN)), (int)(csi_get_config(CSI_CONFIG_ID_DATALANE1_PIN)), (int)(csi_get_config(CSI_CONFIG_ID_DATALANE2_PIN)), (int)(csi_get_config(CSI_CONFIG_ID_DATALANE3_PIN)));

#if 0
	for (temp1 = 0; temp1 < 16; temp1++) {
		for (temp2 = 0; temp2 < 16; temp2++) {

			reg_rxdi.reg = CSI_GETREG(CSI_RXDI_REG_OFS);
			DBG_DUMP("0x%02X ", reg_rxdi.bit.PKT_VALID_DI);

		}
		DBG_DUMP("\r\n");
	}
#endif

	DBG_DUMP("STS0=0x%08X STS1=0x%08X\r\n", (unsigned int)CSI_GETREG(CSI_INTSTS0_REG_OFS), (unsigned int)CSI_GETREG(CSI_INTSTS1_REG_OFS));
	if (CSI_GETREG(CSI_INTSTS0_REG_OFS) & (CSI_INTERRUPT_FS_GOT | CSI_INTERRUPT_FE_GOT)) {

		csi_wait_interrupt(CSI_INTERRUPT_FS_GOT);
		//PERF_MARK();

		for (temp1 = 0; temp1 < 10; temp1++) {
			DBG_DUMP(".");
			csi_wait_interrupt(CSI_INTERRUPT_FS_GOT);
		}
		//DBG_DUMP("One frame time is %d us\r\n", (int)(PERF_GETDURATION()/10));
	}

	CSI_SETREG(CSI_INTSTS0_REG_OFS, CSI_GETREG(CSI_INTSTS0_REG_OFS) & (~CSI_GETREG(CSI_INTEN0_REG_OFS)));
	CSI_SETREG(CSI_INTSTS1_REG_OFS, CSI_GETREG(CSI_INTSTS1_REG_OFS) & (~CSI_GETREG(CSI_INTEN1_REG_OFS)));

	return TRUE;
}
//#endif
#ifdef __KERNEL__
EXPORT_SYMBOL(csi_print_info_to_uart);
#endif
//@}

