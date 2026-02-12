/*
    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver

    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver

    @file       senphy.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifdef __KERNEL__
#include <mach/rcw_macro.h>
#include <kwrap/type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include "../senphy.h"
#include "senphy_int.h"
#include "senphy_dbg.h"
#include "rule_check.h"
#else
#include <kwrap/nvt_type.h>
#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#include "senphy_int.h"
#include "rule_check.h"

#define __MODULE__    rtos_senphy
#include <kwrap/debug.h>
unsigned int rtos_senphy_debug_level = NVT_DBG_WRN;
#endif


static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)


#ifdef _BSP_NA51055_
/* 520 */

static BOOL             sen_phy_en = 0x00000000;
static BOOL             sen_power_en = 0x00000000;
static UINT8            lane_select[SENPHY_MODULES_NUMBER];//For each module, which data lanes are selected.

#ifdef SENPHY_API_DBG
static BOOL             senphy_phy_dbg_msg = ENABLE;
#else
static BOOL             senphy_phy_dbg_msg = DISABLE;
#endif

static void senphy_gating_contorl(BOOL enable)
{
	#ifndef __KERNEL__
	static BOOL mipi1,mipi2;

	if(enable == DISABLE) {

		mipi1 = pll_get_pclk_auto_gating(MIPI_LVDS_GCLK);
		mipi2 = pll_get_pclk_auto_gating(MIPI_LVDS2_GCLK);

		if (mipi1) {
			pll_clear_pclk_auto_gating(MIPI_LVDS_GCLK);
		}
		if (mipi2) {
			pll_clear_pclk_auto_gating(MIPI_LVDS2_GCLK);
		}
	} else {

		if (mipi1) {
			pll_set_pclk_auto_gating(MIPI_LVDS_GCLK);
		}
		if (mipi2) {
			pll_set_pclk_auto_gating(MIPI_LVDS2_GCLK);
		}
	}
	#else
	static BOOL mipi1,mipi2;

	if(enable == DISABLE) {

	  mipi1 = ((*(UINT32 *)(0xFD0200C4))>>3)& 0x1;
	  mipi2 = ((*(UINT32 *)(0xFD0200C4))>>4)& 0x1;

	  if (mipi1) {
		iowrite32(ioread32((void *)(0xFD0200C4))& (~(0x1 << 3)), (void *)(0xFD0200C4));
	  }
	  if (mipi2) {
		iowrite32(ioread32((void *)(0xFD0200C4))& (~(0x1 << 4)), (void *)(0xFD0200C4));
	  }
	} else {
	  if (mipi1) {
		iowrite32(ioread32((void *)(0xFD0200C4))| ((0x1 << 3)), (void *)(0xFD0200C4));
	  }
	  if (mipi2) {
		iowrite32(ioread32((void *)(0xFD0200C4))| ((0x1 << 4)), (void *)(0xFD0200C4));
	  }
	}
	#endif

}

