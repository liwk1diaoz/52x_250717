/*
    Ethernet module driver

    This file is the GMAC in ethernet

    @file       Eth_gmac.c
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
#include "gpio.h"
//#include "Utility.h"
//
// Internal prototypes
//

//
// Internal Definitions
//
/*
    reset gmac

    @return void
*/

void ethGMAC_reset(void)
{
	UINT32 startTime, endTime;
	T_ETH_DMA_MODE_REG dmaModeReg = {0};
	T_ETH_HW_FEATURE0_REG hwFeatureReg = {0};

	dmaModeReg.bit.SWR = 1;
	ETH_SETREG(ETH_DMA_MODE_REG_OFS, dmaModeReg.reg);

	startTime = timer_get_current_count(TIMER_0);

	if (1) {
		// Patch for FPGA no internal clock source
		T_ETH_MAC_CONFIG_REG macConfigReg = {0};
		macConfigReg.bit.PS = 1;    // 10/100 Mbps
		ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
	}
	while (1) {
		dmaModeReg.reg = ETH_GETREG(ETH_DMA_MODE_REG_OFS);
		if (dmaModeReg.bit.SWR == 0) {
			break;
		}

		endTime = timer_get_current_count(TIMER_0);
		if ((endTime - startTime) > 3000000) {
			hwFeatureReg.reg = ETH_GETREG(ETH_HW_FEATURE0_REG_OFS);
			DBG_ERR("SWRST timeout\r\n");
			switch (hwFeatureReg.bit.ACTPHYSEL) {
			case ETH_PHYSEL_ENUM_GMII:
				DBG_ERR("Pinmux is GMII, check TX_CLK/RX_CLK\r\n");
				break;
			case ETH_PHYSEL_ENUM_REVMII:
				DBG_ERR("Pinmux is RevMII, check GTX_CLK\r\n");
				break;
			default:
				break;
			}
			abort();
		}
	}
}

/*
    set MAC address

    @param[in] id           MAC id
    @param[in] pMacAddr     MAC address to be installed. should be 6 bytes array

    @return void
*/
void ethGMAC_setMacAddress(ETH_MAC_ID id, UINT8 *pMacAddr)
{
	T_ETH_MAC_ADDR0_HIGH_REG macHighReg;
	T_ETH_MAC_ADDR0_LOW_REG macLowReg;
	const UINT32 uiMacHighOffset[ETH_MAC_ID_COUNT] = {
		ETH_MAC_ADDR0_HIGH_REG_OFS,     ETH_MAC_ADDR1_HIGH_REG_OFS,
		ETH_MAC_ADDR2_HIGH_REG_OFS,     ETH_MAC_ADDR3_HIGH_REG_OFS,
		ETH_MAC_ADDR4_HIGH_REG_OFS,     ETH_MAC_ADDR5_HIGH_REG_OFS,
		ETH_MAC_ADDR6_HIGH_REG_OFS,     ETH_MAC_ADDR7_HIGH_REG_OFS,
		ETH_MAC_ADDR8_HIGH_REG_OFS,     ETH_MAC_ADDR9_HIGH_REG_OFS,
		ETH_MAC_ADDR10_HIGH_REG_OFS,    ETH_MAC_ADDR11_HIGH_REG_OFS,
		ETH_MAC_ADDR12_HIGH_REG_OFS,    ETH_MAC_ADDR13_HIGH_REG_OFS,
		ETH_MAC_ADDR14_HIGH_REG_OFS,    ETH_MAC_ADDR15_HIGH_REG_OFS,
	};
	const UINT32 uiMacLowOffset[ETH_MAC_ID_COUNT] = {
		ETH_MAC_ADDR0_LOW_REG_OFS,      ETH_MAC_ADDR1_LOW_REG_OFS,
		ETH_MAC_ADDR2_LOW_REG_OFS,      ETH_MAC_ADDR3_LOW_REG_OFS,
		ETH_MAC_ADDR4_LOW_REG_OFS,      ETH_MAC_ADDR5_LOW_REG_OFS,
		ETH_MAC_ADDR6_LOW_REG_OFS,      ETH_MAC_ADDR7_LOW_REG_OFS,
		ETH_MAC_ADDR8_LOW_REG_OFS,      ETH_MAC_ADDR9_LOW_REG_OFS,
		ETH_MAC_ADDR10_LOW_REG_OFS,     ETH_MAC_ADDR11_LOW_REG_OFS,
		ETH_MAC_ADDR12_LOW_REG_OFS,     ETH_MAC_ADDR13_LOW_REG_OFS,
		ETH_MAC_ADDR14_LOW_REG_OFS,     ETH_MAC_ADDR15_LOW_REG_OFS,
	};

	if (id >= ETH_MAC_ID_COUNT) {
		DBG_ERR("id %d exceed range\r\n", id);
		return;
	}

	macHighReg.reg = ETH_GETREG(uiMacHighOffset[id]);
	macLowReg.reg = ETH_GETREG(uiMacLowOffset[id]);

	macHighReg.bit.ADDRHI = (pMacAddr[5] << 8) | pMacAddr[4];
	macLowReg.bit.ADDRLO = (pMacAddr[3] << 24) | (pMacAddr[2] << 16) |
						   (pMacAddr[1] << 8) | pMacAddr[0];

	ETH_SETREG(uiMacHighOffset[id], macHighReg.reg);
	ETH_SETREG(uiMacLowOffset[id], macLowReg.reg);

}

