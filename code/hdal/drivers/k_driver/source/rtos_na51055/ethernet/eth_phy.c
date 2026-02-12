/*
    Ethernet module driver

    This file is the PHY (RTL8201E) in ethernet

    @file       Eth_phy.c
    @ingroup    mIDrvIO_Eth
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#include <string.h>
//#include "SysKer.h"
//#include "ethernet_protected.h"
#include "ethernet_reg.h"
#include "ethernet_int.h"
#include "eth_gmac.h"
#include "eth_phy.h"
#include "eth_platform.h"
//#include "Utility.h"

//
// Internal prototypes
//

//
// Internal Definitions
//
static BOOL bOpened = FALSE;
static EXTPHY_CONTEXT extPhyContext = {0};
static UINT32 uiPhyADDR = ETH_PHY_PHYADDR;
static BOOL is_first = 0;

static UINT32 phy_rst_byps=0;

/*
    open external phy

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_open(void)
{
	UINT32 flags = 0;
	flags = eth_platform_spin_lock();
	if (bOpened == TRUE) {
		eth_platform_spin_unlock(flags);
		DBG_ERR("external PHY already opened\r\n");

		return E_SYS;
	}
	bOpened = TRUE;

	eth_platform_spin_unlock(flags);

	//memset(&extPhyContext, 0, sizeof(extPhyContext));

	if (ethPHY_verifyID() == FALSE) {
		DBG_ERR("open fail\r\n");
		flags = eth_platform_spin_lock();
		bOpened = FALSE;
		eth_platform_spin_unlock(flags);

		return E_SYS;
	}

#ifndef ETH_EXTERNAL_PHY
	ethPHY_setTrim();
#endif

	ethPHY_reset();

	if (ethPHY_waitAutoNegotiation() != E_OK) {
		DBG_ERR("Wait AN fail\r\n");
//	        loc_cpu();
//	        bOpened = FALSE;
//	        unl_cpu();
//	        return E_SYS;
	}

	return E_OK;
}

/*
    close external phy

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_close(void)
{
	UINT32 flags = 0;
	flags = eth_platform_spin_lock();

	if (bOpened == FALSE) {
		eth_platform_spin_unlock(flags);
		DBG_ERR("external PHY already closed\r\n");

		return E_SYS;
	}

	extPhyContext.linkState = ETHPHY_LINKST_DOWN;
	bOpened = FALSE;

	eth_platform_spin_unlock(flags);

	return E_OK;
}

/*
    reset external phy

    @return void
*/
void ethPHY_reset(void)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};
	if(!phy_rst_byps){
		// Issue reset
		controlReg.bit.RESET = 1;
		if (ethGMAC_writeExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, controlReg.reg) != E_OK) {
			DBG_ERR("Issue reset to PHY fail\r\n");
		}
		eth_platform_delay_ms(100);
	}else DBG_DUMP("ethPHY_reset :phy reset bypass\r\n");
	// Wait reset complete
	do {
		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
			DBG_ERR("Read PHY fail\r\n");
		} else {
			if (controlReg.bit.RESET == 0) {
				break;
			}
		}
	} while (1);

#if 0
	controlReg.bit.RESTART_AN = 1;
	if (ethGMAC_writeExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, controlReg.reg) != E_OK) {
		DBG_ERR("Issue Restart-AN to PHY fail\r\n");
	}
#endif

	DBG_IND("ctrl reg 0x%lx\r\n", controlReg.reg);
}