static void senphy_reset(SENPHY_SEL phy_select, BOOL reset)
{
	T_SENPHY_CTRL3_REG  PhyCtl3;
	unsigned long      flags;

	senphy_gating_contorl(DISABLE);

	loc_cpu(flags);
	PhyCtl3.reg = SENPHY_GETREG(SENPHY_CTRL3_REG_OFS);

	if ((lane_select[phy_select] & 0x03) || (phy_select == SENPHY_SEL_MIPILVDS)) {
		PhyCtl3.reg &= ~(0x1 << 0);
		PhyCtl3.reg |= ((reset > 0) << 0);

		#ifndef _SENPHY_TODO_
		if (phy_select == SENPHY_SEL_MIPILVDS) {
			if (reset) {
				pll_disable_clock(MIPI_LVDS_PHYD4_CLK);
			} else {
				pll_enable_clock(MIPI_LVDS_PHYD4_CLK);
			}
		}
		#else
		if (phy_select == SENPHY_SEL_MIPILVDS) {
			if (reset) {
				iowrite32(ioread32((void *)(0xFD020078))& (~(0x1 << 30)), (void *)(0xFD020078));
			} else {
				iowrite32(ioread32((void *)(0xFD020078))|(0x1 << 30), (void *)(0xFD020078));
			}
		}
		#endif

	}

	if (lane_select[phy_select] & 0x0C) {
		PhyCtl3.reg &= ~(0x1 << 1);
		PhyCtl3.reg |= ((reset > 0) << 1);

		#ifndef _SENPHY_TODO_
		if (phy_select == SENPHY_SEL_MIPILVDS) {
			if (reset) {
				pll_disable_clock(MIPI_LVDS_PHYD4_CLK2);
			} else {
				pll_enable_clock(MIPI_LVDS_PHYD4_CLK2);
			}
		} else if (phy_select == SENPHY_SEL_MIPILVDS2) {
			if (reset) {
				pll_disable_clock(MIPI_LVDS2_PHYD4_CLK);
			} else {
				pll_enable_clock(MIPI_LVDS2_PHYD4_CLK);
			}
		}
		#else
		if (phy_select == SENPHY_SEL_MIPILVDS) {
			if (reset) {
				iowrite32(ioread32((void *)(0xFD020078))& (~(0x1 << 29)), (void *)(0xFD020078));
			} else {
				iowrite32(ioread32((void *)(0xFD020078))|(0x1 << 29), (void *)(0xFD020078));
			}
		} else if (phy_select == SENPHY_SEL_MIPILVDS2) {
			if (reset) {
				iowrite32(ioread32((void *)(0xFD020078))& (~(0x1 << 31)), (void *)(0xFD020078));
			} else {
				iowrite32(ioread32((void *)(0xFD020078))|(0x1 << 31), (void *)(0xFD020078));
			}
		}
		#endif

	}

#if 1
	SENPHY_SETREG(SENPHY_CTRL3_REG_OFS, PhyCtl3.reg);
#else
	SENPHY_SETREG(SENPHY_CTRL3_REG_OFS, 0);
#endif
	unl_cpu(flags);

	senphy_gating_contorl(ENABLE);
}



#if 1
#endif

/*
    Set Sensor PHY Default init

    Set Sensor PHY Default init. This is used in the drv_init().
*/
void senphy_init(void)
{
	static BOOL init;
#ifndef _SENPHY_TODO_
#ifdef __KERNEL__
	struct clk *parent;
	struct clk *csi_clk;
	unsigned long parent_rate = 0;
#endif
#endif


	if (!init) {

		senphy_gating_contorl(DISABLE);

		#ifndef __KERNEL__
		OUTW(0xF003007C, 0x30);
		#else
		iowrite32(0x30, (void *)(0xFD03007C));
		#endif

		// Default set all data lanes to reset asserted.
		SENPHY_SETREG(SENPHY_CTRL3_REG_OFS,         0x3);

#ifndef  _SENPHY_TODO_
		pll_disable_clock(MIPI_LVDS_PHYD4_CLK);
		pll_disable_clock(MIPI_LVDS_PHYD4_CLK2);
		pll_disable_clock(MIPI_LVDS2_PHYD4_CLK);

		#ifdef  _NVT_FPGA_
		// During FPGA, Set 120MHz means MIPI-CLK as 24MHz.
		PLL_SET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS, PLL_CLKSEL_MIPI_LVDS_120);
		#endif

#else
		iowrite32(ioread32((void *)(0xFD020078)) & (~(0x7 << 29)), (void *)(0xFD020078));

		#ifdef  _NVT_FPGA_
		iowrite32(ioread32((void *)(0xFD020020)) | (0x9 << 12), (void *)(0xFD020020));
		#endif
#endif

		senphy_gating_contorl(ENABLE);


#ifdef __KERNEL__
		#ifndef _SENPHY_TODO_
		csi_clk = clk_get(NULL, "f0280000.csi");
		parent = clk_get_parent(csi_clk);
		parent_rate = clk_get_rate(parent);
		if (parent_rate == 60000000) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  8);
		} else if (parent_rate == 120000000) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  20);
		} else {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  11);
		}
		#else
		senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  20);
		#endif
#else
		if (PLL_GET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  8);
		} else if (PLL_GET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  20);
		} else {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  11);
		}
#endif
		init = 1;
	}
}

