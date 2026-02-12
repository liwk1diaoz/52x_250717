/*
    H264DEC module driver for NT96660

    Interface for Init and Info

    @file       h264dec_api.c
    @ingroup    mICodecH264

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h26x_def.h"
#include "h26x_common.h"

#include "h264dec_api.h"
#include "h264dec_int.h"
#include "h264dec_cfg.h"
#include "h264dec_header.h"

static INT32 h264Dec_initContextMemory(PH264DEC_INIT pH264DecInit, H26XDEC_VAR *pH264DecVar)
{
	UINT32 uiSizeContext    = SIZE_32X(sizeof(H264DEC_CTX));

	if (pH264DecInit->uiDecBufSize < uiSizeContext) {
		DBG_ERR("[h264Dec_initContextMemory]H264 Decoder Internal context Buffer Size not enough!\r\n");
		return H264DEC_FAIL;
	}

	SetMemoryAddr((UINT32 *)&pH264DecVar->pContext, &pH264DecInit->uiDecBufAddr, uiSizeContext);
	memset(pH264DecVar->pContext, 0, uiSizeContext);

	return H264DEC_SUCCESS;
}

//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL   : size is too small or address is null
*/
static INT32 h264Dec_initInternalMemory(PH264DEC_INIT pH264DecInit, H26XDEC_VAR *pH264DecVar)
{
    UINT32 uiVirMemAddr,uiBufSize;
    H264DEC_CTX   *pContext;
    H26XDecAddr   *pH264VirDecAddr;

	UINT32 uiSizeSideInfo = ((((pH264DecInit->uiWidth + 63)/64)*4 + 31)/32)*32*((pH264DecInit->uiHeight + 15)/16);
	UINT32 uiSizeColMVs   = ((pH264DecInit->uiWidth + 63)/64) * ((pH264DecInit->uiHeight + 15)/16) * 64;
	UINT32 uiSizeBSDMA    = (1 + H26X_MAX_BSDMA_NUM*2)*4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();

	pContext = (H264DEC_CTX *)pH264DecVar->pContext;;
    pH264VirDecAddr = &pContext->sH264VirDecAddr;

    uiVirMemAddr = pH264DecInit->uiDecBufAddr;
    uiBufSize    = SIZE_32X(sizeof(H264DEC_CTX)) + uiSizeSideInfo*2 + uiSizeColMVs + uiSizeBSDMA + uiSizeAPB + 128;

    if (pH264DecInit->uiDecBufSize < uiBufSize){
        DBG_ERR("[h264Dec_initInternalMemory]H264 Decoder Internal Buffer Size not enough!\r\n");
        return H264DEC_FAIL;
    }

#if 0
	SetMemoryAddr(&pH264DecVar->uiAPBAddr, &uiVirMemAddr, uiSizeAPB);
	memset((void *)pH264DecVar->uiAPBAddr, 0, uiSizeAPB);
#else
	pH264DecVar->uiAPBAddr = pH264DecInit->uiAPBAddr;
	memset((void *)pH264DecVar->uiAPBAddr, 0, h26x_getHwRegSize());
	h26x_flushCache(pH264DecVar->uiAPBAddr, h26x_getHwRegSize());
#endif

    SetMemoryAddr( &pH264VirDecAddr->uiIlfSideInfoAddr[0], &uiVirMemAddr, uiSizeSideInfo);
	SetMemoryAddr( &pH264VirDecAddr->uiIlfSideInfoAddr[1], &uiVirMemAddr, uiSizeSideInfo);
    SetMemoryAddr( &pH264VirDecAddr->uiColMvsAddr[0], &uiVirMemAddr, uiSizeColMVs);	// h264 only use 1 buffer //
    SetMemoryAddr( &pH264VirDecAddr->uiCMDBufAddr, &uiVirMemAddr, uiSizeBSDMA);
    h26x_flushCache(pH264VirDecAddr->uiIlfSideInfoAddr[0], uiSizeSideInfo);// paulpaul add
    h26x_flushCache(pH264VirDecAddr->uiIlfSideInfoAddr[1], uiSizeSideInfo);// paulpaul add
    h26x_flushCache(pH264VirDecAddr->uiColMvsAddr[0], uiSizeColMVs);// paulpaul add
    h26x_flushCache(pH264VirDecAddr->uiCMDBufAddr, uiSizeBSDMA);// paulpaul add

	pH264DecVar->uiCtxSize = uiVirMemAddr - pH264DecInit->uiDecBufAddr;

	if (pH264DecVar->uiCtxSize > pH264DecInit->uiDecBufSize) {
		DBG_ERR("end check decBufSize too small: 0x%08x < 0x%08x\r\n", (int)pH264DecInit->uiDecBufSize, (int)pH264DecVar->uiCtxSize);

        return H264DEC_FAIL;
	}

    return H264DEC_SUCCESS;
}