/*
    Init MDC clock

    Initialize MDC clock to 1~2.5 MHz

    @note realchip should get 1.9 MHz

    @return void
*/
void ethGMAC_initMDCclock(void)
{
	T_ETH_MDIO_ADDR_REG mdioAddrReg;

	mdioAddrReg.reg = ETH_GETREG(ETH_MDIO_ADDR_REG_OFS);
	mdioAddrReg.bit.CR = ETH_CR_ENUM_DIV42;
#if defined(_NVT_FPGA_)
	// If FPGA, PCLK is 20~28 MHz, use DIV12 instead
	mdioAddrReg.bit.CR = ETH_CR_ENUM_DIV12;
#endif

	ETH_SETREG(ETH_MDIO_ADDR_REG_OFS, mdioAddrReg.reg);
}

/*
    Write external PHY register

    @param[in] uiPhyAddr        Phy address output to MDIO
    @param[in] uiRegAddr        Register address in PHY (range: 0x00 ~ 0x1F)
    @param[in] uiData           Data to be written (range: 0x0000 ~ 0xFFFF)

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethGMAC_writeExtPhy(UINT32 uiPhyAddr, UINT32 uiRegAddr, UINT32 uiData)
{
	T_ETH_MDIO_ADDR_REG phyAddrReg = {0};
	T_ETH_MDIO_DATA_REG phyDataReg = {0};

	if (uiPhyAddr > 0x1F) {
		DBG_ERR("phy addr 0x%lx exceed 5 bits\r\n", uiPhyAddr);
		return E_SYS;
	}
	if (uiRegAddr > 0x1F) {
		DBG_ERR("reg addr 0x%lx exceed 5 bits\r\n", uiRegAddr);
		return E_SYS;
	}
	if (uiData & 0xFFFF0000) {
		DBG_ERR("Input data 0x%lx exceed 16 bits\r\n", uiData);
		return E_SYS;
	}

	// Ensure MDIO interface ready
	do {
		phyAddrReg.reg = ETH_GETREG(ETH_MDIO_ADDR_REG_OFS);
		if (phyAddrReg.bit.GB == ETH_GMII_GB_READY) {
			break;
		}
	} while (1);

	// Prepare data before trigger writing
	phyDataReg.bit.GD = uiData;
	ETH_SETREG(ETH_MDIO_DATA_REG_OFS, phyDataReg.reg);

	// Fill address and trigger
	phyAddrReg.bit.PA = uiPhyAddr;
	phyAddrReg.bit.RDA = uiRegAddr;
	phyAddrReg.bit.GB = ETH_GMII_GB_BUSY;       // BUSY set by f/w, cleared by h/w
	phyAddrReg.bit.GOC = ETH_GMII_GOC_WRITE;
	ETH_SETREG(ETH_MDIO_ADDR_REG_OFS, phyAddrReg.reg);

	return E_OK;
}

/*
    Read external PHY register

    @param[in] uiPhyAddr        Phy address output to MDIO
    @param[in] uiRegAddr        Register address in PHY (range: 0x00 ~ 0x1F)
    @param[in] puiData          Data to be read (range: 0x0000 ~ 0xFFFF)

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethGMAC_readExtPhy(UINT32 uiPhyAddr, UINT32 uiRegAddr, UINT32 *puiData)
{
	T_ETH_MDIO_ADDR_REG phyAddrReg = {0};
	T_ETH_MDIO_DATA_REG phyDataReg = {0};

	if (uiPhyAddr > 0x1F) {
		DBG_ERR("phy addr 0x%lx exceed 5 bits\r\n", uiPhyAddr);
		return E_SYS;
	}
	if (uiRegAddr > 0x1F) {
		DBG_ERR("reg addr 0x%lx exceed 5 bits\r\n", uiRegAddr);
		return E_SYS;
	}
	if (puiData == NULL) {
		DBG_ERR("Invalid pointer address 0x%lx\r\n", puiData);
		return E_SYS;
	}

	// Ensure MDIO interface ready
	do {
		phyAddrReg.reg = ETH_GETREG(ETH_MDIO_ADDR_REG_OFS);
		if (phyAddrReg.bit.GB == ETH_GMII_GB_READY) {
			break;
		}
	} while (1);

	// Fill address and trigger
	phyAddrReg.bit.PA = uiPhyAddr;
	phyAddrReg.bit.RDA = uiRegAddr;
	phyAddrReg.bit.GB = ETH_GMII_GB_BUSY;       // BUSY set by f/w, cleared by h/w
	phyAddrReg.bit.GOC = ETH_GMII_GOC_READ;
	ETH_SETREG(ETH_MDIO_ADDR_REG_OFS, phyAddrReg.reg);

	// Wait data read
	do {
		phyAddrReg.reg = ETH_GETREG(ETH_MDIO_ADDR_REG_OFS);
		if (phyAddrReg.bit.GB == ETH_GMII_GB_READY) {
			break;
		}
	} while (1);

	// Get read data
	phyDataReg.reg = ETH_GETREG(ETH_MDIO_DATA_REG_OFS);
	*puiData = phyDataReg.bit.GD;

	return E_OK;
}

/*
    set MAC frame burst

    frame burst is introduced by Gigabit Ethernet Half-duplex

    @param[in] bEnable      enable or disable GMAC frame burst
                            - @b TRUE: Enable frame burst.
                            - @b FALSE: Disable frame burst.

    @return void
*/
void ethGMAC_setFrameBurst(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.BE = 1;
	} else {
		macConfigReg.bit.BE = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    set MAC jumbo frame

    @param[in] bEnable      enable or disable jumbo frame support
                            - @b TRUE: support jumbo frame
                            - @b FALSE: NOT support jumbo frame

    @return void
*/
void ethGMAC_setJumboFrame(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.JE = 1;
	} else {
		macConfigReg.bit.JE = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    set MAC Loopback

    Enable loopback at MAC side

    @param[in] bEnable      enable or disable loopback mode
                            - @b TRUE: enable loopback
                            - @b FALSE: disable loopback

    @return void
*/
void ethGMAC_setLoopback(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.LM = 1;
	} else {
		macConfigReg.bit.LM = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    set MAC duplex mode

    @param[in] duplexMode   Duplex mode
                            - @b ETHMAC_DUPLEX_HALF: half duplex
                            - @b ETHMAC_DUPLEX_FULL: full duplex

    @return void
*/
void ethGMAC_setDuplexMode(ETHMAC_DUPLEX_ENUM duplexMode)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (duplexMode == ETHMAC_DUPLEX_HALF) {
		macConfigReg.bit.DM = 0;
	} else {
		macConfigReg.bit.DM = 1;
	}
	DBG_DUMP("[MAC] Duplex: %s\r\n", macConfigReg.bit.DM ? "Full Duplex" : "Half Duplex");

	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    set MAC CRC stripping

    CRC stripping will strip padding/CRC field in received frame if
    receive frame length <= 1500 bytes.

    @param[in] bEnable      enable or disable CRC stripping
                            - @b TRUE: enable stripping
                            - @b FALSE: disable stripping

    @return void
*/
void ethGMAC_setCrcStrip(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.ACS = 1;
		macConfigReg.bit.CST = 1;
	} else {
		macConfigReg.bit.ACS = 0;
		macConfigReg.bit.CST = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    setup MAC receive IP checksum offload

    @param[in] bEnable      enable or disable checksum offload
                            - @b TRUE: enable checksum offload
                            - @b FALSE: disable checksum offload

    @return void
*/
void ethGMAC_setRecvChecksumOffload(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.IPC = 1;
	} else {
		macConfigReg.bit.IPC = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    setup MAC Transmit Enable

    @param[in] bEnable      enable or disable transmit state machine
                            - @b TRUE: enable transmission
                            - @b FALSE: disable transmission

    @return void
*/
void ethGMAC_setTxEn(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.TE = 1;
	} else {
		macConfigReg.bit.TE = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    setup MAC Receive Enable

    @param[in] bEnable      enable or disable receive state machine
                            - @b TRUE: enable reception
                            - @b FALSE: disable reception

    @return void
*/
void ethGMAC_setRxEn(BOOL bEnable)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	if (bEnable) {
		macConfigReg.bit.RE = 1;
	} else {
		macConfigReg.bit.RE = 0;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    setup MAC ethernet speed

    @param[in] speed        Select speed
                            - @b ETHMAC_SPD_10: 10 Mbps
                            - @b ETHMAC_SPD_100: 100 Mbps
                            - @b ETHMAC_SPD_10000: Invalid (1000 Mbps)

    @return void
*/
void ethGMAC_setSpeed(ETHMAC_SPD_ENUM speed)
{
	T_ETH_MAC_CONFIG_REG macConfigReg = {0};

	macConfigReg.reg = ETH_GETREG(ETH_MAC_CONFIG_REG_OFS);
	switch (speed) {
	case ETHMAC_SPD_10:
		macConfigReg.bit.FES = 0;   // 10 Mbps
		macConfigReg.bit.PS = 1;    // 10/100 Mbps
		DBG_DUMP("[MAC] Speed: 10M\r\n");
		break;
	case ETHMAC_SPD_100:
		macConfigReg.bit.FES = 1;   // 100 Mbps
		macConfigReg.bit.PS = 1;    // 10/100 Mbps
		DBG_DUMP("[MAC] Speed: 100M\r\n");
		break;
	default:
		DBG_ERR("%s: unknow speed %d\r\n", __func__, speed);
		macConfigReg.bit.FES = 0;   // 10 Mbps
		macConfigReg.bit.PS = 1;    // 10/100 Mbps
		break;
	}
	ETH_SETREG(ETH_MAC_CONFIG_REG_OFS, macConfigReg.reg);
}

/*
    setup MAC received frame filter mode

    @param[in] filterMode   receive frame filter mode
                            - @b ETHMAC_RX_FRAME_FILTER_DISABLE: disable filtering. i.e. receive all
                            - @b ETHMAC_RX_FRAME_FILTER_PROMISCUOUS: filtering, but promiscuous mode
                            - @b ETHMAC_RX_FRAME_FILTER_NORMAL: filtering mode

    @return void
*/
void ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_ENUM filterMode)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	switch (filterMode) {
	case ETHMAC_RX_FRAME_FILTER_DISABLE:
		frameFilterReg.bit.RA = 1;  // receive all frame. i.e. disable address filter
		frameFilterReg.bit.PR = 0;
		break;
	case ETHMAC_RX_FRAME_FILTER_PROMISCUOUS:
		frameFilterReg.bit.RA = 0;  // receive when address filter passed
		frameFilterReg.bit.PR = 1;  // enable promiscuous mode
		break;
	case ETHMAC_RX_FRAME_FILTER_NORMAL:
	default:
		frameFilterReg.bit.RA = 0;  // receive when address filter passed
		frameFilterReg.bit.PR = 0;  // normal filtering
		break;
	}
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx control frame filtering

    @param[in] ctrlFrameMode    Select control frame processing mode
                                - @b ETHMAC_CTRL_FRAME_FILTER_ALL: filter all control frame
                                - @b ETHMAC_CTRL_FRAME_FILTER_PAUSE: only filter pause frame
                                - @b ETHMAC_CTRL_FRAME_RECV_ALL: receive all control frame
                                - @b ETHMAC_CTRL_FRAME_RECV_ADDR: receive control frame passed address filter

    @return void
*/
void ethGMAC_setRxCtrlFrameFilter(ETHMAC_CTRL_FRAME_ENUM ctrlFrameMode)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	frameFilterReg.bit.PCF = ctrlFrameMode;
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx broadcast frame

    @param[in] bEnable      enable or disable reception of broadcast frame
                            - @b TRUE: enable reception of broadcast frame
                            - @b FALSE: disable reception of boradcast frame

    @return void
*/
void ethGMAC_setRxBroadcast(BOOL bEnable)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	if (bEnable) {
		frameFilterReg.bit.DBF = 0;
	} else {
		frameFilterReg.bit.DBF = 1;
	}
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx Source Address filter

    @param[in] bEnable      enable or disable rx source address filter
                            - @b TRUE: enable source address filter
                            - @b FALSE: disable source address filter

    @return void
*/
void ethGMAC_setRxSrcAddrFilter(BOOL bEnable)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	if (bEnable) {
		frameFilterReg.bit.SAF = 1;
	} else {
		frameFilterReg.bit.SAF = 0;
	}
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx Destination Address filter

    @param[in] filterMode   Destinatin address filter mode
                            - @b ETHMAC_DST_ADDR_FILTER_MODE_NORMAL: normal
                            - @b ETHMAC_DST_ADDR_FILTER_MODE_INVERSE: invert result

    @return void
*/
void ethGMAC_setRxDstAddrFilterMode(ETHMAC_DST_ADDR_FILTER_MODE_ENUM filterMode)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	frameFilterReg.bit.DAIF = filterMode;
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx multicast frame filter

    @param[in] filerMode    multicast filter mode
                            - @b ETHMAC_MULTICAST_FILTER_MODE_NORMAL: normal
                            - @b ETHMAC_MULTICAST_FILTER_MODE_PERFECT: perfect matching
                            - @b ETHMAC_MULTICAST_FILTER_MODE_HASH: hash matching

    @return void
*/
void ethGMAC_setRxMulticastFilter(ETHMAC_MULTICAST_FILTER_MODE_ENUM filerMode)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	switch (filerMode) {
	case ETHMAC_MULTICAST_FILTER_MODE_NORMAL:
		frameFilterReg.bit.PM = 1;      // accept all muticast frame
		frameFilterReg.bit.HMC = 0;
		break;
	case ETHMAC_MULTICAST_FILTER_MODE_PERFECT:
		frameFilterReg.bit.PM = 0;      // refer to HMC field
		frameFilterReg.bit.HMC = 0;     // perfect mathcing
		break;
	case ETHMAC_MULTICAST_FILTER_MODE_HASH:
	default:
		frameFilterReg.bit.PM = 0;      // refer to HMC field
		frameFilterReg.bit.HMC = 1;     // hash matching
		break;
	}
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC Rx unicast hash matching

    @param[in] bEnable      enable or disable unicast hash matching
                            - @b TRUE: enable unicast hash matching
                            - @b FALSE: disable unicast hash matching (compare with destination address register)

    @return void
*/
void ethGMAC_setRxUnicastHash(BOOL bEnable)
{
	T_ETH_MAC_PACKET_FILTER_REG frameFilterReg = {0};

	frameFilterReg.reg = ETH_GETREG(ETH_MAC_PACKET_FILTER_REG_OFS);
	if (bEnable) {
		frameFilterReg.bit.HUC = 1;
	} else {
		frameFilterReg.bit.HUC = 0;
	}
	ETH_SETREG(ETH_MAC_PACKET_FILTER_REG_OFS, frameFilterReg.reg);
}

/*
    setup MAC unicast pause frame

    @param[in] bEnable      enable or disable unicast pause frame support
                            - @b TRUE: detect pause frame with unicast address specified by MAC address register
                            - @b FALSE: detect pause frame with unique multicast address (spec value)

    @return void
*/
void ethGMAC_setUnicastPauseFrame(BOOL bEnable)
{
	T_ETH_Q0_RX_FLOW_CTRL_REG flowCtrlReg = {0};

	flowCtrlReg.reg = ETH_GETREG(ETH_Q0_RX_FLOW_CTRL_REG_OFS);
	if (bEnable) {
		flowCtrlReg.bit.UP = 1;
	} else {
		flowCtrlReg.bit.UP = 0;
	}
	ETH_SETREG(ETH_Q0_RX_FLOW_CTRL_REG_OFS, flowCtrlReg.reg);
}

/*
    setup MAC rx flow control

    @param[in] bEnable      enable or disable rx flow control
                            - @b TRUE: enable rx flow control
                            - @b FALSE: disable rx flow control

    @return void
*/
void ethGMAC_setRxFlowControl(BOOL bEnable)
{
	T_ETH_Q0_RX_FLOW_CTRL_REG flowCtrlReg = {0};

	flowCtrlReg.reg = ETH_GETREG(ETH_Q0_RX_FLOW_CTRL_REG_OFS);
	if (bEnable) {
		flowCtrlReg.bit.RFE = 1;
	} else {
		flowCtrlReg.bit.RFE = 0;
	}
	ETH_SETREG(ETH_Q0_RX_FLOW_CTRL_REG_OFS, flowCtrlReg.reg);
}

/*
    setup MAC tx flow control

    @param[in] bEnable      enable or disable tx flow control
                            - @b TRUE: enable tx flow control
                            - @b FALSE: disable tx flow control

    @return void
*/
void ethGMAC_setTxFlowControl(BOOL bEnable)
{
	T_ETH_Q0_TX_FLOW_CTRL_REG flowCtrlReg = {0};

	flowCtrlReg.reg = ETH_GETREG(ETH_Q0_TX_FLOW_CTRL_REG_OFS);
	if (bEnable) {
		flowCtrlReg.bit.TFE = 1;
	} else {
		flowCtrlReg.bit.TFE = 0;
	}
	ETH_SETREG(ETH_Q0_TX_FLOW_CTRL_REG_OFS, flowCtrlReg.reg);
}

/*
    disable all MAC MMC interrupt

    @return void
*/
void ethGMAC_disableAllMMCIntEn(void)
{
	ETH_SETREG(ETH_MMC_RX_INT_MSK_REG_OFS, 0xFFFFFFFF);
	ETH_SETREG(ETH_MMC_TX_INT_MSK_REG_OFS, 0xFFFFFFFF);
	ETH_SETREG(ETH_MMC_RX_IPC_INT_MSK_REG_OFS, 0xFFFFFFFF);
}

/*
    setup DMA Address Aligned Beats

    @param[in] en       enable/disable address aligned beats
                        - @b TRUE: enable
                        - @b FALSE: disable

    @return void
*/
void ethGMAC_setDmaAAL(BOOL en)
{
	T_ETH_DMA_SYS_BUS_REG dmaSysBusReg;

	dmaSysBusReg.reg = ETH_GETREG(ETH_DMA_SYS_BUS_REG_OFS);

	dmaSysBusReg.bit.AAL = en;
	if( nvt_get_chip_id() == CHIP_NA51084) {
		dmaSysBusReg.bit.RD_OSR_LMT = 3;
		dmaSysBusReg.bit.WR_OSR_LMT = 3;
	}

	ETH_SETREG(ETH_DMA_SYS_BUS_REG_OFS, dmaSysBusReg.reg);
}

/*
    set Fixed Burst Length

    @param[in] en       enable/disable fixed burst length
                        - @b TRUE: enable
                        - @b FALSE: disable

    @return void
*/
void ethGMAC_setFixedBurst(BOOL en)
{
	T_ETH_DMA_SYS_BUS_REG dmaSysBusReg;

	dmaSysBusReg.reg = ETH_GETREG(ETH_DMA_SYS_BUS_REG_OFS);

	dmaSysBusReg.bit.FB = en;

	ETH_SETREG(ETH_DMA_SYS_BUS_REG_OFS, dmaSysBusReg.reg);
}

/*
    setup DMA transmit descriptor address

    @param[in] uiAddr       DRAM address of first tx descriptor

    @return void
*/
void ethGMAC_setDmaTxDescAddr(UINT32 uiAddr)
{
	T_ETH_DMA_CH0_TxDesc_LIST_ADDR_REG txDescriptReg = {0};
	if( nvt_get_chip_id() == CHIP_NA51084) {
		if (uiAddr & (ETHMAC_SYSBUS_WIDTH_MASK|0x4)) {
			DBG_ERR("Address 0x%lx should be 32-bit align\r\n", uiAddr);
		}
	} else {
		if (uiAddr & ETHMAC_SYSBUS_WIDTH_MASK) {
			DBG_ERR("Address 0x%lx should be 32-bit align\r\n", uiAddr);
		}
	}

	txDescriptReg.reg = uiAddr;
	ETH_SETREG(ETH_DMA_CH0_TxDesc_LIST_ADDR_REG_OFS, txDescriptReg.reg);
}

/*
    setup DMA receive descriptor address

    @param[in] uiAddr       DRAM address of first rx descriptor

    @return void
*/
void ethGMAC_setDmaRxDescAddr(UINT32 uiAddr)
{
	T_ETH_DMA_CH0_RxDesc_LIST_ADDR_REG rxDescriptReg = {0};
	if( nvt_get_chip_id() == CHIP_NA51084) {
		if (uiAddr & (ETHMAC_SYSBUS_WIDTH_MASK|0x4)) {
			DBG_ERR("Address 0x%lx should be 32-bit align\r\n", uiAddr);
		}
	} else {
		if (uiAddr & ETHMAC_SYSBUS_WIDTH_MASK) {
			DBG_ERR("Address 0x%lx should be 32-bit align\r\n", uiAddr);
		}
	}

	rxDescriptReg.reg = uiAddr;
	ETH_SETREG(ETH_DMA_CH0_RxDesc_LIST_ADDR_REG_OFS, rxDescriptReg.reg);
}

/*
    setup DMA burst length

    @param[in] burst        DMA burst length

    @return void
*/
void ethGMAC_setDmaBurst(ETHMAC_DMA_BURST_ENUM burst)
{
	T_ETH_DMA_CH0_CTRL_REG dmaCh0CtrlReg = {0};
	T_ETH_DMA_CH0_TX_CTRL_REG dmaCh0TxCtrlReg = {0};
	T_ETH_DMA_CH0_RX_CTRL_REG dmaCh0RxCtrlReg = {0};

	dmaCh0CtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_CTRL_REG_OFS);
	dmaCh0TxCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS);
	dmaCh0RxCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS);
	switch (burst) {
	case ETHMAC_DMA_BURST_2:
		dmaCh0TxCtrlReg.bit.TxPBL = 2;
		dmaCh0RxCtrlReg.bit.RxPBL = 2;
		dmaCh0CtrlReg.bit.PBLx8 = 0;
		break;
	case ETHMAC_DMA_BURST_4:
		dmaCh0TxCtrlReg.bit.TxPBL = 4;
		dmaCh0RxCtrlReg.bit.RxPBL = 4;
		dmaCh0CtrlReg.bit.PBLx8 = 0;
		break;
	case ETHMAC_DMA_BURST_8:
		dmaCh0TxCtrlReg.bit.TxPBL = 8;
		dmaCh0RxCtrlReg.bit.RxPBL = 8;
		dmaCh0CtrlReg.bit.PBLx8 = 0;
		break;
	case ETHMAC_DMA_BURST_16:
		dmaCh0TxCtrlReg.bit.TxPBL = 16;
		dmaCh0RxCtrlReg.bit.RxPBL = 16;
		dmaCh0CtrlReg.bit.PBLx8 = 0;
		break;
	case ETHMAC_DMA_BURST_32:
		dmaCh0TxCtrlReg.bit.TxPBL = 4;
		dmaCh0RxCtrlReg.bit.RxPBL = 4;
		dmaCh0CtrlReg.bit.PBLx8 = 1;
		break;
	case ETHMAC_DMA_BURST_64:
		dmaCh0TxCtrlReg.bit.TxPBL = 8;
		dmaCh0RxCtrlReg.bit.RxPBL = 8;
		dmaCh0CtrlReg.bit.PBLx8 = 1;
		break;
	case ETHMAC_DMA_BURST_128:
		dmaCh0TxCtrlReg.bit.TxPBL = 16;
		dmaCh0RxCtrlReg.bit.RxPBL = 16;
		dmaCh0CtrlReg.bit.PBLx8 = 1;
		break;
	case ETHMAC_DMA_BURST_256:
		dmaCh0TxCtrlReg.bit.TxPBL = 32;
		dmaCh0RxCtrlReg.bit.RxPBL = 32;
		dmaCh0CtrlReg.bit.PBLx8 = 1;
		break;
	case ETHMAC_DMA_BURST_1:
	default:
		dmaCh0TxCtrlReg.bit.TxPBL = 1;
		dmaCh0RxCtrlReg.bit.RxPBL = 1;
		dmaCh0CtrlReg.bit.PBLx8 = 0;
		break;
	}
	ETH_SETREG(ETH_DMA_CH0_CTRL_REG_OFS, dmaCh0CtrlReg.reg);
	ETH_SETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS, dmaCh0TxCtrlReg.reg);
	ETH_SETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS, dmaCh0RxCtrlReg.reg);
}

/*
    setup DMA Maximum Segment Size

    @param[in] uiMSS    TCP MSS (unit: byte)

    @return void
*/
void ethGMAC_setDmaMSS(UINT32 uiMSS)
{
	T_ETH_DMA_CH0_CTRL_REG dmaCh0CtrlReg = {0};

	dmaCh0CtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_CTRL_REG_OFS);
	dmaCh0CtrlReg.bit.MSS = uiMSS;
	ETH_SETREG(ETH_DMA_CH0_CTRL_REG_OFS, dmaCh0CtrlReg.reg);
}