/*

*/
ER senphy_set_power(SENPHY_SEL phy_select, BOOL b_enable)
{
	BOOL                cur_en, sts_en;
	T_SENPHY_CTRL0_REG  phy_ctl0;
	unsigned long       flags;

	if (b_enable) {
		if (sen_power_en & (0x1 << phy_select)) {
			DBG_WRN("Already Power ON. %d\r\n", phy_select);
			return E_OBJ;
		}
	} else {
		if (!(sen_power_en & (0x1 << phy_select))) {
			DBG_WRN("Already Power OFF. %d\r\n", phy_select);
			return E_OBJ;
		}
	}

	senphy_gating_contorl(DISABLE);

	loc_cpu(flags);
	if (b_enable) {
		sen_power_en |= (0x1 << phy_select);
	} else {
		sen_power_en &= ~(0x1 << phy_select);
	}

	phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
	sts_en = phy_ctl0.bit.EN_BIAS;
	cur_en = (sen_power_en > 0);
	if (sts_en != cur_en) {
		phy_ctl0.bit.EN_BIAS = cur_en;
		SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
	}
	unl_cpu(flags);
	senphy_gating_contorl(ENABLE);
	return E_OK;
}

/*

*/
ER senphy_enable(SENPHY_SEL phy_select, BOOL b_enable)
{
	unsigned long       flags;

	if (b_enable) {
		if (sen_phy_en & (0x1 << phy_select)) {
			DBG_WRN("Already enable. %d\r\n", phy_select);
			return E_OBJ;
		}
	}

	senphy_reset(phy_select, !b_enable);

	loc_cpu(flags);
	if (b_enable) {
		sen_phy_en |= (0x1 << phy_select);
	} else {
		sen_phy_en &= ~(0x1 << phy_select);
	}
	unl_cpu(flags);

	return E_OK;
}

/*

*/
ER senphy_set_valid_lanes(SENPHY_SEL phy_select, SENPHY_DATASEL data_select)
{
	if (phy_select >= SENPHY_MODULES_NUMBER) {
		DBG_ERR("No this module %d\r\n", phy_select);
		return E_ID;
	}

	senphy_debug(("[%d]=0x%03X\r\n", phy_select, data_select));

	lane_select[phy_select] = data_select;
	return E_OK;
}

/*

*/
ER senphy_set_clock_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select)
{
	return E_OK;
}


/*
    This is used for LVDS0 modules only
*/
ER senphy_set_clock2_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select)
{
	return E_OK;
}


#if 1
#endif