//! API : initialize decoder
/*!
    Initialize config, buffer and init HW

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL: init fail
*/
INT32 h264Dec_initDecoder(PH264DEC_INIT pH264DecInit, H26XDEC_VAR *pH264DecVar){
    H264DEC_CTX   *pContext;

	H264DecSeqCfg *pH264DecSeqCfg;
    H264DecPicCfg *pH264DecPicCfg;
    H264DecHdrObj *pH264DecHdrObj;

    H26XDecAddr   *pH264VirDecAddr;

    if (h264Dec_initContextMemory(pH264DecInit, pH264DecVar) != H264DEC_SUCCESS) {
		return H264DEC_FAIL;
	}

	pContext = pH264DecVar->pContext;
	pH264DecSeqCfg = &pContext->sH264DecSeqCfg;
	pH264DecPicCfg = &pContext->sH264DecPicCfg;
	pH264DecHdrObj = &pContext->sH264DecHdrObj;
	pH264VirDecAddr = &pContext->sH264VirDecAddr;

	if (H264DecSeqHeader(pH264DecInit, pH264DecHdrObj, &pH264DecVar->stVUI) != H264DEC_SUCCESS) {
		return H264DEC_FAIL;
	}

	if (h264Dec_initInternalMemory(pH264DecInit, pH264DecVar) != H264DEC_SUCCESS) {
		return H264DEC_FAIL;
	}

    h264Dec_initCfg(pH264DecInit,pH264DecSeqCfg,pH264DecPicCfg,pH264DecHdrObj);

    H264Dec_initRegSet((H26XRegSet *)pH264DecVar->uiAPBAddr, pH264VirDecAddr);	// Init H264 RegSet //

    return H264DEC_SUCCESS;
}