/*
    setup DMA descriptor skip length

    @param[in] uiWordCount  word count between descriptors

    @return void
*/
void ethGMAC_setDmaDescSkipLen(UINT32 uiWordCount)
{
	T_ETH_DMA_CH0_CTRL_REG dmaCh0CtrlReg = {0};

	if( nvt_get_chip_id() == CHIP_NA51084) {
		if (uiWordCount % ((ETHMAC_SYSBUS_WIDTH_SIZE*2) / sizeof(UINT32))) {
			DBG_ERR("skipLen should be word (4 byte) align, but %d word\r\n", uiWordCount);
		}

		uiWordCount /= (ETHMAC_SYSBUS_WIDTH_SIZE*2) / sizeof(UINT32);
	} else {
		if (uiWordCount % (ETHMAC_SYSBUS_WIDTH_SIZE / sizeof(UINT32))) {
			DBG_ERR("skipLen should be word (4 byte) align, but %d word\r\n", uiWordCount);
		}
		uiWordCount /= ETHMAC_SYSBUS_WIDTH_SIZE / sizeof(UINT32);
	}

	dmaCh0CtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_CTRL_REG_OFS);
	dmaCh0CtrlReg.bit.DSL = uiWordCount;
	ETH_SETREG(ETH_DMA_CH0_CTRL_REG_OFS, dmaCh0CtrlReg.reg);
}