/*
    verify external phy ID

    @return
        - @b TRUE: ID match
        - @b FALSE: ID not match
*/
BOOL ethPHY_verifyID(void)
{
	UINT32 i, addr;
	T_EXTPHY_ID1_REG id1Reg = {0};
	T_EXTPHY_ID2_REG id2Reg = {0};

	for (i = 0; i < 32; i++) {
#if 0
		if ((pinmux_getPinmux(PIN_FUNC_ETH) == PIN_ETH_CFG_REVMII_10_100) ||
			(pinmux_getPinmux(PIN_FUNC_ETH) == PIN_ETH_CFG_REVMII_10_1000)) {
			// When RevMII is selected, internal PHY ID is fixed at 0
			// just break
			break;
		}
#endif

		if (ethGMAC_readExtPhy(i, EXTPHY_STATUS_REG_OFS, &addr) != E_OK) {
			DBG_ERR("Read PHY %d addr fail\r\n", i);
		} else {
			DBG_DUMP("PHY %d : ID 0x%lx\r\n", i, addr);
		}

		if ((addr == 0) || (addr == 0xFFFF)) {
#ifndef ETH_EXTERNAL_PHY
#if !defined(_NVT_FPGA_)
            DBG_DUMP("Embedded phy and realchip emu => assume PHY ID = 0\r\n");
            //break;
#elif (_NVT_FPGA_ == 0)
            DBG_DUMP("Embedded phy and realchip emu => assume PHY ID = 0\r\n");
            //break;
#endif

#else

            //DBG_DUMP("ETH_EXTERNAL_PHY: %d\r\n", ETH_EXTERNAL_PHY);

#endif
			continue;
		} else {
			break;
		}
	}
	if (i == 32) {
		DBG_ERR("No PHY ID found\r\n");
		return FALSE;
	}

	if (ethGMAC_readExtPhy(i, EXTPHY_ID1_REG_OFS, &id1Reg.reg) != E_OK) {
		DBG_ERR("Read PHY ID1 reg fail\r\n");
		return FALSE;
	}
	if (ethGMAC_readExtPhy(i, EXTPHY_ID2_REG_OFS, &id2Reg.reg) != E_OK) {
		DBG_ERR("Read PHY ID2 reg fail\r\n");
		return FALSE;
	}

	DBG_DUMP("Probed addr is 0x%lx\r\n", i);
	DBG_DUMP("Probed ID is 0x%lx, 0x%lx\r\n", id1Reg.reg, id2Reg.reg);

	uiPhyADDR = i;

	return TRUE;
}