/**
    Set LVDS/MIPI-CSI Sensor PHY Configurations

    This is Driver internal API to LVDS/MIPI-CSI modules usages.
    No exporting to global.
*/
ER senphy_set_config(SENPHY_CONFIG_ID config_id, UINT32 config_value)
{
	unsigned long       flags;

	senphy_gating_contorl(DISABLE);
	loc_cpu(flags);

	switch (config_id) {
	case SENPHY_CONFIG_ID_ENO_DLY: {
			T_SENPHY_CTRL2_REG phy_ctl2;

			phy_ctl2.reg = SENPHY_GETREG(SENPHY_CTRL2_REG_OFS);
			phy_ctl2.bit.CLKENO_DLY = config_value;
			SENPHY_SETREG(SENPHY_CTRL2_REG_OFS, phy_ctl2.reg);
		}
		break;


	case SENPHY_CONFIG_ID_DLY_EN: {
			T_SENPHY_CTRL0_REG  phy_ctl0;

			phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			phy_ctl0.bit.DLY_EN = config_value > 0;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
		}
		break;

	case SENPHY_CONFIG_ID_DLY_CLK0:
	case SENPHY_CONFIG_ID_DLY_CLK1: {
			T_SENPHY_CTRL4_REG phy_ctl4;

			phy_ctl4.reg = SENPHY_GETREG(SENPHY_CTRL4_REG_OFS);
			phy_ctl4.reg &= ~(0x7 << ((config_id - SENPHY_CONFIG_ID_DLY_CLK0) << 2));
			phy_ctl4.reg |= ((config_value & 0x7) << ((config_id - SENPHY_CONFIG_ID_DLY_CLK0) << 2));
			SENPHY_SETREG(SENPHY_CTRL4_REG_OFS, phy_ctl4.reg);
		}
		break;

	case SENPHY_CONFIG_ID_DLY_DAT0:
	case SENPHY_CONFIG_ID_DLY_DAT1:
	case SENPHY_CONFIG_ID_DLY_DAT2:
	case SENPHY_CONFIG_ID_DLY_DAT3: {
			T_SENPHY_CTRL5_REG phy_ctl5;

			phy_ctl5.reg = SENPHY_GETREG(SENPHY_CTRL5_REG_OFS);
			phy_ctl5.reg &= ~(0x7 << ((config_id - SENPHY_CONFIG_ID_DLY_DAT0) << 2));
			phy_ctl5.reg |= ((config_value & 0x7) << ((config_id - SENPHY_CONFIG_ID_DLY_DAT0) << 2));
			SENPHY_SETREG(SENPHY_CTRL5_REG_OFS, phy_ctl5.reg);
		}
		break;

	case SENPHY_CONFIG_ID_IADJ: {
			T_SENPHY_CTRL0_REG  phy_ctl0;

			phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			phy_ctl0.bit.IADJ = config_value;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
		}
		break;

	case SENPHY_CONFIG_ID_CURRDIV2: {
		}
		break;

	case SENPHY_CONFIG_ID_INV_DAT0:
	case SENPHY_CONFIG_ID_INV_DAT1:
	case SENPHY_CONFIG_ID_INV_DAT2:
	case SENPHY_CONFIG_ID_INV_DAT3: {
			T_SENPHY_CTRL6_REG phy_ctl6;

			phy_ctl6.reg = SENPHY_GETREG(SENPHY_CTRL6_REG_OFS);
			phy_ctl6.reg &= ~(0x1 << (16 + config_id - SENPHY_CONFIG_ID_INV_DAT0));
			phy_ctl6.reg |= ((config_value > 0) << (16 + config_id - SENPHY_CONFIG_ID_INV_DAT0));
			SENPHY_SETREG(SENPHY_CTRL6_REG_OFS, phy_ctl6.reg);
		}
		break;

	case SENPHY_CONFIG_CK1_EN: {
			T_SENPHY_CTRL0_REG  PhyCtl0;

			PhyCtl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			PhyCtl0.bit.PHY_CK_MODE = config_value > 0;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, PhyCtl0.reg);
		}
		break;

	case SENPHY_CONFIG_CSI_MODE: {
			T_SENPHY_MODE_REG reg_mode;

			reg_mode.reg = SENPHY_GETREG(SENPHY_MODE_REG_OFS);
			reg_mode.bit.CSI_MODE = config_value > 0;
			SENPHY_SETREG(SENPHY_MODE_REG_OFS, reg_mode.reg);
		}
		break;
	case SENPHY_CONFIG_CSI2_MODE: {
			T_SENPHY_MODE_REG reg_mode;

			reg_mode.reg = SENPHY_GETREG(SENPHY_MODE_REG_OFS);
			reg_mode.bit.CSI2_MODE = config_value > 0;
			SENPHY_SETREG(SENPHY_MODE_REG_OFS, reg_mode.reg);
		}
		break;
	case SENPHY_CONFIG_LVDS_MODE: {
			T_SENPHY_MODE_REG reg_mode;

			reg_mode.reg = SENPHY_GETREG(SENPHY_MODE_REG_OFS);
			reg_mode.bit.LVDS_MODE = config_value > 0;
			SENPHY_SETREG(SENPHY_MODE_REG_OFS, reg_mode.reg);
		}
		break;
	case SENPHY_CONFIG_LVDS2_MODE: {
			T_SENPHY_MODE_REG reg_mode;

			reg_mode.reg = SENPHY_GETREG(SENPHY_MODE_REG_OFS);
			reg_mode.bit.LVDS2_MODE = config_value > 0;
			SENPHY_SETREG(SENPHY_MODE_REG_OFS, reg_mode.reg);
		}
		break;


	case SENPHY_CONFIG_ID_DBGMSG: {
			senphy_phy_dbg_msg = (config_value > 0);
		}
		break;

	default:
		break;

	}

	unl_cpu(flags);
	senphy_gating_contorl(ENABLE);
	return E_OK;
}

#else
/**************************** 680 ***********************************************/

static BOOL             sen_phy_en = 0x00000000;
static BOOL             sen_power_en = 0x00000000;
static UINT8            lane_select[SENPHY_MODULES_NUMBER];//For each module, which data lanes are selected.
static SENPHY_CLKMAP    sen_clock_map[SENPHY_MODULES_NUMBER];//For each module, which clock lane is selected.
static SENPHY_CLKMAP    sen_clock_map2[1] = {SENPHY_CLKMAP_OFF};

