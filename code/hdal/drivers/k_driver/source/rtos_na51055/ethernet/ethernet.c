/*
    Ethernet module driver

    This file is the driver of Ethernet module

    @file       ethernet.c
    @ingroup    mIDrvIO_Eth
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#include <string.h>
//#include "SysKer.h"
#include "ethernet_protected.h"
#include "ethernet_reg.h"
#include "ethernet_int.h"
#include "eth_gmac.h"
#include "eth_phy.h"
#include "eth_descript.h"
#include "eth_platform.h"
#include "dma.h"
#include "dma_protected.h"
#include "pll_protected.h"

#include "gpio.h"
//#include "Utility.h"

//
// Internal prototypes
//

static void eth_initMAC(void);
static void eth_initDMA(void);
static void eth_setLoopbackMode(ETH_LOOPBACK_MODE_ENUM mode);
static void eth_dispatchEvent(void *parameters);
static void eth_initIMMac(void);
static void eth_setLPI(UINT32 lpi_en);
static void eth_setPMT(UINT32 lpi_en);


static BOOL bOpened = FALSE;
static UINT8 vDefaultMacAddr[] = ETH_DEFAULT_MAC_ADDR;

static ETH_LOOPBACK_MODE_ENUM loopBackMode = ETH_LOOPBACK_MODE_DISABLE;
static EXTPHY_CONTEXT phyContext = {0};

static _ALIGNED(32) UINT32 vTxDescriptBuf[ETH_TX_DESC_QUEUE_LENGTH * ETH_DESC_WORD_SIZE];
//static _ALIGNED(4) UINT32 vTxDescriptBuf[ETH_TX_DESC_QUEUE_LENGTH*ETH_DESC_WORD_SIZE];
static _ALIGNED(32) UINT32 vRxDescriptBuf[ETH_RX_DESC_QUEUE_LENGTH * ETH_DESC_WORD_SIZE];
//static _ALIGNED(4) UINT32 vRxDescriptBuf[ETH_RX_DESC_QUEUE_LENGTH*ETH_DESC_WORD_SIZE];

static UINT32 uiEthMemPool = 0;
static UINT32 uiTxBufBase = 0;      // buffer to store transmit frames
static UINT32 uiRxBufBase = 0;      // buffer to store received frames
static UINT32 uiRxFrameBufBase = 0; // buffer to store 1 frame to upper layer
static UINT32 uiRxFrameLength = 0;  // store frame length of uiRxFrameBufBase

static UINT32 phyID = 0;
static ETH_CALLBACK_HDL pEthCallback = NULL;

static UINT32 phy_rst_byps = 0;

/*
    Ethernet install ID

    @return void
*/
void eth_init(void)
{
	eth_platform_create_resource();
}


/*
    Ethernet interrupt handler

    @return void
*/
void eth_isr(void)
{
	eth_platform_int_disable(0);
	ETH_GETREG(ETH_DMA_CH0_STS_REG_OFS);

	eth_platform_flg_set(0, ETH_FLGPTN_ISR);
}