/*
    setup DMA rx descriptor count

    @param[in] uiCount      total descriptor count (unit: descriptor) (range: 1~1024)

    @return void
*/
void ethGMAC_setDmaRxDescCount(UINT32 uiCount)
{
	T_ETH_DMA_CH0_RxDesc_RING_LEN_REG rxDescRingLenReg = {0};

	if ((uiCount == 0) || (uiCount > 1024)) {
		DBG_ERR("desc count should between 1~1024, but %d\r\n", uiCount);
		if (uiCount == 0) {
			uiCount = 1;
		} else {
			uiCount = 1024;
		}
	}

	rxDescRingLenReg.bit.RDRL = uiCount - 1;
	ETH_SETREG(ETH_DMA_CH0_RxDesc_RING_LEN_REG_OFS, rxDescRingLenReg.reg);
}

/*
    setup DMA tx descriptor count

    @param[in] uiCount      total descriptor count (unit: descriptor) (range: 1~1024)

    @return void
*/
void ethGMAC_setDmaTxDescCount(UINT32 uiCount)
{
	T_ETH_DMA_CH0_TxDesc_RING_LEN_REG txDescRingLenReg = {0};

	if ((uiCount == 0) || (uiCount > 1024)) {
		DBG_ERR("desc count should between 1~1024, but %d\r\n", uiCount);
		if (uiCount == 0) {
			uiCount = 1;
		} else {
			uiCount = 1024;
		}
	}

	txDescRingLenReg.bit.TDRL = uiCount - 1;
	ETH_SETREG(ETH_DMA_CH0_TxDesc_RING_LEN_REG_OFS, txDescRingLenReg.reg);
}