static const UINT8      sen_valid_clk_map[SENPHY_DATALANE_NUMBER] = { //For each data lane, Can map to which clock lane.
	0x5, 0x7, 0x5, 0xD, 0x51, 0x71, 0x51, 0xD1
};

#ifdef SENPHY_API_DBG
static BOOL             senphy_phy_dbg_msg = ENABLE;
#else
static BOOL             senphy_phy_dbg_msg = DISABLE;
#endif

static BOOL senphy_validate_lane_map(SENPHY_SEL phy_select)
{
	T_SENPHY_CTRL1_REG  phy_ctl1;
	T_SENPHY_CTRL3_REG  phy_ctl3;
	UINT8               dl, mod;
	unsigned long      flags;


	loc_cpu(flags);
	phy_ctl1.reg = SENPHY_GETREG(SENPHY_CTRL1_REG_OFS);
	phy_ctl3.reg = SENPHY_GETREG(SENPHY_CTRL3_REG_OFS);

	for (dl = 0; dl < SENPHY_DATALANE_NUMBER; dl++) {
		if (lane_select[phy_select] & (0x1 << dl)) {
			// Check if data lane is already used by other module or not
			for (mod = 0; mod < SENPHY_MODULES_NUMBER; mod++) {
				if ((mod != phy_select) && (lane_select[mod] & (0x1 << dl)) && (sen_phy_en & (0x1 << mod))) {
					unl_cpu(flags);
					DBG_ERR("DataLane-%d is used by MIPILVDS-%d\r\n", dl, mod);
					return TRUE;
				}
			}

			// Check the selected data lane to clock lane map is legal or not.
			if ((phy_select == SENPHY_SEL_MIPILVDS) && (dl >= 4) && (sen_clock_map2[phy_select] != SENPHY_CLKMAP_OFF)) {
				if (!(sen_valid_clk_map[dl] & (0x1 << sen_clock_map2[phy_select]))) {
					unl_cpu(flags);
					DBG_ERR("MIPILVDS%d DataLane-%d Map to CLK-%d illegal\r\n", phy_select + 1, dl, sen_clock_map2[phy_select]);
					goto errret;
				} else {
					if (senphy_phy_dbg_msg) {
						unl_cpu(flags);
						DBG_DUMP("MIPILVDS%d DataLane-%d Map to CLK-%d OK\r\n", phy_select + 1, dl, sen_clock_map2[phy_select]);
						loc_cpu(flags);
					}

					phy_ctl1.reg &= ~(0x7 << (dl << 2));
					phy_ctl1.reg |= (sen_clock_map2[phy_select] << (dl << 2));
					phy_ctl3.reg &= ~(0x1 << dl);

					// Selected Clock lane's Data lane shall also release reset
					phy_ctl3.reg &= ~(0x1 << sen_clock_map2[phy_select]);
					phy_ctl1.reg &= ~(0x7 << (sen_clock_map2[phy_select] << 2));
					phy_ctl1.reg |= (sen_clock_map2[phy_select] << (sen_clock_map2[phy_select] << 2));
				}
			} else {
				if (!(sen_valid_clk_map[dl] & (0x1 << sen_clock_map[phy_select]))) {
					unl_cpu(flags);
					DBG_ERR("MIPILVDS%d DataLane-%d Map to CLK-%d illegal\r\n", phy_select + 1, dl, sen_clock_map[phy_select]);
					goto errret;
				} else {
					if (senphy_phy_dbg_msg) {
						unl_cpu(flags);
						DBG_DUMP("MIPILVDS%d DataLane-%d Map to CLK-%d OK\r\n", phy_select + 1, dl, sen_clock_map[phy_select]);
						loc_cpu(flags);
					}

					phy_ctl1.reg &= ~(0x7 << (dl << 2));
					phy_ctl1.reg |= (sen_clock_map[phy_select] << (dl << 2));
					phy_ctl3.reg &= ~(0x1 << dl);

					// Selected Clock lane's Data lane shall also release reset
					phy_ctl3.reg &= ~(0x1 << sen_clock_map[phy_select]);
					phy_ctl1.reg &= ~(0x7 << (sen_clock_map[phy_select] << 2));
					phy_ctl1.reg |= (sen_clock_map[phy_select] << (sen_clock_map[phy_select] << 2));

					if (phy_select == SENPHY_SEL_MIPILVDS) {
						// MIPILVDS1 fixed using the CK0. must be open
						phy_ctl3.reg &= ~(0x1 << 0);
						phy_ctl1.reg &= ~(0x7 << 0);
						phy_ctl1.reg |= (sen_clock_map[phy_select] << 0);
					} else if (phy_select == SENPHY_SEL_MIPILVDS2) {
						// MIPILVDS2 fixed using the CK4. must be open
						phy_ctl3.reg &= ~(0x1 << 4);
						phy_ctl1.reg &= ~(0x7 << 16);
						phy_ctl1.reg |= (sen_clock_map[phy_select] << 16);
					} else if (phy_select == SENPHY_SEL_MIPILVDS3) {
						// MIPILVDS3 fixed using the CK2. must be open
						phy_ctl3.reg &= ~(0x1 << 2);
						phy_ctl1.reg &= ~(0x7 << 8);
						phy_ctl1.reg |= (sen_clock_map[phy_select] << 8);
					} else if (phy_select == SENPHY_SEL_MIPILVDS4) {
						// MIPILVDS4 fixed using the CK6. must be open
						phy_ctl3.reg &= ~(0x1 << 6);
						phy_ctl1.reg &= ~(0x7 << 24);
						phy_ctl1.reg |= (sen_clock_map[phy_select] << 24);
					}
				}
			}


		}
	}

	// Validate the mapping.
	SENPHY_SETREG(SENPHY_CTRL1_REG_OFS, phy_ctl1.reg);
#if 0
	SENPHY_SETREG(SENPHY_CTRL3_REG_OFS, 0x00);
#else
	SENPHY_SETREG(SENPHY_CTRL3_REG_OFS, phy_ctl3.reg);
#endif
	unl_cpu(flags);

	return FALSE;

errret:
	return TRUE;

}