/*
    wait Auto-Negotiation complete

    Wait AN complete and fetch link partner abillity (speed, duplex)

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_waitAutoNegotiation(void)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};
	T_EXTPHY_STATUS_REG statusReg = {0};
	T_EXTPHY_AN_ADV_REG advReg = {0};
	T_EXTPHY_AN_LINK_PARTNER lpaReg = {0};
	T_EXTPHY_1000_CONTROL_REG advgbReg = {0};
	T_EXTPHY_1000_STATUS_REG lpagbReg = {0};

	UINT32 uiCounter = 0;

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
		DBG_ERR("Read PHY ctrl fail\r\n");
		return E_SYS;
	}

	extPhyContext.network_speed = ETHPHY_SPD_10;
	extPhyContext.duplexMode = ETHPHY_DUPLEX_HALF;

	if (controlReg.bit.AN_EN) {
		DBG_DUMP("[PHY] Auto-Negotiation\r\n");
		do {
			if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_STATUS_REG_OFS, &statusReg.reg) != E_OK) {
				DBG_ERR("Read PHY status reg fail, cnt 0x%lx\r\n", uiCounter);
				return E_SYS;
			}
			if (statusReg.bit.AN_COMPLETE == 1) {
				break;
			}

			eth_platform_delay_ms(5);
		} while (uiCounter++ < 1000);

		if (uiCounter >= 1000) {
			DBG_ERR("Polling auto completion timeout\r\n");
			return E_SYS;
		}

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_STATUS_REG_OFS, &statusReg.reg) != E_OK) {
			DBG_ERR("Read PHY status reg fail, cnt 0x%lx\r\n", uiCounter);
			return E_SYS;
		}

		if (statusReg.bit.LINK_STS == 0) {
			DBG_ERR("Link NOT established\r\n");
			return E_SYS;
		}

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_AN_ADV_REG_OFS, &advReg.reg) != E_OK) {
			DBG_ERR("Read PHY ctrl fail\r\n");
			return E_SYS;
		}

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_AN_LINK_PARTNER_OFS, &lpaReg.reg) != E_OK) {
			DBG_ERR("Read PHY ctrl fail\r\n");
			return E_SYS;
		}

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_1000_CONTROL_REG_OFS, &advgbReg.reg) != E_OK) {
			DBG_ERR("Read PHY ctrl fail\r\n");
			return E_SYS;
		}

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_1000_STATUS_REG_OFS, &lpagbReg.reg) != E_OK) {
			DBG_ERR("Read PHY ctrl fail\r\n");
			return E_SYS;
		}
		extPhyContext.linkState = ETHPHY_LINKST_UP;

		if ((advgbReg.bit.HALF_1000 | advgbReg.bit.FULL_1000) & (lpagbReg.bit.HALF_1000 | lpagbReg.bit.FULL_1000)) {
			extPhyContext.network_speed = ETHPHY_SPD_1000;
			if (advgbReg.bit.FULL_1000 & lpagbReg.bit.FULL_1000) {
				extPhyContext.duplexMode = ETHPHY_DUPLEX_FULL;
			}
		} else if ((advReg.bit.L_100TXFD | advReg.bit.L_100BASE_TX) & (lpaReg.bit.P_100TXFD | lpaReg.bit.P_100BASE_TX)) {
			extPhyContext.network_speed = ETHPHY_SPD_100;
			if (advReg.bit.L_100TXFD & lpaReg.bit.P_100TXFD) {
				extPhyContext.duplexMode = ETHPHY_DUPLEX_FULL;
			}
		} else {
			if (advReg.bit.L_10FD & lpaReg.bit.P_10FD) {
				extPhyContext.duplexMode = ETHPHY_DUPLEX_FULL;
			}
		}
	} else {
		DBG_DUMP("[PHY] Force Speed/Duplex from Reg0\r\n");

		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_STATUS_REG_OFS, &statusReg.reg) != E_OK) {
			DBG_ERR("Read PHY status reg fail, cnt 0x%lx\r\n", uiCounter);
			return E_SYS;
		}

		if (statusReg.bit.LINK_STS == 0) {
			DBG_ERR("Link NOT established\r\n");
			return E_SYS;
		}
		extPhyContext.linkState = ETHPHY_LINKST_UP;

		switch ((controlReg.bit.SPD_1000 << 1) | controlReg.bit.SPD_SET) {
		case 0:
			extPhyContext.network_speed = ETHPHY_SPD_10;
			break;
		case 1:
			extPhyContext.network_speed = ETHPHY_SPD_100;
			break;
		case 2:
			extPhyContext.network_speed = ETHPHY_SPD_1000;
			break;
		default:
			DBG_ERR("Unkown speed: both bit 6 and 13 are set\r\n");
			break;
		}
		if (controlReg.bit.DUPLEX_MODE == 0) {
			extPhyContext.duplexMode = ETHPHY_DUPLEX_HALF;
		} else {
			extPhyContext.duplexMode = ETHPHY_DUPLEX_FULL;
		}
	}
	DBG_DUMP("[PHY] Speed: %d, Duplex: %d\r\n", extPhyContext.network_speed, extPhyContext.duplexMode);

	DBG_DUMP("[PHY] Speed: ");
	switch (extPhyContext.network_speed) {
	case ETHPHY_SPD_10:
		DBG_DUMP("10M \r\n");
		break;
	case ETHPHY_SPD_100:
		DBG_DUMP("100M \r\n");
		break;
	case ETHPHY_SPD_1000:
		DBG_DUMP("1000M \r\n");
		break;
	default:
		DBG_ERR("Unkown speed\r\n");
		break;
	}

	DBG_DUMP("[PHY] Duplex: ");
	switch (extPhyContext.duplexMode) {
	case ETHPHY_DUPLEX_HALF:
		DBG_DUMP("Half Duplex\r\n");
		break;
	case ETHPHY_DUPLEX_FULL:
		DBG_DUMP("Full Duplex\r\n");
		break;
	default:
		DBG_ERR("Unkown duplex mode\r\n");
		break;
	}

	return E_OK;
}

/*
    get link capability

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_getLinkCapability(PEXTPHY_CONTEXT pPhyContext)
{
	if (pPhyContext == NULL) {
		DBG_ERR("Input invalid NULL pointer\r\n");
		return E_SYS;
	}

	memcpy(pPhyContext, &extPhyContext, sizeof(extPhyContext));

	return E_OK;
}

/*
    set PHY Loopback

    Enable loopback at PHY side

    @param[in] bEnable      enable or disable loopback mode
                            - @b TRUE: enable loopback
                            - @b FALSE: disable loopback

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_setLoopback(BOOL bEnable, BOOL bAutoNegotiation)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
		DBG_ERR("Read PHY ctrl fail\r\n");
		return E_SYS;
	}

	controlReg.bit.LOOPBACK = bEnable;
	if (!bAutoNegotiation) {
		controlReg.bit.AN_EN = 0;
		controlReg.bit.DUPLEX_MODE = 1; // PHY full duplex
//        controlReg.bit.SPD_SET = 1;     // emu: force 100Mbps
		ethGMAC_setDuplexMode(ETHMAC_DUPLEX_FULL);
	}

	if (ethGMAC_writeExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, controlReg.reg) != E_OK) {
		DBG_ERR("Write PHY ctrl fail\r\n");
		return E_SYS;
	}

	return E_OK;
}

/*
    setup PHY ethernet speed

    @param[in] speed        Select speed
                            - @b ETHMAC_SPD_10: 10 Mbps
                            - @b ETHMAC_SPD_100: 100 Mbps
                            - @b ETHMAC_SPD_10000: 1000 Mbps

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_setSpeed(ETHMAC_SPD_ENUM speed)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
		DBG_ERR("Read PHY ctrl fail\r\n");
		return E_SYS;
	}

	controlReg.bit.AN_EN = 0;
	switch (speed) {
	case ETHMAC_SPD_100:
		controlReg.bit.SPD_1000 = 0;
		controlReg.bit.SPD_SET = 1;
		break;
	case ETHMAC_SPD_1000:
		controlReg.bit.SPD_1000 = 1;
		controlReg.bit.SPD_SET = 0;
		break;
	case ETHMAC_SPD_10:
	default:
		controlReg.bit.SPD_1000 = 0;
		controlReg.bit.SPD_SET = 0;
		break;
	}

	if (ethGMAC_writeExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, controlReg.reg) != E_OK) {
		DBG_ERR("Write PHY ctrl fail\r\n");
		return E_SYS;
	}

	return E_OK;
}

UINT32 ethPHY_getControl(void)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
		DBG_ERR("Read PHY ctrl fail\r\n");
		return E_SYS;
	}

	return controlReg.reg;
}

UINT32 ethPHY_getStatus(void)
{
	T_EXTPHY_STATUS_REG statusReg = {0};
	UINT32 cnt = 2;

	while (cnt) {
		if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_STATUS_REG_OFS, &statusReg.reg) != E_OK) {
			DBG_ERR("Read PHY status reg fail, cnt 0x%lx\r\n", cnt);
			return E_SYS;
		}
		cnt--;
	}

	return statusReg.reg;
}

UINT32 ethPHY_getAbility(void)
{
	T_EXTPHY_AN_LINK_PARTNER lpaReg = {0};

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_AN_LINK_PARTNER_OFS, &lpaReg.reg) != E_OK) {
		DBG_ERR("Read PHY status reg fail\r\n");
		return E_SYS;
	}

	return lpaReg.reg;
}

/*
    setup PHY ethernet duplex

    @param[in] duplex       Select duplex
                            - @b ETHMAC_DUPLEX_HALF: half duplex
                            - @b ETHMAC_DUPLEX_FULL: full duplex

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethPHY_setDuplex(ETHMAC_DUPLEX_ENUM duplex)
{
	T_EXTPHY_CONTROL_REG controlReg = {0};

	if (ethGMAC_readExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, &controlReg.reg) != E_OK) {
		DBG_ERR("Read PHY ctrl fail\r\n");
		return E_SYS;
	}

	switch (duplex) {
	case ETHMAC_DUPLEX_HALF:
		controlReg.bit.DUPLEX_MODE = 0;
		break;
	case ETHMAC_DUPLEX_FULL:
	default:
		controlReg.bit.DUPLEX_MODE = 1;
		break;
	}

	if (ethGMAC_writeExtPhy(uiPhyADDR, EXTPHY_CONTROL_REG_OFS, controlReg.reg) != E_OK) {
		DBG_ERR("Write PHY ctrl fail\r\n");
		return E_SYS;
	}

	return E_OK;
}

void ethPHY_set(UINT32 en)
{
	if(en) phy_rst_byps = 1;
	else if(en == 0) phy_rst_byps = 0;
	DBG_DUMP("ethPHY_set : Set phy reset bypass\r\n");
}

/*
    Get PHY ID

    @return probed PHY ID
*/
UINT32 ethPHY_getID(void)
{
	return uiPhyADDR;
}