/*
    setup DMA receive threshold level

    @param[in] uiByteCount  receive threshold level (unit: byte)
                            - @b 32: 32 bytes
                            - @b 64: 64 bytes
                            - @b 96: 96 bytes
                            - @b 128: 128 bytes

    @return void
*/
void ethGMAC_setDmaRxThreshold(UINT32 uiByteCount)
{
	T_ETH_MTL_RxQ0_OP_MODE_REG rxOpReg = {0};

	rxOpReg.reg = ETH_GETREG(ETH_MTL_RxQ0_OP_MODE_REG_OFS);
	switch (uiByteCount) {
	case 64:
		rxOpReg.bit.RTC = 0;
		break;
	case 96:
		rxOpReg.bit.RTC = 2;
		break;
	case 128:
		rxOpReg.bit.RTC = 3;
		break;
	case 32:
	default:
		rxOpReg.bit.RTC = 1;
		break;
	}
	ETH_SETREG(ETH_MTL_RxQ0_OP_MODE_REG_OFS, rxOpReg.reg);
}

/*
    setup DMA receive mode

    @param[in] rxMode       DMA receive mode
                            - @b ETHMAC_DMA_RXMODE_CUT_THROUGH: cut through mode
                            - @b ETHMAC_DMA_RXMODE_STORE_FORWARD: storea and forward mode

    @return void
*/
void ethGMAC_setDmaRxMode(ETHMAC_DMA_RXMODE_ENUM rxMode)
{
	T_ETH_MTL_RxQ0_OP_MODE_REG rxOpReg = {0};

	rxOpReg.reg = ETH_GETREG(ETH_MTL_RxQ0_OP_MODE_REG_OFS);
	if (rxMode == ETHMAC_DMA_RXMODE_CUT_THROUGH) {
		rxOpReg.bit.RSF = 0;
	} else {
		rxOpReg.bit.RSF = 1;
	}
	ETH_SETREG(ETH_MTL_RxQ0_OP_MODE_REG_OFS, rxOpReg.reg);
}