static void senphy_reset(SENPHY_SEL phy_select, BOOL reset)
{
	T_SENPHY_CTRL3_REG  phy_ctl3;
	UINT8               dl;
	unsigned long      flags;

	loc_cpu(flags);

	SENPHYECO_SETREG(0, SENPHYECO_GETREG(0)|(0x1<<9));

	phy_ctl3.reg = SENPHY_GETREG(SENPHY_CTRL3_REG_OFS);

	for (dl = 0; dl < SENPHY_DATALANE_NUMBER; dl++) {
		if (lane_select[phy_select] & (0x1 << dl)) {
			phy_ctl3.reg &= ~(0x1 << dl);
			phy_ctl3.reg |= ((reset > 0) << dl);
		}
	}

	if (sen_clock_map[phy_select] != SENPHY_CLKMAP_OFF) {
		phy_ctl3.reg &= ~(0x1 << sen_clock_map[phy_select]);
		phy_ctl3.reg |= ((reset > 0) << sen_clock_map[phy_select]);
	}
	if ((phy_select == SENPHY_SEL_MIPILVDS) && (sen_clock_map2[phy_select] != SENPHY_CLKMAP_OFF)) {
		phy_ctl3.reg &= ~(0x1 << sen_clock_map2[phy_select]);
		phy_ctl3.reg |= ((reset > 0) << sen_clock_map2[phy_select]);
	}

	SENPHY_SETREG(SENPHY_CTRL3_REG_OFS, phy_ctl3.reg);
	unl_cpu(flags);

}



#if 1
#endif

/*
    Set Sensor PHY Default init

    Set Sensor PHY Default init. This is used in the drv_init().
*/
void senphy_init(void)
{
	static BOOL init;

	if (!init) {

		// Default set all data lanes to reset asserted.
		SENPHY_SETREG(SENPHY_CTRL3_REG_OFS,         0xFF);

		#ifdef  _NVT_FPGA_
		// During FPGA, Set 120MHz means MIPI-CLK as 24MHz.
		PLL_SET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS, PLL_CLKSEL_MIPI_LVDS_120);
		#endif

		if (PLL_GET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  8);
		} else if (PLL_GET_CLOCK_RATE(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  20);
		} else {
			senphy_set_config(SENPHY_CONFIG_ID_ENO_DLY,  11);
		}

		init = 1;
	}
}