/*
    set trim for internal phy

    @return void
*/
void ethPHY_setTrim(void)
{
#if 0
	T_ETH_IM_PHY_TX_ROUT_REG txRoutReg = {0};
	T_ETH_IM_PHY_TX_DAC_REG txAdcReg = {0};
	T_ETH_IM_PHY_TX_SEL_REG txSelReg = {0};
	UINT16 data;
	INT32  result;
	result = efuse_readParamOps(0x100, &data);

	// DBG_DUMP("Ethernet read result = %d\r\n", result);

	if (result >= 0) {
		DBG_DUMP(" => Trim data =  0x%04x\r\n", data);
	}

	txRoutReg.reg = ETH_GETREG(ETH_IM_PHY_TX_ROUT_REG_OFS);
	txAdcReg.reg = ETH_GETREG(ETH_IM_PHY_TX_DAC_REG_OFS);
	txSelReg.reg = ETH_GETREG(ETH_IM_PHY_TX_SEL_REG_OFS);

	txRoutReg.bit.TX_ROUT = (data & 0x07);
	txAdcReg.bit.TX_DAC = (data & 0xF8)>>3;
	txSelReg.bit.SEL_TX = (data & 0x700)>>8;
	txSelReg.bit.SEL_RX = (data & 0x3800)>>11;

	ETH_SETREG(ETH_IM_PHY_TX_ROUT_REG_OFS, txRoutReg.reg);
	ETH_SETREG(ETH_IM_PHY_TX_DAC_REG_OFS, txAdcReg.reg);
	ETH_SETREG(ETH_IM_PHY_TX_SEL_REG_OFS, txSelReg.reg);
#endif
}