/*
    setup DMA transmit mode

    @param[in] txMode       DMA transmit mode
                            - @b ETHMAC_DMA_TXMODE_THRESHOLD: threshold mode
                            - @b ETHMAC_DMA_TXMODE_STORE_FORWARD: storea and forward mode

    @return void
*/
void ethGMAC_setDmaTxMode(ETHMAC_DMA_TXMODE_ENUM txMode)
{
	T_ETH_MTL_TxQ0_OP_MODE_REG txOpReg = {0};

	txOpReg.reg = ETH_GETREG(ETH_MTL_TxQ0_OP_MODE_REG_OFS);
	if (txMode == ETHMAC_DMA_TXMODE_THRESHOLD) {
		txOpReg.bit.TSF = 0;
	} else {
		txOpReg.bit.TSF = 1;
	}
	ETH_SETREG(ETH_MTL_TxQ0_OP_MODE_REG_OFS, txOpReg.reg);
}

/*
    setup DMA receive enable

    @param[in] bEnable      enable/disable rx DMA
                            - @b TRUE: enable rx DMA
                            - @b FALSE: disable rx DMA

    @return void
*/
void ethGMAC_setDmaRxEn(BOOL bEnable)
{
	T_ETH_DMA_CH0_RX_CTRL_REG rxCtrlReg = {0};

	rxCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS);
	if (bEnable) {
		rxCtrlReg.bit.SR = 1;
	} else {
		rxCtrlReg.bit.SR = 0;
	}
	ETH_SETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS, rxCtrlReg.reg);
}