//! API :  H264 Pre-decode
/*!
    prepare deoder config

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL: init fail
*/
INT32 h264Dec_prepareOnePicture(PH264DEC_INFO pH264DecInfo, H26XDEC_VAR *pH264DecVar)
{
    H264DEC_CTX   *pContext        = pH264DecVar->pContext;
    H264DecHdrObj *pH264DecHdrObj  = &pContext->sH264DecHdrObj;
	H26XRegSet    *pH264DecRegSet  = (H26XRegSet *)pH264DecVar->uiAPBAddr;
	H26XDecAddr   *pH264VirDecAddr = &pContext->sH264VirDecAddr;

	UINT32 uiHwHeaderNum, uiHwHeaderAddr, uiHwHeaderSize[320];
	UINT32 uiBsSize = (pH264DecInfo->uiCurBsSize == 0) ? pH264DecInfo->uiBSSize : pH264DecInfo->uiCurBsSize;
	UINT32 uiIdx = 0;

	//h26x_flushCache(pH264DecInfo->uiBSAddr, pH264DecInfo->uiBSSize);

	if (H264DecSlice(pH264DecInfo,pH264DecHdrObj,pH264DecInfo->uiBSAddr,pH264DecInfo->uiBSSize) != H264DEC_SUCCESS)
	{
		DBG_ERR("[H264Dec] decode slice header error\r\n");
		return H264DEC_FAIL;
	}

	pH264VirDecAddr->uiHwBsAddr = pH264DecInfo->uiBSAddr;

	pH264DecRegSet->NAL_HDR_LEN_TOTAL_LEN = pH264DecInfo->uiBSSize;
	uiHwHeaderNum = (pH264DecInfo->uiCurBsSize == 0) ? (pH264DecInfo->uiBSSize + 0x7ffff)/0x80000 : (pH264DecInfo->uiCurBsSize + 0x7ffff)/0x80000;
	uiHwHeaderAddr = pH264DecInfo->uiBSAddr ;

    //DBG_DUMP("NAL_HDR_LEN_TOTAL_LEN = %08x, %08x\r\n", (unsigned int)pH264DecRegSet->NAL_HDR_LEN_TOTAL_LEN,(unsigned int)pH264DecInfo->uiBSSize);
	for (uiIdx = 0; uiIdx < (uiHwHeaderNum - 1); uiIdx++)
	{
		uiHwHeaderSize[uiIdx] = 0x80000;
		uiBsSize -= 0x80000;
	}
	uiHwHeaderSize[uiIdx] = uiBsSize;

	// Set BSDMA Buffer //
	h26x_setBSDMA(pH264VirDecAddr->uiCMDBufAddr, uiHwHeaderNum, h26x_getPhyAddr(uiHwHeaderAddr), uiHwHeaderSize);

	//if(pH264DecSliceObj[0]->uiNalRefIdc != 0)
	{

		H264Dec_setFrmIdx(pH264DecHdrObj, pH264VirDecAddr);

		pH264DecRegSet->REC_Y_ADDR = h26x_getPhyAddr(pH264DecInfo->uiSrcYAddr);
		pH264DecRegSet->REC_C_ADDR = h26x_getPhyAddr(pH264DecInfo->uiSrcUVAddr);
		pH264DecRegSet->REF_Y_ADDR = pH264VirDecAddr->uiRefAndRecYAddr[pH264VirDecAddr->uiRef0Idx];
		pH264DecRegSet->REF_C_ADDR = pH264VirDecAddr->uiRefAndRecUVAddr[pH264VirDecAddr->uiRef0Idx];

		pH264VirDecAddr->uiRefAndRecYAddr[pH264VirDecAddr->uiRecIdx]  = pH264DecRegSet->REC_Y_ADDR;
		pH264VirDecAddr->uiRefAndRecUVAddr[pH264VirDecAddr->uiRecIdx] = pH264DecRegSet->REC_C_ADDR;
		//DBG_DUMP("rec(%d) %08x, %08x\r\n", pH264VirDecAddr->uiRecIdx, pContext->sH264DecRegSet.eRecYuvAddr.uiYAddr, pContext->sH264DecRegSet.eRecYuvAddr.uiUVAddr);
		//DBG_DUMP("ref(%d) %08x, %08x\r\n", pH264VirDecAddr->uiRef0Idx, pContext->sH264DecRegSet.eRefYuvAddr.uiYAddr, pContext->sH264DecRegSet.eRefYuvAddr.uiUVAddr);

		H264Dec_modifyFrmIdx(pH264DecHdrObj, pH264VirDecAddr);

	}

	H26X_SWAP(pH264DecRegSet->SIDE_INFO_WR_ADDR_0, pH264DecRegSet->SIDE_INFO_RD_ADDR_0, UINT32);

	//DBG_DUMP("[%s][%d]%08x, %08x\r\n", __FUNCTION__, __LINE__, pContext->sH264DecRegSet.eRecYuvAddr.uiYAddr, pH264DecInfo->uiSrcUVAddr[0]);
	H264Dec_prepareRegSet(pH264DecRegSet, pH264VirDecAddr, pContext);

    return H264DEC_SUCCESS;
}

//! API :  H264 Decode one picture
/*!
    prepare deoder config

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL: init fail
*/
INT32 h264Dec_decodeOnePicture(PH264DEC_INFO pH264DecInfo, H26XDEC_VAR *pH264DecVar){

	if (h264Dec_prepareOnePicture(pH264DecInfo, pH264DecVar) != H264DEC_SUCCESS)
	{
        return H264DEC_FAIL;
    }

    return H264DEC_SUCCESS;
}