/*

*/
ER senphy_set_power(SENPHY_SEL phy_select, BOOL b_enable)
{
	BOOL                cur_en, sts_en;
	T_SENPHY_CTRL0_REG  phy_ctl0;
	unsigned long      flags;

	if (b_enable) {
		if (sen_power_en & (0x1 << phy_select)) {
			DBG_WRN("Already Power ON. %d\r\n", phy_select);
			return E_OBJ;
		}
	} else {
		if (!(sen_power_en & (0x1 << phy_select))) {
			DBG_WRN("Already Power OFF. %d\r\n", phy_select);
			return E_OBJ;
		}
	}

	if (b_enable) {
		loc_cpu(flags);
		sen_power_en |= (0x1 << phy_select);
	} else {
		loc_cpu(flags);
		sen_power_en &= ~(0x1 << phy_select);
	}

	phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
	sts_en = phy_ctl0.bit.EN_BIAS;
	cur_en = (sen_power_en > 0);
	if (sts_en != cur_en) {
		phy_ctl0.bit.EN_BIAS = cur_en;
		SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
	}
	unl_cpu(flags);
	return E_OK;
}

/*

*/
ER senphy_enable(SENPHY_SEL phy_select, BOOL b_enable)
{
	unsigned long      flags;

	if (b_enable) {
		if (sen_phy_en & (0x1 << phy_select)) {
			DBG_WRN("Already enable. %d\r\n", phy_select);
			return E_OBJ;
		}
	}

	if (b_enable) {
		// Set Clock Map & Release Reset
		if (senphy_validate_lane_map(phy_select)) {
			return E_OACV;
		}

		loc_cpu(flags);
		sen_phy_en |= (0x1 << phy_select);
	} else {
		// Reset bit asserted
		senphy_reset(phy_select, TRUE);

		loc_cpu(flags);
		sen_phy_en &= ~(0x1 << phy_select);
	}

	unl_cpu(flags);
	return E_OK;
}

/*

*/
ER senphy_set_valid_lanes(SENPHY_SEL phy_select, SENPHY_DATASEL data_select)
{
	if (phy_select >= SENPHY_MODULES_NUMBER) {
		DBG_ERR("No this module %d\r\n", phy_select);
		return E_ID;
	}

	senphy_debug(("[%d]=0x%03X\r\n", phy_select, data_select));

	lane_select[phy_select] = data_select;
	return E_OK;
}

/*

*/
ER senphy_set_clock_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select)
{
	if (phy_select >= SENPHY_MODULES_NUMBER) {
		DBG_ERR("No this module %d\r\n", phy_select);
		return E_ID;
	}

	sen_clock_map[phy_select] = clk_select;
	return E_OK;
}


/*
    This is used for LVDS0 modules only
*/
ER senphy_set_clock2_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select)
{
	if (phy_select != SENPHY_SEL_MIPILVDS) {
		DBG_ERR("valid for LVDS0 only.\r\n");
		return E_OACV;
	}

	sen_clock_map2[phy_select] = clk_select;
	return E_OK;
}


#if 1
#endif