/*
    setup DMA receive buffer size

    All rx descriptor buffer should have the same size.

    @param[in] uiSize       receive buffer size (unit: byte) (must 16 byte alignment)

    @return void
*/
void ethGMAC_setDmaRxBufSize(UINT32 uiSize)
{
	T_ETH_DMA_CH0_RX_CTRL_REG rxCtrlReg = {0};

	if( nvt_get_chip_id() == CHIP_NA51084) {
		if (uiSize & (ETHMAC_SYSBUS_WIDTH_MASK|0x4)) {
			DBG_ERR("rx buf size should 8 byte align, but %d\r\n", uiSize);
		}
	} else {
		if (uiSize & ETHMAC_SYSBUS_WIDTH_MASK) {
			DBG_ERR("rx buf size should 4 byte align, but %d\r\n", uiSize);
		}
	}

	rxCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS);
	//rxCtrlReg.bit.RBSZ = uiSize / ETHMAC_SYSBUS_WIDTH_SIZE;
	rxCtrlReg.bit.RBSZ = uiSize;
	ETH_SETREG(ETH_DMA_CH0_RX_CTRL_REG_OFS, rxCtrlReg.reg);
}

/*
    setup DMA TSO enable

    @param[in] bEnable      enable/disable TSO
                            - @b TRUE: enable TSO
                            - @b FALSE: disable TSO

    @return void
*/
void ethGMAC_setDmaTsoEn(BOOL bEnable)
{
	T_ETH_DMA_CH0_TX_CTRL_REG txCtrlReg = {0};

	txCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS);
	if (bEnable) {
		txCtrlReg.bit.TSE = 1;
		txCtrlReg.bit.UFO = 0x01;
		//txCtrlReg.bit.UFO = 0x10;
	} else {
		txCtrlReg.bit.TSE = 0;
	}
	ETH_SETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS, txCtrlReg.reg);
}