#if 0
INT32 h264Dec_setNextBsBuf(H26XDEC_VAR *pH264DecVar, UINT32 uiBsAddr, UINT32 uiBsSize, UINT32 uiTotalBsLen)
{
	H264DEC_CTX   *pContext        = pH264DecVar->pContext;
	H26XDecAddr   *pH264VirDecAddr = &pContext->sH264VirDecAddr;

	UINT32 uiIdx;

	UINT32 uiHwHeaderSize[32];
	UINT32 uiHwHeaderNum = (uiBsSize + 0x7ffff)/0x80000;

	h26x_flushCache(uiBsAddr, uiBsSize);

	for (uiIdx = 0; uiIdx < (uiHwHeaderNum - 1); uiIdx++)
	{
		uiHwHeaderSize[uiIdx] = 0x80000;
		uiBsSize -= 0x80000;
	}
	uiHwHeaderSize[uiIdx] = uiBsSize;

	if (uiTotalBsLen != 0)
		h26x_setBsLen(uiTotalBsLen);

	// Set BSDMA Buffer //
	h26x_setBSDMA(pH264VirDecAddr->uiCMDBufAddr, uiHwHeaderNum, h26x_getPhyAddr(uiBsAddr), uiHwHeaderSize);
	h26x_setNextBsDmaBuf(h26x_getPhyAddr(pH264VirDecAddr->uiCMDBufAddr));

	return H264DEC_SUCCESS;
}
#endif

//! API : H264 check decoding finished and get result
/*!
    wait decode finish

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL: init fail
*/
INT32 h264Dec_IsDecodeFinish(void)
{
    //return h26x_checkINT();
    return h26x_waitINT();
}


//! API : H264 reset hw
/*!
    call swrest to fix hang issue

    @return void
*/
void h264Dec_reset(void)
{
    h26x_resetHW();
}

#if 0
static UINT32 query_dec_mem_size(UINT32 uiWidth, UINT32 uiHeight)
{
	UINT32 uiSizeContext  = SIZE_32X(sizeof(H264DEC_CTX));
	UINT32 uiSizeSideInfo = ((((uiWidth + 63)/64)*4 + 31)/32)*32*((uiHeight + 15)/16);
	UINT32 uiSizeColMVs   = ((uiWidth + 63)/64) * ((uiHeight + 15)/16) * 64;
	UINT32 uiSizeBSDMA    = (1 + H26X_MAX_BSDMA_NUM*2)*4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();

	return SIZE_4X(uiSizeContext + uiSizeSideInfo*2 + uiSizeColMVs + uiSizeBSDMA + uiSizeAPB + 128);
}

INT32 h264Dec_queryMemSize(H264DecMemInfo *pMemInfo)
{
	UINT8 buf[64];
	UINT32 buf_size;

	H264DecSpsObj sps;
	H26XDEC_VUI   vui;

	bstream sBstream;
    bstream *pBstream = &sBstream;

	buf_size = ebspTorbsp((UINT8 *)pMemInfo->uiHdrBsAddr, buf, ((pMemInfo->uiHdrBsLen > 64) ? 64 : pMemInfo->uiHdrBsLen));

	init_parse_bitstream(pBstream, buf, buf_size);

	if (H264DecSps(&sps, pBstream, &vui) != H264DEC_SUCCESS) {
		return H264DEC_FAIL;
    }

	pMemInfo->uiWidth = sps.uiMbWidth<<4;
	pMemInfo->uiHeight = sps.uiMbHeight<<4;
	pMemInfo->uiMemSize = query_dec_mem_size(pMemInfo->uiWidth, pMemInfo->uiHeight);

	return H264DEC_SUCCESS;
}

UINT32 h264Dec_getResYAddr(H26XDEC_VAR *pH264DecVar)
{
	H264DEC_CTX *pContext = pH264DecVar->pContext;

	return pContext->sH264VirDecAddr.uiResYAddr;
}
#endif
