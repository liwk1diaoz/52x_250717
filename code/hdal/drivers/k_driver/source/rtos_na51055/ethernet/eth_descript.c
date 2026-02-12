/*
    Ethernet DMA descript

    This file controls the DMA descriptor in ethernet gmac

    @file       Eth_descript.c
    @ingroup    mIDrvIO_Eth
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#include <string.h>
//#include "SysKer.h"
//#include "ethernet_protected.h"
#include "ethernet_reg.h"
#include "ethernet_int.h"
#include "eth_descript.h"
#include "eth_platform.h"
#include "dma.h"
#include "dma_protected.h"
//#include "Delay.h"

//#define DATA_BUF_BYTE_ALIGN

//
// Internal prototypes
//
static ETH_DESC_QUEUE txQueue;
static ETH_DESC_QUEUE rxQueue;

//
// Internal Definitions
//


/*
    ethernet initialize descriptor queue

    @param[in] uiTxQueueBase        base address of tx queue
    @param[in] uiTxPackeBufBase     base address of tx packet/frame buffer
    @param[in] uiRxQueueBase        base address of rx queue
    @param[in] uiRxPackeBufBase     base address of rx packet/frame buffer

    @return void
*/
void ethDesc_initQueue(ETHMAC_DMA_DESCRIPT_MODE_ENUM txQueueMode,
					   UINT32 uiTxQueueBase,
					   UINT32 uiTxPacketBufBase,
					   ETHMAC_DMA_DESCRIPT_MODE_ENUM rxQueueMode,
					   UINT32 uiRxQueueBase,
					   UINT32 uiRxPacketBufBase)
{
	UINT32 i;
	ETH_RX_DESC *pRxDesc;
	ETH_TX_DESC *pTxDesc;
#ifdef DATA_BUF_BYTE_ALIGN
	UINT8 *pBuf;
#else
	UINT32 *pBuf;
#endif

	txQueue.queueType = txQueueMode;
	txQueue.uiQueueBaseAddr = (UINT32)uiTxQueueBase;
	txQueue.uiMaxDepth = ETH_TX_DESC_QUEUE_LENGTH;
	txQueue.uiFrameBaseAddr = uiTxPacketBufBase;
	txQueue.uiCurrDepth = 0;
	txQueue.uiWritePos = 0;
	txQueue.uiReadPos = 0;

	rxQueue.queueType = rxQueueMode;
	rxQueue.uiQueueBaseAddr = (UINT32)uiRxQueueBase;
	rxQueue.uiMaxDepth = ETH_RX_DESC_QUEUE_LENGTH;
	rxQueue.uiFrameBaseAddr = uiRxPacketBufBase;
	rxQueue.uiCurrDepth = 0;
	rxQueue.uiWritePos = 0;
	rxQueue.uiReadPos = 0;

	memset((void *)txQueue.uiQueueBaseAddr, 0, txQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32));
	memset((void *)rxQueue.uiQueueBaseAddr, 0, rxQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32));

	memset((void *)txQueue.uiFrameBaseAddr, 0, txQueue.uiMaxDepth * ETH_DESC_BUF_SIZE * 2);
	memset((void *)rxQueue.uiFrameBaseAddr, 0, rxQueue.uiMaxDepth * ETH_DESC_BUF_SIZE * 2);

	//
	// initialize tx descriptors
	//