/**
    Set LVDS/MIPI-CSI Sensor PHY Configurations

    This is Driver internal API to LVDS/MIPI-CSI modules usages.
    No exporting to global.
*/
ER senphy_set_config(SENPHY_CONFIG_ID config_id, UINT32 config_value)
{
	unsigned long      flags;

	loc_cpu(flags);

	switch (config_id) {
	case SENPHY_CONFIG_ID_ENO_DLY: {
			T_SENPHY_CTRL2_REG phy_ctl2;

			phy_ctl2.reg = SENPHY_GETREG(SENPHY_CTRL2_REG_OFS);
			phy_ctl2.bit.CLKENO_DLY = config_value;
			SENPHY_SETREG(SENPHY_CTRL2_REG_OFS, phy_ctl2.reg);
		}
		break;


	case SENPHY_CONFIG_ID_DLY_EN: {
			T_SENPHY_CTRL0_REG  phy_ctl0;

			phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			phy_ctl0.bit.DLY_EN = config_value > 0;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
		}
		break;

	case SENPHY_CONFIG_ID_DLY_CLK0:
	case SENPHY_CONFIG_ID_DLY_CLK1:
	case SENPHY_CONFIG_ID_DLY_CLK2:
	case SENPHY_CONFIG_ID_DLY_CLK3:
	case SENPHY_CONFIG_ID_DLY_CLK4:
	case SENPHY_CONFIG_ID_DLY_CLK5:
	case SENPHY_CONFIG_ID_DLY_CLK6:
	case SENPHY_CONFIG_ID_DLY_CLK7: {
			T_SENPHY_CTRL4_REG phy_ctl4;

			phy_ctl4.reg = SENPHY_GETREG(SENPHY_CTRL4_REG_OFS);
			phy_ctl4.reg &= ~(0x7 << ((config_id - SENPHY_CONFIG_ID_DLY_CLK0) << 2));
			phy_ctl4.reg |= ((config_value & 0x7) << ((config_id - SENPHY_CONFIG_ID_DLY_CLK0) << 2));
			SENPHY_SETREG(SENPHY_CTRL4_REG_OFS, phy_ctl4.reg);
		}
		break;

	case SENPHY_CONFIG_ID_DLY_DAT0:
	case SENPHY_CONFIG_ID_DLY_DAT1:
	case SENPHY_CONFIG_ID_DLY_DAT2:
	case SENPHY_CONFIG_ID_DLY_DAT3:
	case SENPHY_CONFIG_ID_DLY_DAT4:
	case SENPHY_CONFIG_ID_DLY_DAT5:
	case SENPHY_CONFIG_ID_DLY_DAT6:
	case SENPHY_CONFIG_ID_DLY_DAT7: {
			T_SENPHY_CTRL5_REG phy_ctl5;

			phy_ctl5.reg = SENPHY_GETREG(SENPHY_CTRL5_REG_OFS);
			phy_ctl5.reg &= ~(0x7 << ((config_id - SENPHY_CONFIG_ID_DLY_DAT0) << 2));
			phy_ctl5.reg |= ((config_value & 0x7) << ((config_id - SENPHY_CONFIG_ID_DLY_DAT0) << 2));
			SENPHY_SETREG(SENPHY_CTRL5_REG_OFS, phy_ctl5.reg);
		}
		break;

	case SENPHY_CONFIG_ID_IADJ: {
			T_SENPHY_CTRL0_REG  phy_ctl0;

			phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			phy_ctl0.bit.IADJ = config_value;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
		}
		break;

	case SENPHY_CONFIG_ID_CURRDIV2: {
			T_SENPHY_CTRL0_REG  phy_ctl0;

			phy_ctl0.reg = SENPHY_GETREG(SENPHY_CTRL0_REG_OFS);
			phy_ctl0.bit.CURR_DIV2 = config_value > 0;
			SENPHY_SETREG(SENPHY_CTRL0_REG_OFS, phy_ctl0.reg);
		}
		break;

	case SENPHY_CONFIG_ID_INV_DAT0:
	case SENPHY_CONFIG_ID_INV_DAT1:
	case SENPHY_CONFIG_ID_INV_DAT2:
	case SENPHY_CONFIG_ID_INV_DAT3:
	case SENPHY_CONFIG_ID_INV_DAT4:
	case SENPHY_CONFIG_ID_INV_DAT5:
	case SENPHY_CONFIG_ID_INV_DAT6:
	case SENPHY_CONFIG_ID_INV_DAT7: {
			T_SENPHY_CTRL6_REG phy_ctl6;

			phy_ctl6.reg = SENPHY_GETREG(SENPHY_CTRL6_REG_OFS);
			phy_ctl6.reg &= ~(0x1 << (16 + config_id - SENPHY_CONFIG_ID_INV_DAT0));
			phy_ctl6.reg |= ((config_value > 0) << (16 + config_id - SENPHY_CONFIG_ID_INV_DAT0));
			SENPHY_SETREG(SENPHY_CTRL6_REG_OFS, phy_ctl6.reg);
		}
		break;

	case SENPHY_CONFIG_ID_DBGMSG: {
			senphy_phy_dbg_msg = (config_value > 0);
		}
		break;

	default:
		break;

	}

	unl_cpu(flags);
	return E_OK;
}




#endif



#ifdef __KERNEL__
EXPORT_SYMBOL(senphy_init);
EXPORT_SYMBOL(senphy_set_power);
EXPORT_SYMBOL(senphy_enable);
EXPORT_SYMBOL(senphy_set_valid_lanes);
EXPORT_SYMBOL(senphy_set_clock_map);
EXPORT_SYMBOL(senphy_set_clock2_map);
EXPORT_SYMBOL(senphy_set_config);
#endif