/*
    ethernet open

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_open(void)
{
	DBG_DUMP("%s\r\n", __func__);

	eth_init();

	if (bOpened == TRUE) {
		DBG_ERR("eth already opened\r\n");
		return E_SYS;
	}

	if (uiEthernetTaskID == 0) {
		DBG_ERR("ethernet driver ID never installed!!\r\n");
		return E_SYS;
	}

	if (uiEthMemPool == 0) {
		DBG_ERR("ethernet driver memory never installed!!\r\n");
		return E_SYS;
	}

	// 0. setup pinux and asynchronous reset
	// (pinmux should be setup in PinmuxCfg.c)
	pll_enableSystemReset(ETH_RSTN);
	pll_disableSystemReset(ETH_RSTN);
	//pll_disableSystemReset(ETH_AXIB_RSTN);

#if defined(_NVT_FPGA_)
	nvt_disable_sram_shutdown(ETH_SD);
#endif

	// 1. Enable module clock
	pll_enableClock(ETH_CLK);

#ifdef ETH_EXTERNAL_PHY
	ethGMAC_externalPhyEn();
	pll_enableClock(ETHPHY_CLK);
#ifndef ETH_EXTERNAL_CLK
	ethGMAC_rmiiRefClkOutputEn();
#endif
	if(!phy_rst_byps){
		gpio_clearPin(L_GPIO_22);
		eth_platform_delay_ms(200);
		gpio_setPin(L_GPIO_22);
		eth_platform_delay_ms(20);
		gpio_clearPin(L_GPIO_22);
		eth_platform_delay_ms(20);
		gpio_setPin(L_GPIO_22);
		eth_platform_delay_ms(200);
		ethGMAC_rmiiRefClkInputEn();
	}else DBG_DUMP("eth:ext phy reset bypass\r\n");
#else
	if(!phy_rst_byps){
		// embeded phy async rest
		pll_enableSystemReset(ETH_PHY_HI_RSTN);
		pll_enableSystemReset(ETH_PHY_RSTN);
		eth_platform_delay_ms(10);

		pll_disableSystemReset(ETH_PHY_HI_RSTN);
		pll_disableSystemReset(ETH_PHY_RSTN);
		eth_platform_delay_ms(20);
		ethInternalPHY_powerOn();
	} else DBG_DUMP("eth:embd phy reset bypass\r\n");
#endif

	eth_platform_flg_clear(0, FLGPTN_BIT_ALL);
	eth_platform_sta_tsk(0);

	// Wait task really started
	eth_platform_flg_wait(0, ETH_FLGPTN_TASK_CREATED);

	eth_initIMMac();             // Init Ethernet Controller

	// 2. S/W reset
	ethGMAC_reset();

	// 3. setup MAC address
	ethGMAC_setMacAddress(ETH_MAC_ID_0, vDefaultMacAddr);

	// 4. initialize ethernet PHY
	ethGMAC_initMDCclock();     // setup MAC side clock divider
	if (ethPHY_open() != E_OK) { // send command to open external PHY
		DBG_ERR("open external phy fail\r\n");
		return E_OK;
	}
	eth_platform_sta_phy_chk_tsk(0);

	// 5. initialize GMAC
	eth_initMAC();
	eth_initDMA();

	// 6. Setup interrupt
	ethGMAC_disableAllMMCIntEn();   // disable MMC related interrupts
	ethGMAC_initMacIntEn();         // enable interrupts of MACIS
	ethGMAC_initDmaIntEn();         // enable interrupts of DC0IS

//#if (_EMULATION_ON_CPU2_ == ENABLE)
	{
		// manually disable INT_TO_CPU1[ethernet]
//        volatile UINT32 *pCpu1Int = (UINT32*)0xC0080004;

		//HH: should modify to GIC
//        *pCpu1Int &= ~(1<<2);
	}
//#endif

	eth_platform_int_enable(0);

	// 7. apply configurations
	eth_setLoopbackMode(loopBackMode);

	// 8. Start tx/rx
	ethGMAC_setDmaRxEn(TRUE);
	ethGMAC_setDmaTxEn(TRUE);

	bOpened = TRUE;

	return E_OK;
}

/*
    ethernet close

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_close(void)
{
	if (bOpened == FALSE) {
		DBG_ERR("eth already closed\r\n");
	}

	eth_platform_flg_set(0, ETH_FLGPTN_TASK_STOP_REQ);

	// Wait task really stopped
	eth_platform_flg_wait(0, ETH_FLGPTN_TASK_STOPPED);

	eth_platform_ter_tsk(0);

	eth_platform_ter_phy_chk_tsk(0);

	// Close external phy
	ethPHY_close();

	pll_disableClock(ETH_CLK);

#if defined(_NVT_FPGA_)
//	    pll_enableSramShutDown(ETH_RSTN);
	//pinmux_enableSramShutDown(ETH_SD);
#endif

	bOpened = FALSE;

	return E_OK;
}

/*
    ethernet send

    @param[in] pDstAddr         pointer to destination address array
    @param[in] type             type of payload
    @param[in] uiLen            payload data length to be sent
    @param[in] pData            pointer to data (payload) to be sent

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_send(UINT8 *pDstAddr, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData)
{
	ER erReturn;
	ETH_FRAME_HEAD frameHead = {0};
	T_ETH_DMA_DBG_STS0_REG dmaDbgSts0Reg = {0};

	if ((type == ETH_PAYLOAD_RAW) && (uiLen > 0x600)) {
		DBG_ERR("raw frame with length > 0x600: 0x%lx\r\n", uiLen);
		return E_SYS;
	}

	if (pDstAddr == NULL) {
		DBG_ERR("Destination mac addr pointer NULL\r\n");
		return E_SYS;
	}
	if (pData == NULL) {
		DBG_ERR("Data pointer NULL\r\n");
		return E_SYS;
	}

	eth_platform_flg_clear(0, ETH_FLGPTN_TASK_TX_DONE);

	if (ethDesc_isTxFull() == TRUE) {
		DBG_ERR("Tx queue full!!\r\n");
		return E_SYS;
	}

	// 1. Ensure gmac is stopped/suspended
	dmaDbgSts0Reg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
	if (dmaDbgSts0Reg.bit.TPS0 == 0) {        // stopped
	} else if (dmaDbgSts0Reg.bit.TPS0 == 6) { // suspended
	} else {
		DBG_ERR("Transmit Process State 0x%lx: not stop/suspend\r\n", dmaDbgSts0Reg.bit.TPS0);
		// currently still not implement error handle (bcz this is test fw)
	}

	// 2. Insert transmitted data to descriptor buffer
	memcpy(frameHead.dstAddr, pDstAddr, sizeof(frameHead.dstAddr));
	memcpy(frameHead.srcAddr, vDefaultMacAddr, sizeof(frameHead.srcAddr));
	if (type == ETH_PAYLOAD_RAW) {
		frameHead.typeLenMsb = uiLen >> 8;
		frameHead.typeLenLsb = uiLen & 0xFF;
	} else if (type == ETH_PAYLOAD_ARP) {
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x06;
	} else if (type == ETH_PAYLOAD_TCP) {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;
	} else if (type == ETH_PAYLOAD_UDP) {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;
	} else {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;

		// TODO: split a large (>0x600) IP packet to several eth frame??
	}

//    frameHead.vlanTPIDMsb = 0x81;
//    frameHead.vlanTPIDLsb = 0x00;
	erReturn = ethDesc_insertTxFrame(&frameHead, type, uiLen, pData);
	if (erReturn != E_OK) {
		DBG_ERR("Insert tx queue fail\r\n");
		return E_SYS;
	}


	// 3. Set gmac to running state
	gpio_setPin(D_GPIO_2);
	gpio_clearPin(D_GPIO_2);
	if (dmaDbgSts0Reg.bit.TPS0 == 0) {        // stopped
		ethGMAC_setDmaTxEn(TRUE);
	} else { // if (dmaStsReg.bit.TS == 6)     // suspended
//        T_ETH_CURR_HOST_TX_DESCRIPT_REG currTxDesc = {0};

//        currTxDesc.reg = ETH_GETREG(ETH_CURR_HOST_TX_DESCRIPT_REG_OFS);
//        DBG_IND("Curr descript addr: 0x%lx\r\n", currTxDesc.bit.CURR_TX_DESCRIPT_ADDR);

		ethGMAC_resumeDmaTx(dma_getPhyAddr(ethDesc_getLastTxDescAddr()) + sizeof(ETH_TX_DESC));
	}

	if (0) {
		UINT32 ran = randomUINT32();

		if (ran & 0x01) {
			DBG_ERR("disable DMA\r\n");
			dma_disableChannel(DMA_CH_Ethernet);
		}
		eth_platform_delay_ms(ran & 0xFFF);
		dma_enableChannel(DMA_CH_Ethernet);
	}


	// 5. Wait dma done
	return eth_platform_flg_wait(0, ETH_FLGPTN_TASK_TX_DONE);
}

/*
    ethernet send prepare

    @param[in] pDstAddr         pointer to destination address array
    @param[in] type             type of payload
    @param[in] uiLen            payload data length to be sent
    @param[in] pData            pointer to data (payload) to be sent

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_send_prepare(UINT8 *pDstAddr, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData)
{
	ER erReturn;
	ETH_FRAME_HEAD frameHead = {0};
	T_ETH_DMA_DBG_STS0_REG dmaDbgSts0Reg = {0};

	if ((type == ETH_PAYLOAD_RAW) && (uiLen > 0x600)) {
		DBG_ERR("raw frame with length > 0x600: 0x%lx\r\n", uiLen);
		return E_SYS;
	}

	if (pDstAddr == NULL) {
		DBG_ERR("Destination mac addr pointer NULL\r\n");
		return E_SYS;
	}
	if (pData == NULL) {
		DBG_ERR("Data pointer NULL\r\n");
		return E_SYS;
	}

	eth_platform_flg_clear(0, ETH_FLGPTN_TASK_TX_DONE);

	if (ethDesc_isTxFull() == TRUE) {
		DBG_ERR("Tx queue full!!\r\n");
		return E_SYS;
	}

	// 1. Ensure gmac is stopped/suspended
	dmaDbgSts0Reg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
	if (dmaDbgSts0Reg.bit.TPS0 == 0) {        // stopped
	} else if (dmaDbgSts0Reg.bit.TPS0 == 6) { // suspended
	} else {
		DBG_ERR("Transmit Process State 0x%lx: not stop/suspend\r\n", dmaDbgSts0Reg.bit.TPS0);
		// currently still not implement error handle (bcz this is test fw)
	}

	// 2. Insert transmitted data to descriptor buffer
	memcpy(frameHead.dstAddr, pDstAddr, sizeof(frameHead.dstAddr));
	memcpy(frameHead.srcAddr, vDefaultMacAddr, sizeof(frameHead.srcAddr));
	if (type == ETH_PAYLOAD_RAW) {
		frameHead.typeLenMsb = uiLen >> 8;
		frameHead.typeLenLsb = uiLen & 0xFF;
	} else if (type == ETH_PAYLOAD_ARP) {
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x06;
	} else if (type == ETH_PAYLOAD_TCP) {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;
	} else if (type == ETH_PAYLOAD_UDP) {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;
	} else {
		// IP datagram
		frameHead.typeLenMsb = 0x08;
		frameHead.typeLenLsb = 0x00;

		// TODO: split a large (>0x600) IP packet to several eth frame??
	}

//    frameHead.vlanTPIDMsb = 0x81;
//    frameHead.vlanTPIDLsb = 0x00;
	erReturn = ethDesc_insertTxFrame(&frameHead, type, uiLen, pData);
	if (erReturn != E_OK) {
		DBG_ERR("Insert tx queue fail\r\n");
		return E_SYS;
	}

	return E_OK;
}

/*
    ethernet send and wait

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_sendandwait(void)
{
	T_ETH_DMA_DBG_STS0_REG dmaDbgSts0Reg = {0};
	dmaDbgSts0Reg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);

	// 3. Set gmac to running state
	gpio_setPin(D_GPIO_2);
	gpio_clearPin(D_GPIO_2);
	if (dmaDbgSts0Reg.bit.TPS0 == 0) {        // stopped
		ethGMAC_setDmaTxEn(TRUE);
	} else { // if (dmaStsReg.bit.TS == 6)     // suspended
//        T_ETH_CURR_HOST_TX_DESCRIPT_REG currTxDesc = {0};

//        currTxDesc.reg = ETH_GETREG(ETH_CURR_HOST_TX_DESCRIPT_REG_OFS);
//        DBG_IND("Curr descript addr: 0x%lx\r\n", currTxDesc.bit.CURR_TX_DESCRIPT_ADDR);

		ethGMAC_resumeDmaTx(dma_getPhyAddr(ethDesc_getLastTxDescAddr()) + sizeof(ETH_TX_DESC));
	}

	if (0) {
		UINT32 ran = randomUINT32();

		if (ran & 0x01) {
			DBG_ERR("disable DMA\r\n");
			dma_disableChannel(DMA_CH_Ethernet);
		}
		eth_platform_delay_ms(ran & 0xFFF);
		dma_enableChannel(DMA_CH_Ethernet);
	}


	// 5. Wait dma done
	return eth_platform_flg_wait(0, ETH_FLGPTN_TASK_TX_DONE);
}

/*
    ethernet send

    @param[in] uiBufSize        buffer size of pData
    @param[in] puiLen           payload data received
    @param[in] pData            pointer to data (payload) to be sent

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_waitRcv(UINT32 uiBufSize, UINT32 *puiLen, UINT8 *pData)
{
	//ER erReturn;

	// Wait rx done
#if defined(_NVT_EMULATION_)
	eth_platform_flg_wait(0, ETH_FLGPTN_TASK_RX_DONE);
#endif
	/*
	if (erReturn != E_OK) {
		DBG_ERR("wai_flg err: 0x%lx\r\n", erReturn);
		return erReturn;
	}
	*/
	DBG_WRN("wait flag ok\r\n");

	// Get rx frame
	memcpy(pData, (void *)uiRxFrameBufBase, uiRxFrameLength);
	*puiLen = uiRxFrameLength;