#if 1
	if (txQueue.queueType == ETHMAC_DMA_DESCRIPT_MODE_RING) {
#ifdef DATA_BUF_BYTE_ALIGN
		for (i = 0, pTxDesc = (ETH_TX_DESC *)txQueue.uiQueueBaseAddr, pBuf = (UINT8 *)txQueue.uiFrameBaseAddr;
			 i < txQueue.uiMaxDepth;
			 i++, pTxDesc++, pBuf += (2 * ETH_DESC_BUF_SIZE / sizeof(pBuf[0]) + 1))
#else
		for (i = 0, pTxDesc = (ETH_TX_DESC *)txQueue.uiQueueBaseAddr, pBuf = (UINT32 *)txQueue.uiFrameBaseAddr;
			 i < txQueue.uiMaxDepth;
			 i++, pTxDesc++, pBuf += (2 * ETH_DESC_BUF_SIZE / sizeof(pBuf[0])))
#endif
		{
			// Assign frame/packet buffer to each descriptor
			pTxDesc->uiUsr1AP = (UINT32)pBuf;
			pTxDesc->uiUsr2AP = ((UINT32)pBuf) + ETH_DESC_BUF_SIZE;
			pTxDesc->uiBuf1AP = dma_getPhyAddr(pTxDesc->uiUsr1AP);
			pTxDesc->uiBuf2AP = dma_getPhyAddr(pTxDesc->uiUsr2AP);
			//DBG_ERR("Tx-%d Desc = 0x%08x, buf1 = 0x%08x, buf2 = 0x%08x\r\n",i ,(UINT32)pTxDesc, pTxDesc->uiUsr1AP, pTxDesc->uiUsr2AP);
		}

		// Set total Tx descriptor count
		ethGMAC_setDmaTxDescCount(txQueue.uiMaxDepth);

		// Set tx tail pointer
		ethGMAC_resumeDmaTx((UINT32)dma_getPhyAddr((UINT32)(pTxDesc - 1)));
	} else {
		DBG_ERR("Tx queue still not implement CHAIN mode\r\n");
	}
#else
#endif
	// flush frame(packet) buffer
	/*
	dma_flushReadCacheWidthNEQLineOffset(txQueue.uiFrameBaseAddr,
										 txQueue.uiMaxDepth * 2 * ETH_DESC_BUF_SIZE);
	// flush descriptor
	dma_flushReadCacheWidthNEQLineOffset((UINT32)txQueue.uiQueueBaseAddr,
										 txQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32));
	*/

	vos_cpu_dcache_sync((VOS_ADDR)txQueue.uiFrameBaseAddr, txQueue.uiMaxDepth * 2 * ETH_DESC_BUF_SIZE, VOS_DMA_TO_DEVICE_NON_ALIGN);
	vos_cpu_dcache_sync((VOS_ADDR)txQueue.uiQueueBaseAddr, txQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32), VOS_DMA_TO_DEVICE_NON_ALIGN);

	//
	// initialize rx descriptors
	//
#if 1
	if (rxQueue.queueType == ETHMAC_DMA_DESCRIPT_MODE_RING) {
#ifdef DATA_BUF_BYTE_ALIGN
		for (i = 0, pRxDesc = (ETH_RX_DESC *)rxQueue.uiQueueBaseAddr, pBuf = (UINT8 *)rxQueue.uiFrameBaseAddr;
			 i < rxQueue.uiMaxDepth;
			 i++, pRxDesc++, pBuf += ((2 * ETH_DESC_BUF_SIZE / sizeof(pBuf[0]) + 1)))
#else
		for (i = 0, pRxDesc = (ETH_RX_DESC *)rxQueue.uiQueueBaseAddr, pBuf = (UINT32 *)rxQueue.uiFrameBaseAddr;
			 i < rxQueue.uiMaxDepth;
			 i++, pRxDesc++, pBuf += (2 * ETH_DESC_BUF_SIZE / sizeof(pBuf[0])))
#endif
		{
			// Assign frame/packet buffer to each descriptor
			pRxDesc->bBUF1V = 1;
			pRxDesc->bBUF2V = 1;
			pRxDesc->bOWN = 1;
			pRxDesc->bIOC = 1;
			pRxDesc->uiUsr1AP = (UINT32)pBuf;
			pRxDesc->uiBuf1AP = dma_getPhyAddr(pRxDesc->uiUsr1AP);
			pRxDesc->uiUsr2AP = ((UINT32)pBuf) + ETH_DESC_BUF_SIZE;
			pRxDesc->uiBuf2AP = dma_getPhyAddr(pRxDesc->uiUsr2AP);
			//DBG_ERR("Rx-%d Desc = 0x%08x, buf1 = 0x%08x, buf2 = 0x%08x\r\n",i ,(UINT32)pRxDesc, pRxDesc->uiUsr1AP, pRxDesc->uiUsr2AP);
		}

		// Set total Rx descriptor count
		ethGMAC_setDmaRxDescCount(rxQueue.uiMaxDepth);

		// Set rx tail pointer
		ethGMAC_updateDmaRxTailPtr((UINT32)(dma_getPhyAddr((UINT32)(pRxDesc - 1))));
	} else {
		DBG_ERR("eQoS not support descriptor chain mode\r\n");
		abort();
	}