/*
    setup DMA transmit enable

    @param[in] bEnable      enable/disable tx DMA
                            - @b TRUE: enable tx DMA
                            - @b FALSE: disable tx DMA

    @return void
*/
void ethGMAC_setDmaTxEn(BOOL bEnable)
{
	T_ETH_DMA_CH0_TX_CTRL_REG txCtrlReg = {0};

	txCtrlReg.reg = ETH_GETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS);
	txCtrlReg.bit.OSF = 1;
	if (bEnable) {
		txCtrlReg.bit.ST = 1;
	} else {
		txCtrlReg.bit.ST = 0;
	}
	ETH_SETREG(ETH_DMA_CH0_TX_CTRL_REG_OFS, txCtrlReg.reg);
}

/*
    init MAC interrupt enable

    @return void
*/
void ethGMAC_initMacIntEn(void)
{
	T_ETH_INT_EN_REG intEnReg = {0};

	//intEnReg.bit.RGSMIIIE = 1;  // for RGMII LNKSTS
	intEnReg.bit.LPIIE = 1;     // for EEE LPI
	intEnReg.bit.PMTIE = 1;

	ETH_SETREG(ETH_INT_EN_REG_OFS, intEnReg.reg);
}

/*
    init DMA interrupt enable

    @return void
*/
void ethGMAC_initDmaIntEn(void)
{
	T_ETH_DMA_CH0_INTEN_REG intEnReg = {0};

	intEnReg.bit.TIE = 1;
	intEnReg.bit.TXSE = 1;
	intEnReg.bit.TBUE = 1;

	intEnReg.bit.RIE = 1;
	intEnReg.bit.RBUE = 1;
	intEnReg.bit.RSE = 1;

	intEnReg.bit.FBEE = 1;
	intEnReg.bit.AIE = 1;
	intEnReg.bit.NIE = 1;

	ETH_SETREG(ETH_DMA_CH0_INTEN_REG_OFS, intEnReg.reg);
}

/*
    resume DMA transmit

    @return void
*/
void ethGMAC_resumeDmaTx(UINT32 uiDescAddr)
{
	//DBG_WRN("0x%lx\r\n", uiDescAddr);
	ETH_SETREG(ETH_DMA_CH0_TxDesc_TAIL_PNTR_REG_OFS, uiDescAddr);
}

/*
    update DMA rx tail pointer

    @return void
*/
void ethGMAC_updateDmaRxTailPtr(UINT32 uiDescAddr)
{
	ETH_SETREG(ETH_DMA_CH0_RxDesc_TAIL_PNTR_REG_OFS, uiDescAddr);
}


/*
    enable rmii reference clock output

    @return void
*/
void ethGMAC_rmiiRefClkOutputEn(void)
{
	T_ETH_IM_IO_CTRL_REG IOReg;
	IOReg.reg = ETH_GETREG(ETH_IM_IO_CTRL_REG_OFS);

	IOReg.bit.RMII_REF_CLK_O_EN = 1;

	ETH_SETREG(ETH_IM_IO_CTRL_REG_OFS, IOReg.reg);
}

/*
    enable rmii reference clock input

    @return void
*/
void ethGMAC_rmiiRefClkInputEn(void)
{
	T_ETH_IM_IO_CTRL_REG IOReg;
	IOReg.reg = ETH_GETREG(ETH_IM_IO_CTRL_REG_OFS);

	IOReg.bit.RMII_REF_CLK_I_EN = 1;

	ETH_SETREG(ETH_IM_IO_CTRL_REG_OFS, IOReg.reg);
}

/*
    set interrupt mode

    @return void
*/
void ethGMAC_setIntMode(ETHMAC_DMA_INTMODE_ENUM mode)
{
	T_ETH_DMA_MODE_REG IOReg;
	IOReg.reg = ETH_GETREG(ETH_DMA_MODE_REG_OFS);

	IOReg.bit.INTM = mode;

	ETH_SETREG(ETH_DMA_MODE_REG_OFS, IOReg.reg);
}

/*
    enable external phy path

    @return void
*/
#ifdef ETH_EXTERNAL_PHY
void ethGMAC_externalPhyEn(void)
{
	T_ETH_IM_CTRL_EN_REG imCtrlReg;
	T_ETH_IM_IO_CTRL_REG IOReg;
    imCtrlReg.reg = ETH_GETREG(ETH_IM_CTRL_EN_REG_OFS);
	imCtrlReg.bit.ETH_PHY_SEL = 1;
	if( nvt_get_chip_id() == CHIP_NA51055) {
		imCtrlReg.bit.RMII_OUTPUT_PHASE = 1; // RMII_OUTPUT_PHASE = Raising
	} else {
		imCtrlReg.bit.RMII_OUTPUT_PHASE = 0; // RMII_OUTPUT_PHASE = Raising
	}
	ETH_SETREG(ETH_IM_CTRL_EN_REG_OFS, imCtrlReg.reg);

	IOReg.reg = ETH_GETREG(ETH_IM_IO_CTRL_REG_OFS);
    IOReg.bit.TXD_SRC = 0;
#ifdef ETH_EXTERNAL_CLK
	IOReg.bit.EXT_RMII_REF_EN = 1;
#else
	IOReg.bit.EXT_RMII_REF_EN = 0;
#endif
	ETH_SETREG(ETH_IM_IO_CTRL_REG_OFS, IOReg.reg);
}
#endif