//    ethDesc_retrieveRxFrame(uiBufSize, puiLen, pData);

	return E_OK;
}

/*
    ethernet set config

    @note For test usage. can be invoke when driver closed

    @param[in] configID         configuration ID
    @param[in] uiConfig         configuration value for configID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_setConfig(ETH_CONFIG_ID configID, UINT32 uiConfig)
{
	switch (configID) {
	case ETH_CONFIG_ID_SET_MEM_REGION:
		uiEthMemPool = uiConfig;
		return E_OK;
	case ETH_CONFIG_ID_MAC_ADDR: {
			UINT8 *pMacAddr = (UINT8 *)uiConfig;

			if (pMacAddr == NULL) {
				DBG_ERR("input mac addr buffer is NULL\r\n");
				return E_SYS;
			}
			memcpy(vDefaultMacAddr, pMacAddr, sizeof(vDefaultMacAddr));
		}
		if (bOpened) {
			// re-install to mac
			ethGMAC_setMacAddress(ETH_MAC_ID_0, vDefaultMacAddr);
		}
		break;
	case ETH_CONFIG_ID_LOOPBACK:
		loopBackMode = uiConfig;
		if (bOpened) {
			eth_setLoopbackMode(loopBackMode);
		}
		return E_OK;

	case ETH_CONFIG_ID_WRITE_DETECT_TEST: {
/*
			DMA_WRITEPROT_ATTR  ProtectAttr = {0};
			const UINT32        uiSet = DMA_WPSET_0;
			UINT8              *pBuf;


			memset(&ProtectAttr.chEnMask, 0, sizeof(ProtectAttr.chEnMask));
			ProtectAttr.chEnMask.bETHERNET = TRUE;
//            ProtectAttr.uiProtectSrc = DMA_WPSRC_DMA;

			pBuf = (UINT8 *)(uiRxBufBase);
			ProtectAttr.uiProtectlel = DMA_WPLEL_UNWRITE;
//            ProtectAttr.uiProtectlel = DMA_WPLEL_DETECT;
//			pBuf = (UINT8 *)(uiTxBufBase);
//			ProtectAttr.uiProtectlel = DMA_RPLEL_UNREAD;
//			ProtectAttr.uiProtectlel = DMA_RWPLEL_UNRW;
			ProtectAttr.uiStartingAddr = (UINT32)pBuf;
			ProtectAttr.uiSize = ETH_RX_DESC_QUEUE_LENGTH * ETH_DESC_BUF_SIZE * 2;

			if (uiSet == DMA_WPSET_0) {
				dma_configWPFunc(uiSet, &ProtectAttr, NULL);
			} else if (uiSet == DMA_WPSET_1) {
				dma_configWPFunc(uiSet, &ProtectAttr, NULL);
			} else {
				dma_configWPFunc(uiSet, &ProtectAttr, NULL);
			}

			if (uiConfig == TRUE) {
				dma_enableWPFunc(uiSet);
			} else {
				dma_disableWPFunc(uiSet);
			}
*/
		}

		return E_OK;
	case ETH_CONFIG_ID_SPEED: {
			switch (uiConfig) {
			case 100:
				phyContext.network_speed = ETHPHY_SPD_100;
				ethPHY_setSpeed(ETHMAC_SPD_100);
				ethGMAC_setSpeed(ETHMAC_SPD_100);
				break;
			case 1000:
				phyContext.network_speed = ETHPHY_SPD_1000;
				ethPHY_setSpeed(ETHMAC_SPD_1000);
				ethGMAC_setSpeed(ETHMAC_SPD_1000);
				break;
			case 10:
			default:
				phyContext.network_speed = ETHPHY_SPD_10;
				ethPHY_setSpeed(ETHMAC_SPD_10);
				ethGMAC_setSpeed(ETHMAC_SPD_10);
				break;
			}
		}
		return E_OK;
	case ETH_CONFIG_ID_DUPLEX: {
			switch (uiConfig) {
			case ETH_DUPLEX_HALF:
				phyContext.duplexMode = ETHPHY_DUPLEX_HALF;
				ethGMAC_setDuplexMode(ETHMAC_DUPLEX_HALF);
				ethPHY_setDuplex(ETHMAC_DUPLEX_HALF);
				break;
			case ETH_DUPLEX_FULL:
			default:
				phyContext.duplexMode = ETHPHY_DUPLEX_FULL;
				ethGMAC_setDuplexMode(ETHMAC_DUPLEX_FULL);
				ethPHY_setDuplex(ETHMAC_DUPLEX_FULL);
				break;
			}
		}
		return E_OK;
	case ETH_CONFIG_SET_LPI: {
			eth_setLPI(uiConfig);
		}
		return E_OK;
	case ETH_CONFIG_SET_PMT: {
			eth_setPMT(uiConfig);
		}
		return E_OK;
	case ETH_CONFIG_SET_MSS: {
			ethGMAC_setDmaMSS(uiConfig);
		}
		return E_OK;
	case ETH_PHY_RST_BYPASS:{
			phy_rst_byps = 1;
			ethPHY_set(1);
		}
		return E_OK;
	default:
		break;
	}

	return E_SYS;
}