#else
#endif
	// flush frame(packet) buffer
	/*
	dma_flushReadCacheWidthNEQLineOffset(rxQueue.uiFrameBaseAddr,
										 rxQueue.uiMaxDepth * 2 * ETH_DESC_BUF_SIZE);
	// flush descriptor
	dma_flushReadCacheWidthNEQLineOffset((UINT32)rxQueue.uiQueueBaseAddr,
										 rxQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32));
	*/
	vos_cpu_dcache_sync((VOS_ADDR)rxQueue.uiFrameBaseAddr, rxQueue.uiMaxDepth * 2 * ETH_DESC_BUF_SIZE, VOS_DMA_TO_DEVICE_NON_ALIGN);
	vos_cpu_dcache_sync((VOS_ADDR)rxQueue.uiQueueBaseAddr, rxQueue.uiMaxDepth * ETH_DESC_WORD_SIZE * sizeof(UINT32), VOS_DMA_TO_DEVICE_NON_ALIGN);
}

/*
    check tx queue full

    @return
        - @b TRUE: tx queue full
        - @b FALSE: tx queue not full
*/
BOOL ethDesc_isTxFull(void)
{
	if (txQueue.uiCurrDepth >= txQueue.uiMaxDepth) {
		return TRUE;
	}

	return FALSE;
}

/*
    check tx queue empty

    @return
        - @b TRUE: tx queue empty
        - @b FALSE: tx queue not empty
*/
BOOL ethDesc_isTxEmpty(void)
{
	if (txQueue.uiCurrDepth == 0) {
		return TRUE;
	}

	return FALSE;
}