void nvteth_register_link_cb(NVTETH_LINK_CB fp)
{
	UINT32 flags;
    //cyg_mutex_lock(&priv_data.mutex);
    flags = eth_platform_spin_lock();

    extPhyContext.link_cb = fp;

    //cyg_mutex_unlock(&priv_data.mutex);
    eth_platform_spin_unlock(flags);
}

void nvtimeth_phy_thread(void)
{
    //struct if_priv_data_t* priv = (struct if_priv_data_t*)data;
    const UINT32 tick = 500;
    UINT16 /*ctrl,*/ status = 0;
    UINT32 DuplexMode = ETHPHY_DUPLEX_HALF;
    UINT32 Speed = ETHPHY_SPD_10;
    UINT32 Link;
    UINT32 flags;

    while (1)
    {
        //UINT32 new_state = 0;

        //ctrl = ethPHY_getControl();
        status = ethPHY_getStatus();

        if (status & Mii_Link)
            Link = ETHPHY_LINKST_UP;
        else
            Link = ETHPHY_LINKST_DOWN;

        if ((Link != extPhyContext.linkState) || (is_first == 0))
        {
            UINT16 linkAbility = 0;
            const char* link_str[] = {"down", "up"};

            if (Link == ETHPHY_LINKST_UP)
            {
                linkAbility = ethPHY_getAbility();

                DuplexMode = ETHPHY_DUPLEX_HALF;
                if (linkAbility & (Mii_LinkAbl_100BaseTX_FD|Mii_LinkAbl_100BaseTX_HD))
                {
                    Speed = ETHPHY_SPD_100;

                    if (linkAbility & Mii_LinkAbl_100BaseTX_FD)
                    {
                        DuplexMode = ETHPHY_DUPLEX_FULL;
                    }
                }
                else
                {
                    Speed = ETHPHY_SPD_10;

                    if (linkAbility & Mii_LinkAbl_10BaseT_FD)
                    {
                        DuplexMode = ETHPHY_DUPLEX_FULL;
                    }
                }
            }

            //cyg_mutex_lock(&priv->mutex);
            flags = eth_platform_spin_lock();

            if (DuplexMode != extPhyContext.duplexMode)
            {
                extPhyContext.duplexMode = DuplexMode;
                //new_state = 1;
            }
            if (Speed != extPhyContext.network_speed)
            {
                extPhyContext.network_speed = Speed;
                //new_state = 1;
            }

            extPhyContext.linkState = Link;
            if (Link)
                DBG_DUMP("%s duplex, %sMbps, link %s\r\n",
                              (DuplexMode==ETHPHY_DUPLEX_FULL) ? "full":"half",
                              (Speed==ETHPHY_SPD_100) ? "100":"10",
                              link_str[Link]);
            else
                DBG_DUMP("link down\r\n");

            eqos_mac_init(&extPhyContext);

            //cyg_mutex_unlock(&priv->mutex);
            eth_platform_spin_unlock(flags);

            if (extPhyContext.link_cb)
            {
                extPhyContext.link_cb(Link==ETHPHY_LINKST_UP ? NVTETH_LINK_UP : NVTETH_LINK_DOWN);
            }
        }
        is_first = 1;

        //coverity[loop_top], exit_condition
        if (Link == 0xFF)
            break;

        eth_platform_delay_ms(tick);
    }
}