/*
    Query ethernet configuration

    @param[in] configID         Configuration identifier
    @param[in] pConfigContext   returning context for configID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER eth_getConfig(ETH_CONFIG_ID configID, UINT32 *pConfigContext)
{
	switch (configID) {
	case ETH_CONFIG_ID_MAC_ADDR: {
			if (pConfigContext == NULL) {
				DBG_ERR("input mac addr buffer is NULL\r\n");
				return E_SYS;
			}
			memcpy((void *)pConfigContext, vDefaultMacAddr, sizeof(vDefaultMacAddr));
		}
		return E_OK;
	case ETH_CONFIG_ID_LOOPBACK:
		*pConfigContext = loopBackMode;
		return E_OK;
	case ETH_CONFIG_ID_SPEED:
		switch (phyContext.network_speed) {
		case ETHPHY_SPD_100:
			*pConfigContext = 100;
			break;
		case ETHPHY_SPD_1000:
			*pConfigContext = 1000;
			break;
		default:
			*pConfigContext = 10;
			break;
		}
		return E_OK;
	case ETH_CONFIG_ID_DUPLEX:
		*pConfigContext = phyContext.duplexMode;
		return E_OK;
	default:
		break;
	}

	return E_SYS;
}

/*
    Set call back routine

    @param[in] callBackID   callback routine identifier
    @param[in] pCallBack    callback routine to be installed

    @return
        - @b E_OK: install callback success
        - @b E_NOSPT: callBackID is not valid
*/
ER eth_setCallBack(ETH_CALLBACK callBackID, ETH_CALLBACK_HDL pCallBack)
{
	UINT32 flags = 0;
	switch (callBackID) {
	case ETH_CALLBACK_RX_DONE:
		flags = eth_platform_spin_lock();

		pEthCallback = pCallBack;

		eth_platform_spin_unlock(flags);
		return E_OK;
	default:
		break;
	}

	return E_NOSPT;
}

/*
    Get PHY ID

    @return probed PHY ID
*/
UINT32 eth_getPhyID(void)
{
	phyID = ethPHY_getID();
	return phyID;
}