/*
    Insert frame to tx queue

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethDesc_insertTxFrame(ETH_FRAME_HEAD *pFrameHead, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData)
{
	UINT32 uiQueuePos;
	UINT32 uiTxBuf1Addr;
	UINT32 uiTxBuf2Addr;
	UINT32 uiDepth = 0;
#if defined(_NVT_EMULATION_)
	const UINT32 uiTcpPayloadLen = uiLen - 20 - 20;
	const UINT32 uiUdpPayloadLen = uiLen - 20;
#else
	const UINT32 uiTcpPayloadLen = uiLen - 14 - 20 - 20;
	const UINT32 uiUdpPayloadLen = uiLen - 14 - 20;
#endif
	BOOL bFirstSeg = TRUE;
	ETH_TX_DESC *pTxDesc;
	volatile ETH_TX_DESC __attribute__((unused)) dummyDesc;

	if (ethDesc_isTxFull() == TRUE) {
		return E_NOMEM;
	}

	uiQueuePos = txQueue.uiWritePos;
	pTxDesc = (ETH_TX_DESC *)txQueue.uiQueueBaseAddr;

	while (uiLen) {
		UINT32 uiChunkLen;

		uiTxBuf1Addr = pTxDesc[uiQueuePos].uiUsr1AP;            // backup setting
		uiTxBuf2Addr = pTxDesc[uiQueuePos].uiUsr2AP;
		memset(&pTxDesc[uiQueuePos], 0, sizeof(pTxDesc[0]));    // reset this descriptor
		pTxDesc[uiQueuePos].uiUsr1AP = uiTxBuf1Addr;            // restore setting
		pTxDesc[uiQueuePos].uiUsr2AP = uiTxBuf2Addr;
		pTxDesc[uiQueuePos].uiBuf1AP = dma_getPhyAddr(uiTxBuf1Addr);
		pTxDesc[uiQueuePos].uiBuf2AP = dma_getPhyAddr(uiTxBuf2Addr);

		//DBG_WRN("desc 0x%lx\r\n", &pTxDesc[uiQueuePos]);
		//DBG_WRN("uiQueuePos %d, buf1=0x%x, buf2=0x%x\r\n", uiQueuePos, uiTxBuf1Addr, uiTxBuf2Addr);
		DBG_WRN("uiLen %d\r\n", uiLen);
		//
		// 1. Handle buffer1
		//

		// copy ethernet frame header
		if (bFirstSeg) {
			pTxDesc[uiQueuePos].bFD = 1;
//            bFirstSeg = FALSE;
#if defined(_NVT_EMULATION_)
			memcpy((void *)uiTxBuf1Addr, pFrameHead, sizeof(ETH_FRAME_HEAD));
			uiTxBuf1Addr += sizeof(ETH_FRAME_HEAD);
			pTxDesc[uiQueuePos].uiHLB1L = sizeof(ETH_FRAME_HEAD);
#endif

		}

		// copy ethernet frame payload
		if (uiLen) {
			if ((type == ETH_PAYLOAD_TCP) && bFirstSeg) {
#if defined(_NVT_EMULATION_)
				uiChunkLen = 20 + 20;   // IP hdr len + TCP hdr len
#else
				uiChunkLen = 14 + 20 + 20;   //ETH header + IP hdr len + TCP hdr len
#endif
			} else if ((type == ETH_PAYLOAD_UDP) && bFirstSeg) {
//                uiChunkLen = 20;    // IP hdr len + UDP hdr len
#if defined(_NVT_EMULATION_)
				uiChunkLen = 20 + 8;    // IP hdr len + UDP hdr len
#else
				uiChunkLen = 14 + 20 + 8;    ///ETH header + IP hdr len + UDP hdr len
#endif
			} else {
				if (bFirstSeg) {
#if defined(_NVT_EMULATION_)
					uiChunkLen = ((uiLen + sizeof(ETH_FRAME_HEAD)) < ETH_DESC_BUF_SIZE) ? uiLen : (ETH_DESC_BUF_SIZE - sizeof(ETH_FRAME_HEAD));
#else
					uiChunkLen = (uiLen < ETH_DESC_BUF_SIZE) ? uiLen : (ETH_DESC_BUF_SIZE - sizeof(ETH_FRAME_HEAD));
#endif
				} else {
					uiChunkLen = (uiLen < ETH_DESC_BUF_SIZE) ? uiLen : (ETH_DESC_BUF_SIZE);
					//uiChunkLen = ((uiLen+sizeof(ETH_FRAME_HEAD)) < ETH_DESC_BUF_SIZE) ? uiLen : (ETH_DESC_BUF_SIZE);
				}
			}
//            bFirstSeg = FALSE;
DBG_WRN("uiChunkLen %d\r\n", uiChunkLen);
			memcpy((void *)uiTxBuf1Addr, pData, uiChunkLen);
			pData += uiChunkLen;
			uiLen -= uiChunkLen;
			pTxDesc[uiQueuePos].uiHLB1L += uiChunkLen;
		}
		//DBG_WRN("BL1: 0x%lx\r\n", pTxDesc[uiQueuePos].uiHLB1L);
		// flush payload cache
		/*
		dma_flushReadCacheWidthNEQLineOffset(pTxDesc[uiQueuePos].uiUsr1AP,
											 pTxDesc[uiQueuePos].uiHLB1L);*/
		vos_cpu_dcache_sync((VOS_ADDR)pTxDesc[uiQueuePos].uiUsr1AP, pTxDesc[uiQueuePos].uiHLB1L, VOS_DMA_TO_DEVICE_NON_ALIGN);

		//
		// 2. Handle buffer2 (if required)
		//
		if (uiLen) {
			uiChunkLen = (uiLen < ETH_DESC_BUF_SIZE) ? uiLen : ETH_DESC_BUF_SIZE;
			memcpy((void *)uiTxBuf2Addr, pData, uiChunkLen);
			pData += uiChunkLen;
			uiLen -= uiChunkLen;
			pTxDesc[uiQueuePos].uiB2L += uiChunkLen;
		}
		DBG_WRN("BL2: 0x%lx\r\n", pTxDesc[uiQueuePos].uiB2L);
		DBG_WRN("uiHLB1L: 0x%lx\r\n", pTxDesc[uiQueuePos].uiHLB1L);
		// update total length of packet
		pTxDesc[uiQueuePos].uiFL_TPL = pTxDesc[uiQueuePos].uiHLB1L + pTxDesc[uiQueuePos].uiB2L;
		// flush payload cache
		/*
		dma_flushReadCacheWidthNEQLineOffset(pTxDesc[uiQueuePos].uiUsr2AP,
											 pTxDesc[uiQueuePos].uiB2L);
		*/
		vos_cpu_dcache_sync((VOS_ADDR)pTxDesc[uiQueuePos].uiUsr2AP, pTxDesc[uiQueuePos].uiB2L, VOS_DMA_TO_DEVICE_NON_ALIGN);