#ifndef ETH_EXTERNAL_PHY
/*
    poweron sequence for internal phy

    @return void
*/
void ethInternalPHY_powerOn(void)
{
	DBG_DUMP("%s\r\n", __func__);

    // EN_BGR        0xF8[7] = 1
    *(UINT32 *) (0xF02B3800 +  0xF8) |=  (1<<7);

    // Delay 20 us
    eth_platform_delay_us(20);

    // EN_PLL_LDO    0xC8[0] = 0
    *(UINT32 *) (0xF02B3800 +  0xC8) &= ~(1<<0);

    // Delay 200 us
    eth_platform_delay_us(200);

    // EN_PLL        0xC8[1] = 0
    *(UINT32 *) (0xF02B3800 +  0xC8) &= ~(1<<1);

    // Delay 250 us
    eth_platform_delay_us(250);

    // EN_TX_DAC     0x2E8[0] = 0
    *(UINT32 *) (0xF02B3800 + 0x2E8) &= ~(1<<0);

    // EN_TX_LD      N.A.

    // EN_RX         0xCC[0] = 0
    *(UINT32 *) (0xF02B3800 +  0xCC) &= ~(1<<0);

    // EN_ADC        0xDC[0] = 1  0x9C[0] = 0
    *(UINT32 *) (0xF02B3800 +  0xDC) |=  (1<<0);
    *(UINT32 *) (0xF02B3800 +  0x9C) &= ~(1<<0);


    // The following is for a-phy EVB
    // ana_rclk125m inversed
#if 0
    *(UINT32 *) (0xF02B3808) &= ~(0x3<<2);
    *(UINT32 *) (0xF02B3808) |=  (0x1<<2);

    *(UINT32 *) (0xF02B3808) &= ~(0x3<<0);
    *(UINT32 *) (0xF02B3808) |=  (0x1<<0);
#endif
    //*(UINT32 *) (0xF02B3A90) &= ~(0x1<<1);
    //*(UINT32 *) (0xF02B3A90) |=  (0x1<<1);
}
#endif