/*
    ethernet event dispatch

    @return void
*/
static void eth_dispatchEvent(void *parameters)
{
	T_ETH_INT_STS_REG stsReg;
	T_ETH_INT_EN_REG intenReg;
	T_ETH_DMA_CH0_STS_REG dmaStsReg;
	T_ETH_DMA_CH0_INTEN_REG dmaIntEnReg;
	T_ETH_IM_INT_STS_REG imStsReg;

	imStsReg.reg = ETH_GETREG(ETH_IM_INT_STS_REG_OFS);

	stsReg.reg = ETH_GETREG(ETH_INT_STS_REG_OFS);
	intenReg.reg = ETH_GETREG(ETH_INT_EN_REG_OFS);
	dmaStsReg.reg = ETH_GETREG(ETH_DMA_CH0_STS_REG_OFS);
	dmaIntEnReg.reg = ETH_GETREG(ETH_DMA_CH0_INTEN_REG_OFS);

	if (imStsReg.bit.GMIIPHY_STS == 1) {
		UINT32 uiData;
		DBG_WRN("GMIIPHY_STS=%d\r\n", imStsReg.bit.GMIIPHY_STS);
		//DBG_DUMP("REVMIIPHY_STS=%d\r\n",imStsReg.bit.REVMII_STS);

		ethGMAC_readExtPhy(ETH_REVMII_LOCAL_ADDR, 16, &uiData);
		DBG_WRN("uiData=%x\r\n", uiData);
		//return;
	}

	if (imStsReg.bit.LPI_STS == 1) {
		// MAC exit the Rx LPI state
		DBG_WRN("lpi_intr_o trigger !!\r\n");
		//return;
	}

	if (imStsReg.bit.PMT_STS == 1) {
		DBG_WRN("pmt_intr_o trigger !!\r\n");
		DBG_WRN("stsReg.bit.PMTIS=%d\r\n", stsReg.bit.PMTIS);
		//return;
	}

	if (stsReg.bit.LPIIS == 1) {
		T_ETH_LPI_CTRL_STS_REG lpiCtrlStsReg = {0};

		DBG_WRN("LPI sts changed ==> ");

		lpiCtrlStsReg.reg = ETH_GETREG(ETH_LPI_CTRL_STS_REG_OFS);

		if (lpiCtrlStsReg.bit.TLPIEN) {
			DBG_WRN("Enter Tx LPI\r\n");
		}
		if (lpiCtrlStsReg.bit.TLPIEX) {
			DBG_WRN("Exit Tx LPI\r\n");
		}
		if (lpiCtrlStsReg.bit.RLPIEN) {
			DBG_WRN("Enter Rx LPI\r\n");
		}
		if (lpiCtrlStsReg.bit.RLPIEX) {
			DBG_WRN("Exit Rx LPI\r\n");
		}
	}

	if (stsReg.bit.PMTIS == 1) {
		T_ETH_PMT_CTRL_STS_REG pmtCtrlStsReg = {0};

		pmtCtrlStsReg.reg = ETH_GETREG(ETH_PMT_CTRL_STS_REG_OFS);

		if (pmtCtrlStsReg.bit.PWRDWN) {
			DBG_WRN("iPWRDWN\r\n");
		}
		if (pmtCtrlStsReg.bit.MGKPRCVD) {
			DBG_WRN("iMagic packet received\r\n");
		}
		if (pmtCtrlStsReg.bit.RWKPRCVD) {
			DBG_WRN("iRemote wake-up packet received\r\n");
		}
	}

	stsReg.reg &= ~intenReg.reg;
	dmaStsReg.reg &= dmaIntEnReg.reg;
	if (stsReg.reg != 0) {
		// interrupt status in ETH_INT_STS_REG are level trigger
		// NOT need to write 1 clear
//        ETH_SETREG(ETH_INT_STS_REG_OFS, stsReg.reg);
	}
//    DBG_DUMP("%s 0x%lx\r\n", __func__, dmaStsReg.reg);
	if (dmaStsReg.reg != 0) {
		ETH_SETREG(ETH_DMA_CH0_STS_REG_OFS, dmaStsReg.reg);
	}

	if (imStsReg.bit.SBD_STS == 0) {
		DBG_WRN("SBD_STS=%d\r\n", imStsReg.bit.SBD_STS);
		//return;
	}

	if ((stsReg.reg == 0) && (dmaStsReg.reg == 0)) {
		return;
	}
	DBG_WRN("%s(%d) stsReg.reg=0x%lx  dmaStsReg.reg=0x%lx\r\n", __func__,__LINE__, stsReg.reg, dmaStsReg.reg);

	if (1) {
//	if (dmaStsReg.bit.NIS) {
		if (dmaStsReg.bit.TI) {
			// remove pending jobs from tx queue
			ethDesc_removeTxFrame();

			eth_platform_flg_set(0, ETH_FLGPTN_TASK_TX_DONE);
		}
		if (dmaStsReg.bit.TBU) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_WRN("Transmit buffer unavailable\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			if (lastStsReg.bit.TPS0 != 6) {
				DBG_WRN("Transmit Process State not 0x6: 0x%lx\r\n", lastStsReg.bit.TPS0);
			}
		}
		if (dmaStsReg.bit.RI) {
			ER ErrValue;
			dmaIntEnReg.bit.RIE = 0;
			ETH_SETREG(ETH_DMA_CH0_INTEN_REG_OFS, dmaIntEnReg.reg);
			DBG_WRN("Receive Interrupt, hdl 0%lx\r\n", pEthCallback);
			ErrValue = ethDesc_retrieveRxFrame(ETH_DESC_BUF_SIZE, &uiRxFrameLength, (void *)uiRxFrameBufBase);

			//DBG_DUMP("ErrValue %d\r\n", ErrValue);
			if (pEthCallback) {
				DBG_WRN("pEthCallback\r\n");
				if (ErrValue == 0) {
					pEthCallback(uiRxFrameBufBase, uiRxFrameLength);
				}

				while (ErrValue == 0) {
					ErrValue = ethDesc_retrieveRxFrame(ETH_DESC_BUF_SIZE, &uiRxFrameLength, (void *)uiRxFrameBufBase);
					if (ErrValue == 0) {
						pEthCallback(uiRxFrameBufBase, uiRxFrameLength);
					}
				}
			}


#if !defined(_NVT_EMULATION_)
			ethernetif_input(parameters);
			while (ErrValue == 1) {
				ErrValue = ethDesc_retrieveRxFrame(ETH_DESC_BUF_SIZE, &uiRxFrameLength, (void *)uiRxFrameBufBase);
				ethernetif_input(parameters);
			}
			dmaIntEnReg.bit.RIE = 1;
			ETH_SETREG(ETH_DMA_CH0_INTEN_REG_OFS, dmaIntEnReg.reg);
#else
			eth_platform_flg_set(0, ETH_FLGPTN_TASK_RX_DONE);
#endif
		}
		if (dmaStsReg.bit.ERI) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_WRN("Early Receive Interrupt\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			DBG_WRN("Receive Process State : 0x%lx\r\n", lastStsReg.bit.RPS0);
		}
	}

	if (dmaStsReg.bit.AIS) {
		if (dmaStsReg.bit.TPS) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_WRN("Transmit Stopped\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			if (lastStsReg.bit.TPS0 != 0) {
				DBG_WRN("Transmit Process State not zero: 0x%lx\r\n", lastStsReg.bit.TPS0);
			}
		}
		if (dmaStsReg.bit.RBU) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			// Should excercise flow control here??

			DBG_WRN("Receive buffer unavailable\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			if (lastStsReg.bit.RPS0 != 4) {
				DBG_WRN("Receive Process State not 0x4: 0x%lx\r\n", lastStsReg.bit.RPS0);
			}
		}
		if (dmaStsReg.bit.RPS) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_WRN("Receive Stopped\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			if (lastStsReg.bit.RPS0 != 0) {
				DBG_WRN("Receive Process State not zero: 0x%lx\r\n", lastStsReg.bit.RPS0);
			}
		}
		if (dmaStsReg.bit.ETI) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_WRN("Early Transmit Interrupt\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			DBG_WRN("Receive Process State : 0x%lx\r\n", lastStsReg.bit.RPS0);
		}
		if (dmaStsReg.bit.FBE) {
			T_ETH_DMA_DBG_STS0_REG lastStsReg;

			DBG_ERR("Fatal Bus Error\r\n");

			lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
			DBG_WRN("Receive Process State : 0x%lx\r\n", lastStsReg.bit.RPS0);
		}
	}

	if (dmaStsReg.bit.RWT) {
		T_ETH_DMA_DBG_STS0_REG lastStsReg;

		DBG_WRN("Receive watchdog timeout (receive frame > 2KB)\r\n");

		lastStsReg.reg = ETH_GETREG(ETH_DMA_DBG_STS0_REG_OFS);
		DBG_WRN("Receive Process State : 0x%lx\r\n", lastStsReg.bit.RPS0);
	}
}