DBG_WRN("uiFL_TPL %d\r\n", pTxDesc[uiQueuePos].uiFL_TPL);
		if (type == ETH_PAYLOAD_RAW) {
			pTxDesc[uiQueuePos].uiCIC_TPL = 0;
		} else if (type == ETH_PAYLOAD_TCP) {
//            UINT32 uiTcpPayloadLen = uiLen - 20 - 20;

			if (bFirstSeg) {
				pTxDesc[uiQueuePos].bTSE = 1;
				pTxDesc[uiQueuePos].uiTHL = 20 / sizeof(UINT32);

				pTxDesc[uiQueuePos].uiFL_TPL = uiTcpPayloadLen & 0x7FFF;
				pTxDesc[uiQueuePos].uiTPL = (uiTcpPayloadLen >> 15) & 0x01;
				pTxDesc[uiQueuePos].uiCIC_TPL = (uiTcpPayloadLen >> 16) & 0x03;
			}
			DBG_WRN("tcp len %d\r\n", uiTcpPayloadLen);
		} else if (type == ETH_PAYLOAD_UDP) {
			if (bFirstSeg) {
				pTxDesc[uiQueuePos].bTSE = 1;
				pTxDesc[uiQueuePos].uiTHL = 2;  // synopsys design is 2 = UDP segmentation

				pTxDesc[uiQueuePos].uiFL_TPL = uiUdpPayloadLen & 0x7FFF;
				pTxDesc[uiQueuePos].uiTPL = (uiUdpPayloadLen >> 15) & 0x01;
				pTxDesc[uiQueuePos].uiCIC_TPL = (uiUdpPayloadLen >> 16) & 0x03;
			}
			DBG_WRN("udp len %d\r\n", uiUdpPayloadLen);
			DBG_WRN("uiLen %d\r\n", uiLen);
		} else {
			// if payload is IP, use checksum insertion mode 2
			pTxDesc[uiQueuePos].uiCIC_TPL = 3;
		}

		pTxDesc[uiQueuePos].bIOC = 1;           // generate interrupt when tx complete
		if (uiLen == 0) { // tricky: assume ring structure is applied
			pTxDesc[uiQueuePos].bLD = 1;
		}
		pTxDesc[uiQueuePos].bOWN = 1;           // own by DMA
		dummyDesc = pTxDesc[uiQueuePos];        // read back to ensure write success
		// flush descriptor cache
		{
			UINT32 uiAddr = (UINT32)(&pTxDesc[uiQueuePos]);

			//dma_flushReadCacheWidthNEQLineOffset(uiAddr, sizeof(pTxDesc[0]));
			vos_cpu_dcache_sync((VOS_ADDR)uiAddr, sizeof(pTxDesc[0]), VOS_DMA_TO_DEVICE_NON_ALIGN);
		}

#if 0
		{
			UINT32 *ptr = &pTxDesc[uiQueuePos];
			DBG_WRN("TDES0 0x%lx\r\n", ptr[0]);
			DBG_WRN("TDES1 0x%lx\r\n", ptr[1]);
			DBG_WRN("TDES2 0x%lx\r\n", ptr[2]);
			DBG_WRN("TDES3 0x%lx\r\n", ptr[3]);
		}
#endif
		bFirstSeg = FALSE;
		uiQueuePos = (uiQueuePos + 1) % txQueue.uiMaxDepth;
		uiDepth++;
	}

    __asm__ __volatile__("dsb\n\t");

	txQueue.uiWritePos = uiQueuePos;
	txQueue.uiCurrDepth += uiDepth;
	if (txQueue.uiCurrDepth > txQueue.uiMaxDepth) {
		DBG_ERR("Tx descriptor queue overflow, depth %d, max %d\r\n",
				txQueue.uiCurrDepth,
				txQueue.uiMaxDepth);
		return E_SYS;
	}

	return E_OK;
}