void eqos_mac_init(PEXTPHY_CONTEXT priv)
{
    //DBG_DUMP("duplex=%d, speed=%d\r\n", priv->duplexMode, priv->network_speed);

    // Do not change the configuration when the DWC_EQOS is actively transmitting or receiving
    ethGMAC_setRxEn(DISABLE);
    ethGMAC_setTxEn(DISABLE);

    if (priv->duplexMode == ETHPHY_DUPLEX_FULL)
    {
        ethGMAC_setDuplexMode(ETHMAC_DUPLEX_FULL);
/*
        if (priv->Speed == ETHPHY_SPD_1000)
            eqos_select_gmii(priv);
        else
            eqos_select_mii(priv);
*/
        // set fast ethernet speed
        ethGMAC_setSpeed((ETHMAC_SPD_ENUM)priv->network_speed);

        /* Flow Control Configuration. */
    }
    else  // Half Duplex configuration
    {
        ethGMAC_setDuplexMode(ETHMAC_DUPLEX_HALF);
/*
        if (priv->Speed == ETHPHY_SPD_1000)
            eqos_select_gmii(priv);
        else
            eqos_select_mii(priv);
*/
        // Set fast ethernet speed.
        ethGMAC_setSpeed((ETHMAC_SPD_ENUM)priv->network_speed);

        /* Flow Control Configuration. */
    }


    if (priv->linkState == ETHPHY_LINKST_UP)
    {
        ethGMAC_setTxEn(ENABLE);
        ethGMAC_setRxEn(ENABLE);
    }
    else
    {
		ethGMAC_setRxEn(DISABLE);
		ethGMAC_setTxEn(DISABLE);
    }

    DBG_DUMP("\r\n");
    return;
}


/*
    ethernet task

    @return void
*/
void eth_task(void *parameters)
{
	FLGPTN uiFlag;

	//kent_tsk();
	eth_platform_flg_set(0, ETH_FLGPTN_TASK_CREATED);

	DBG_WRN("%s: task start\r\n", __func__);

	while (1) {
		uiFlag = eth_platform_flg_wait(0, ETH_FLGPTN_ISR | ETH_FLGPTN_TASK_STOP_REQ);

//        DBG_DUMP("%s: recf flg 0x%lx\r\n", __func__, uiFlag);

		if (uiFlag & ETH_FLGPTN_ISR) {
			eth_dispatchEvent(parameters);
			eth_platform_int_enable(0);
		} else if (uiFlag & ETH_FLGPTN_TASK_STOP_REQ) {
			eth_platform_flg_set(0, ETH_FLGPTN_TASK_STOPPED);
			eth_platform_flg_wait(0, ETH_FLGPTN_TASK_FREEZE);
			abort();
		}
	}
}

/*
    ethernet initialize MAC controller

    @return void
*/
static void eth_initMAC(void)
{
//    EXTPHY_CONTEXT phyContext = {0};

	ethPHY_getLinkCapability(&phyContext);

//    ethGMAC_setMacWatchdog(TRUE);   // only recevie frame <= 2048 bytes
//    ethGMAC_setMacJabber(TRUE);     // only transmit frame <= 2048 bytes
	ethGMAC_setFrameBurst(TRUE);    // support frame burst
	ethGMAC_setJumboFrame(FALSE);   // NOT support jumbo frame
//    ethGMAC_setReceiveOwn(TRUE);    // receive while transmitting
	ethGMAC_setLoopback(FALSE);     // Disable loopback mode
//    ethGMAC_setRetryAfterCollision(TRUE);   // Retry after collision
	ethGMAC_setCrcStrip(TRUE);     // NOT strip recevied padding/CRC field
//    ethGMAC_setupBackoffLimit(ETHMAC_BACKOFF_LIMIT_1023);
//    ethGMAC_setDeferralCheck(FALSE);        // Disable deferral check
	ethGMAC_setRecvChecksumOffload(TRUE);   // Enable receive path checksum offload

//    phyContext.duplexMode = ETHPHY_DUPLEX_FULL;
	if (phyContext.duplexMode == ETHPHY_DUPLEX_FULL) {
		ethGMAC_setDuplexMode(ETHMAC_DUPLEX_FULL);
	} else {
		ethGMAC_setDuplexMode(ETHMAC_DUPLEX_HALF);
	}

	ethGMAC_setTxEn(TRUE);          // Enable transmission
	ethGMAC_setRxEn(TRUE);          // Enable reception

	switch (phyContext.network_speed) {
	case ETHMAC_SPD_100:
		ethGMAC_setSpeed(ETHMAC_SPD_100);
		break;
	case ETHMAC_SPD_1000:
		ethGMAC_setSpeed(ETHMAC_SPD_1000);
		break;
	case ETHMAC_SPD_10:
	default:
		ethGMAC_setSpeed(ETHMAC_SPD_10);
		break;
	}

	// Rx Frame filter configuration
	ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_NORMAL);
	ethGMAC_setRxCtrlFrameFilter(ETHMAC_CTRL_FRAME_FILTER_ALL);
	ethGMAC_setRxBroadcast(TRUE);
	ethGMAC_setRxSrcAddrFilter(FALSE);
	ethGMAC_setRxMulticastFilter(ETHMAC_MULTICAST_FILTER_MODE_PERFECT);
	ethGMAC_setRxDstAddrFilterMode(ETHMAC_DST_ADDR_FILTER_MODE_NORMAL);
	ethGMAC_setRxUnicastHash(FALSE);

	// Flow control configuration
	ethGMAC_setUnicastPauseFrame(FALSE);
	ethGMAC_setRxFlowControl(TRUE);
	ethGMAC_setTxFlowControl(TRUE);
}

/*
    ethernet initialize DMA (in GMAC) controller

    @return void
*/
static void eth_initDMA(void)
{
//    uiTxBufBase = ALIGN_CEIL_16((uiEthMemPool+32)); // AXI bus width is 128 bit (16 byte), adjust tx buffer to 16 byte alignment
	uiRxFrameBufBase = uiEthMemPool + 32;        // data buff not require alignment
#if 0   // Test AXI 4K boundary
	uiTxBufBase = uiRxFrameBufBase + ETH_DESC_BUF_SIZE * 4 + 4096 * 2;
	uiTxBufBase /= 4096;
	uiTxBufBase *= 4096;
	uiTxBufBase -= 256;
#else
	uiTxBufBase = uiRxFrameBufBase + ETH_DESC_BUF_SIZE * 4;
#endif
	uiRxBufBase = uiTxBufBase + ETH_TX_DESC_QUEUE_LENGTH * ETH_DESC_BUF_SIZE * 2 + 128 + ETH_TX_DESC_QUEUE_LENGTH * 2;
	//uiRxBufBase = uiTxBufBase + ETH_TX_DESC_QUEUE_LENGTH*ETH_DESC_BUF_SIZE*2 + 128;

	ethGMAC_setDmaRxBufSize(ETH_DESC_BUF_SIZE);
	ethDesc_initQueue(ETHMAC_DMA_DESCRIPT_MODE_RING,
					  (UINT32)(dma_getCacheAddr(((UINT32)vTxDescriptBuf))),
					  uiTxBufBase,
					  ETHMAC_DMA_DESCRIPT_MODE_RING,
					  (UINT32)(dma_getCacheAddr(((UINT32)vRxDescriptBuf))),
					  uiRxBufBase
					 );
	ethGMAC_setDmaTxDescAddr((UINT32)(dma_getPhyAddr(((UINT32)vTxDescriptBuf))));
	ethGMAC_setDmaRxDescAddr((UINT32)(dma_getPhyAddr(((UINT32)vRxDescriptBuf))));

	if( nvt_get_chip_id() == CHIP_NA51084) {
		ethGMAC_setDmaAAL(FALSE);
		ethGMAC_setFixedBurst(FALSE);
	} else {
		ethGMAC_setDmaAAL(TRUE);
		ethGMAC_setFixedBurst(TRUE);
	}


	ethGMAC_setDmaDescSkipLen((ETH_DESC_WORD_SIZE * sizeof(UINT32) - 16) / sizeof(UINT32));
//    ethGMAC_setDmaDescSkipLen((ETH_DESC_WORD_SIZE*sizeof(UINT32)-16)/(ETHMAC_SYSBUS_WIDTH_SIZE/sizeof(UINT32)));
	ethGMAC_setDmaBurst(ETHMAC_DMA_BURST_64);  // 96510 setting /* FIFO Size(2048) / Data Width(32/8) / 2 = 256 */
	/* Packet Size < TXFIFO_SIZE !V ((PBL + N)*(DATAWIDTH/8)) */
	/* Packet Size < 2048 !V ((PBL + N)*(32/8)) ==> PBL: 64 */
	ethGMAC_setDmaRxThreshold(128);
	ethGMAC_setDmaRxMode(ETHMAC_DMA_RXMODE_STORE_FORWARD);
	ethGMAC_setDmaTxMode(ETHMAC_DMA_TXMODE_STORE_FORWARD);

//    ethGMAC_setDmaMSS(1000-40);
	ethGMAC_setDmaMSS(1500 - 40);
	ethGMAC_setDmaTsoEn(TRUE);
	//if( nvt_get_chip_id() == CHIP_NA51084) {
		ethGMAC_setIntMode(ETHMAC_DMA_INTMODE_LEVEL);
	//}
}

/*
    ethernet set loopback mode

    @param[in] mode     Loopback mode

    @return void
*/
static void eth_setLoopbackMode(ETH_LOOPBACK_MODE_ENUM mode)
{
	switch (mode) {
	case ETH_LOOPBACK_MODE_DISABLE:
		ethGMAC_setLoopback(FALSE);
//        ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_DISABLE);
		ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_NORMAL);
		ethPHY_setLoopback(FALSE, TRUE);
		break;
	case ETH_LOOPBACK_MODE_MAC:
		ethGMAC_setDuplexMode(ETHMAC_DUPLEX_FULL);  // synopsys MAC loopback requires full duplex
		ethGMAC_setLoopback(TRUE);
//        ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_NORMAL);
		ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_DISABLE);
		ethPHY_setLoopback(FALSE, TRUE);
		break;
	case ETH_LOOPBACK_MODE_EXTERNAL:
		ethGMAC_setLoopback(FALSE);
		ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_DISABLE);
		ethPHY_setLoopback(FALSE, FALSE);
		break;
	case ETH_LOOPBACK_MODE_PHY:
	default:
		ethGMAC_setLoopback(FALSE);
		ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_DISABLE);
		ethPHY_setLoopback(TRUE, FALSE);
		break;
	}
}

static void eth_initIMMac(void)
{
	T_ETH_IM_DEBUG_CTRL_REG imDebugCtrlReg;
	UINT32 flags = 0;

	imDebugCtrlReg.reg = ETH_GETREG(ETH_IM_DEBUG_CTRL_REG_OFS);
	imDebugCtrlReg.bit.LOCAL_ADDR = ETH_REVMII_LOCAL_ADDR;
	imDebugCtrlReg.bit.REMOTE_ADDR = ETH_REVMII_REMOTE_ADDR;
	flags = eth_platform_spin_lock();
	ETH_SETREG(ETH_IM_DEBUG_CTRL_REG_OFS, imDebugCtrlReg.reg);
	eth_platform_spin_unlock(flags);
	//DBG_DUMP("[Check !!]ETH_IM_DEBUG_CTRL_REG_OFS = 0x%lx\r\n", ETH_GETREG(ETH_IM_DEBUG_CTRL_REG_OFS));
}