/*
    Get latest pendint tx desc addr

    @return
        - @b E_OK: success
        - @b Else: fail
*/
UINT32 ethDesc_getLastTxDescAddr(void)
{
	ETH_TX_DESC *pTxDesc;
	UINT32 idx;

	idx = (txQueue.uiWritePos - 1) % txQueue.uiMaxDepth;

	pTxDesc = (ETH_TX_DESC *)txQueue.uiQueueBaseAddr;

	return (UINT32)(&pTxDesc[idx]);
}

/*
    Get latest pendint rx desc addr

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static UINT32 ethDesc_getLastRxDescAddr(void)
{
	ETH_RX_DESC *pRxDesc;
	UINT32 idx;

	idx = (rxQueue.uiReadPos - 1) % rxQueue.uiMaxDepth;

	pRxDesc = (ETH_RX_DESC *)rxQueue.uiQueueBaseAddr;

	return (UINT32)(&pRxDesc[idx]);
}

/*
    Remove frame to tx queue

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethDesc_removeTxFrame(void)
{
	UINT32 uiQueuePos;
	ETH_TX_DESC_READ *pTxDesc;

	if (ethDesc_isTxEmpty() == TRUE) {
		return E_NOMEM;
	}

	pTxDesc = (ETH_TX_DESC_READ *)txQueue.uiQueueBaseAddr;

	while (txQueue.uiCurrDepth) {
		UINT32 uiAddr;
		uiQueuePos = txQueue.uiReadPos;

		// Invalidate descriptor cache
		uiAddr = (UINT32)(&pTxDesc[uiQueuePos]);
		uiAddr = dma_getCacheAddr(uiAddr);
		//DBG_WRN("dma_flushReadCache 0x%x\r\n", uiAddr);
		//dma_flushReadCache(uiAddr, sizeof(pTxDesc[0]));
		vos_cpu_dcache_sync((VOS_ADDR)uiAddr, sizeof(pTxDesc[0]), VOS_DMA_FROM_DEVICE_NON_ALIGN);

		if (pTxDesc[uiQueuePos].bOWN) {
			UINT32 i = 0;

			//coverity[loop_top]
			while (1) {
				//coverity[exit_condition]
				if (pTxDesc[uiQueuePos].bOWN == 0) {
					break;
				}
				i++;
				if ((i % 300) == 0) {
					DBG_DUMP(".");
				}
				eth_platform_delay_ms(10);
			}
#if 0
			UINT32 *pData;

			DBG_ERR("Descriptor removed when owned by DMA\r\n");
			DBG_ERR("Queue base 0x%lx\r\n", txQueue.uiQueueBaseAddr);
			DBG_ERR("Queue pos 0x%lx, max 0x%lx\r\n", uiQueuePos, txQueue.uiMaxDepth);
			DBG_ERR("write pos 0x%lx, read pos 0x%lx\r\n", txQueue.uiWritePos, txQueue.uiReadPos);
			pData = (UINT32 *)(&pTxDesc[uiQueuePos]);
			DBG_ERR("Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", pData[0], pData[1], pData[2], pData[3]);
			return E_SYS;
#endif
		}

		txQueue.uiReadPos = (txQueue.uiReadPos + 1) % txQueue.uiMaxDepth;
		txQueue.uiCurrDepth--;
	}

	return E_OK;
}

/*
    Retrieve RX frame

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER ethDesc_retrieveRxFrame(UINT32 uiBufSize, UINT32 *puiLen, UINT8 *pData)
{
	UINT32 uiQueuePos;
	UINT32 uiRxBuf1Addr;
	UINT32 uiRxBuf2Addr;
	UINT32 uiFrameLen = 0;
	UINT32 uiRecvLen = 0;
	BOOL bFirstSeg = TRUE;
	BOOL bLastSeg = FALSE;
	ETH_RX_DESC_READ *pRxDesc;
	volatile ETH_RX_DESC_READ __attribute__((unused)) dummyDesc;

	if (ethDesc_isTxFull() == TRUE) {
		return E_NOMEM;
	}

	uiQueuePos = rxQueue.uiReadPos;
	pRxDesc = (ETH_RX_DESC_READ *)rxQueue.uiQueueBaseAddr;

	while (bLastSeg == FALSE) {
		UINT32 uiChunkLen;
		UINT32 uiAddr;

		// Invalidate descriptor cache
		uiAddr = (UINT32)(&pRxDesc[uiQueuePos]);
		uiAddr = dma_getCacheAddr(uiAddr);
		//DBG_ERR("dma_flushReadCache 0x%x\r\n", uiAddr);
		//dma_flushReadCache(uiAddr, sizeof(pRxDesc[0]));
		vos_cpu_dcache_sync((VOS_ADDR)uiAddr, sizeof(pRxDesc[0]), VOS_DMA_FROM_DEVICE_NON_ALIGN);

		if (pRxDesc[uiQueuePos].bOWN) {
#if 0
			UINT32 *pData;

			DBG_DUMP("Rx Descriptor retrieved when owned by DMA\r\n");
			DBG_DUMP("Queue base 0x%lx\r\n", rxQueue.uiQueueBaseAddr);
			DBG_DUMP("Queue pos 0x%lx, max\r\n", uiQueuePos, rxQueue.uiMaxDepth);
			DBG_DUMP("write pos 0x%lx, read pos 0x%lx\r\n", rxQueue.uiWritePos, rxQueue.uiReadPos);
			pData = (UINT32 *)(&pRxDesc[uiQueuePos]);
			DBG_DUMP("Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", pData[0], pData[1], pData[2], pData[3]);
#endif
			return E_SYS;
		}

		if (bFirstSeg) {
			bFirstSeg = FALSE;
			if (pRxDesc[uiQueuePos].bFD == 0) {
				UINT32 *pData;

				DBG_ERR("First read, but FS flag not set\r\n");
				DBG_ERR("Queue base 0x%lx\r\n", rxQueue.uiQueueBaseAddr);
				DBG_ERR("Queue pos 0x%lx, max\r\n", uiQueuePos, rxQueue.uiMaxDepth);
				DBG_ERR("write pos 0x%lx, read pos 0x%lx\r\n", rxQueue.uiWritePos, rxQueue.uiReadPos);
				pData = (UINT32 *)(&pRxDesc[uiQueuePos]);
				DBG_ERR("Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", pData[0], pData[1], pData[2], pData[3]);
				return E_SYS;
			}
		}

//        DBG_WRN("PL = 0x%lx\r\n", pRxDesc[uiQueuePos].uiPL);
		if (pRxDesc[uiQueuePos].uiPL == 0) {
			UINT32 *pData;

			DBG_ERR("First read, but PL flag not set\r\n");
			DBG_ERR("Queue base 0x%lx\r\n", rxQueue.uiQueueBaseAddr);
			DBG_ERR("Queue pos 0x%lx, max\r\n", uiQueuePos, rxQueue.uiMaxDepth);
			DBG_ERR("write pos 0x%lx, read pos 0x%lx\r\n", rxQueue.uiWritePos, rxQueue.uiReadPos);
			pData = (UINT32 *)(&pRxDesc[uiQueuePos]);
			DBG_ERR("Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", pData[0], pData[1], pData[2], pData[3]);

			// Impossible path
			uiFrameLen = 0;
//            uiFrameLen = pRxDesc[uiQueuePos].uiBuf1Size + pRxDesc[uiQueuePos].uiBuf2Size;
		} else {
			uiFrameLen = pRxDesc[uiQueuePos].uiPL - uiRecvLen;
		}

		//
		// 1. Handle buffer1
		//
		uiChunkLen = (uiFrameLen < ETH_DESC_BUF_SIZE) ? uiFrameLen : ETH_DESC_BUF_SIZE;
		//dma_flushReadCache(pRxDesc[uiQueuePos].uiUsr1AP, uiChunkLen);
		vos_cpu_dcache_sync((VOS_ADDR)pRxDesc[uiQueuePos].uiUsr1AP, uiChunkLen, VOS_DMA_FROM_DEVICE_NON_ALIGN);
		memcpy(pData, (void *)pRxDesc[uiQueuePos].uiUsr1AP, uiChunkLen);
		uiRecvLen += uiChunkLen;
		pData += uiChunkLen;
		uiFrameLen -= uiChunkLen;

		//
		// 2. Handle buffer2 (if required)
		//
		if (uiFrameLen) {
			uiChunkLen = (uiFrameLen < ETH_DESC_BUF_SIZE) ? uiFrameLen : ETH_DESC_BUF_SIZE;
			//dma_flushReadCache(pRxDesc[uiQueuePos].uiUsr2AP, uiChunkLen);
			vos_cpu_dcache_sync((VOS_ADDR)pRxDesc[uiQueuePos].uiUsr2AP, uiChunkLen, VOS_DMA_FROM_DEVICE_NON_ALIGN);
			memcpy(pData, (void *)pRxDesc[uiQueuePos].uiUsr2AP, uiChunkLen);
			uiRecvLen += uiChunkLen;
			pData += uiChunkLen;
			uiFrameLen -= uiChunkLen;
		}

		bLastSeg = pRxDesc[uiQueuePos].bLD;


		uiRxBuf1Addr = pRxDesc[uiQueuePos].uiUsr1AP;            // backup setting
		uiRxBuf2Addr = pRxDesc[uiQueuePos].uiUsr2AP;
		{
			ETH_RX_DESC *pRxDesc_W = (ETH_RX_DESC *)&pRxDesc[uiQueuePos];

			memset(pRxDesc_W, 0, sizeof(pRxDesc[0]));   // reset this descriptor
			pRxDesc_W->uiUsr1AP = uiRxBuf1Addr;            // restore setting
			pRxDesc_W->uiBuf1AP = dma_getPhyAddr(pRxDesc_W->uiUsr1AP);
			pRxDesc_W->uiUsr2AP = uiRxBuf2Addr;
			pRxDesc_W->uiBuf2AP = dma_getPhyAddr(pRxDesc_W->uiUsr2AP);
			pRxDesc_W->bBUF1V = 1;
			pRxDesc_W->bBUF2V = 1;
			pRxDesc_W->bIOC = 1;
			pRxDesc_W->bOWN = 1;           // re-owned to DMA
		}

		// Flush descriptor cache
		dummyDesc = pRxDesc[uiQueuePos];
		uiAddr = (UINT32)(&pRxDesc[uiQueuePos]);
		//dma_flushReadCacheWidthNEQLineOffset(uiAddr, sizeof(pRxDesc[0]));
		vos_cpu_dcache_sync((VOS_ADDR)uiAddr, sizeof(pRxDesc[0]), VOS_DMA_TO_DEVICE_NON_ALIGN);

		if (0) {
			UINT32 *pData;

			pData = (UINT32 *)(&pRxDesc[uiQueuePos]);
			DBG_ERR("%d 0x%lx: Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", uiQueuePos, pData, pData[0], pData[1], pData[2], pData[3]);
			pData = (UINT32 *)(&pRxDesc[15]);
			DBG_ERR("%d 0x%lx: Context: 0x%lx, 0x%lx, 0x%lx, 0x%lx\r\n", 15, pData, pData[0], pData[1], pData[2], pData[3]);
		}

		// Index to next descriptor
		uiQueuePos = (uiQueuePos + 1) % rxQueue.uiMaxDepth;

//        DBG_ERR("Next pos: %d\r\n", uiQueuePos);
	}

	rxQueue.uiReadPos = uiQueuePos;

	ethGMAC_updateDmaRxTailPtr(dma_getPhyAddr(ethDesc_getLastRxDescAddr()));

	*puiLen = uiRecvLen;
#if !defined(_NVT_EMULATION_)
	vos_cpu_dcache_sync((VOS_ADDR)dma_getCacheAddr(&pRxDesc[uiQueuePos]), sizeof(pRxDesc[0]), VOS_DMA_FROM_DEVICE_NON_ALIGN);
	if (pRxDesc[uiQueuePos].bOWN == 0) {
		if (ETH_GETREG(ETH_DMA_CH0_STS_REG_OFS) & 0x40) {
			ETH_SETREG(ETH_DMA_CH0_STS_REG_OFS, 0x40);
		}
		return 1;
	}
#endif
	return E_OK;
}