static void eth_setLPI(UINT32 lpi_en)
{
	T_ETH_LPI_CTRL_STS_REG lpictrlReg;
	T_ETH_LPI_TIMERS_CTRL_REG lpitimerReg;
	T_ETH_1US_TIC_CNTR_REG lpiticReg;
	T_ETH_LPI_ENTRY_TIMER_REG lpientryReg;
	UINT32 flags = 0;

	DBG_DUMP("%s: %s\r\n", __func__, lpi_en ? "Enable" : "Disable");
	flags = eth_platform_spin_lock();
	lpictrlReg.reg = ETH_GETREG(ETH_LPI_CTRL_STS_REG_OFS);
	lpictrlReg.bit.LPIEN   = lpi_en;
	lpictrlReg.bit.LPITXA  = lpi_en;
	lpictrlReg.bit.LPIATE  = lpi_en;
	lpictrlReg.bit.PLS     = lpi_en;  // Suppose that PHY is up
	lpictrlReg.bit.LPITCSE = 1;
	ETH_SETREG(ETH_LPI_CTRL_STS_REG_OFS, lpictrlReg.reg);


	lpitimerReg.reg = ETH_GETREG(ETH_LPI_TIMERS_CTRL_REG_OFS);
	lpitimerReg.bit.TWT = 0x200;
	ETH_SETREG(ETH_LPI_TIMERS_CTRL_REG_OFS, lpitimerReg.reg);

	lpiticReg.reg = ETH_GETREG(ETH_1US_TIC_CNTR_REG_OFS);
	lpiticReg.bit.TIC_1US_CNTR = 7;
	ETH_SETREG(ETH_1US_TIC_CNTR_REG_OFS, lpiticReg.reg);

	lpientryReg.reg = ETH_GETREG(ETH_LPI_ENTRY_TIMER_REG_OFS);
	lpientryReg.bit.LPIET = 0x80;
	ETH_SETREG(ETH_LPI_ENTRY_TIMER_REG_OFS, lpientryReg.reg);

	eth_platform_spin_unlock(flags);
}


UINT32 rwuFilter [16] = {
	0x03C00803,  // Filter 0 Byte Mask
	0x3C000203,  // Filter 1 Byte Mask
	0x00000000,  // Filter 2 Byte Mask
	0x00000000,  // Filter 3 Byte Mask
	0x00000909,  // Filter 0~3 Command
	0x00000C0C,  // Filter 0~3 Offset
	0xC6407EF1,  // Filter 0 1 CRC
	0x00000000,  // Filter 2 3 CRC

	0xDEAD0010,  // Filter 4 Byte Mask
	0xDEAD0020,  // Filter 5 Byte Mask
	0xDEAD0030,  // Filter 6 Byte Mask
	0xDEAD0040,  // Filter 7 Byte Mask
	0xDEAD0050,  // Filter 4~7 Command
	0xDEAD0060,  // Filter 4~7 Offset
	0xDEAD0070,  // Filter 4 5 CRC
	0xDEAD0080,  // Filter 6 7 CRC
};

static void eth_setPMT(UINT32 pmt_en)
{
	T_ETH_PMT_CTRL_STS_REG pmtctrlReg;
	T_ETH_PMT_FLITER_REG pmtfilterReg;
	UINT32 i;
	UINT32 flags = 0;

	DBG_DUMP("%s: %s\r\n", __func__, pmt_en ? "Enable" : "Disable");
	flags = eth_platform_spin_lock();
	if (pmt_en == 0) {
		pmtctrlReg.bit.PWRDWN     = 0;
		pmtctrlReg.bit.RWKFILTRST = 0;
		pmtctrlReg.bit.MGKPKTEN   = 0;
		pmtctrlReg.bit.RWKPKTEN   = 0;
	} else {
		pmtctrlReg.bit.PWRDWN     = 1;
		pmtctrlReg.bit.RWKFILTRST = 1;
		if (pmt_en == 1) {
			pmtctrlReg.bit.MGKPKTEN   = 1;
			pmtctrlReg.bit.RWKPKTEN   = 0;
		} else if (pmt_en == 2) {
			pmtctrlReg.bit.MGKPKTEN   = 0;
			pmtctrlReg.bit.RWKPKTEN   = 1;
		} else {
			pmtctrlReg.bit.MGKPKTEN   = 1;
			pmtctrlReg.bit.RWKPKTEN   = 1;
		}

	}
	ETH_SETREG(ETH_PMT_CTRL_STS_REG_OFS, pmtctrlReg.reg);

	if (pmtctrlReg.bit.RWKPKTEN == 1) {
		for (i = 0; i < 16; i++) {
			pmtctrlReg.reg = ETH_GETREG(ETH_PMT_CTRL_STS_REG_OFS);
			pmtfilterReg.reg = rwuFilter[i];
			ETH_SETREG(ETH_PMT_FLITER_REG_OFS, pmtfilterReg.reg);
			DBG_DUMP("Write [%2d]: 0x%8x\r\n", pmtctrlReg.bit.RWKPTR, pmtfilterReg.reg);
		}

		for (i = 0; i < 16; i++) {
			pmtctrlReg.reg = ETH_GETREG(ETH_PMT_CTRL_STS_REG_OFS);
			pmtfilterReg.reg = ETH_GETREG(ETH_PMT_FLITER_REG_OFS);
			DBG_DUMP("Read [%2d]: 0x%8x\r\n", pmtctrlReg.bit.RWKPTR, pmtfilterReg.reg);
		}
	}
	eth_platform_spin_unlock(flags);
}

void eth_clearWGisr(void)
{
	T_ETH_PMT_CTRL_STS_REG pmtCtrlStsReg = {0};

	pmtCtrlStsReg.reg = ETH_GETREG(ETH_PMT_CTRL_STS_REG_OFS);

	if (pmtCtrlStsReg.bit.PWRDWN) {
		DBG_DUMP("PWRDWN\r\n");
	}
	if (pmtCtrlStsReg.bit.MGKPRCVD) {
		DBG_DUMP("Magic packet received\r\n");
	}
	if (pmtCtrlStsReg.bit.RWKPRCVD) {
		DBG_DUMP("Remote wake-up packet received\r\n");
	}
}
