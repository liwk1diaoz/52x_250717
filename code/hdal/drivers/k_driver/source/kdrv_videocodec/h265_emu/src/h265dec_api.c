/*
    H265DEC module driver for NT96660

    Interface for Init and Info

    @file       h265dec_api.c
    @ingroup    mICodecH265

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h26x_common.h"
#include "h26x_def.h"

#include "h265dec_api.h"
#include "h265dec_int.h"
#include "h265dec_cfg.h"
#include "h265dec_header.h"

//! Set memory address
/*!
    Set virtual address and add size

    @return void
*/
static void setMemoryAddr(UINT32 *puiVirAddr, UINT32 *puiVirMemAddr, UINT32 uiSize)
{
	*puiVirAddr = *puiVirMemAddr;
	*puiVirMemAddr = *puiVirMemAddr + SIZE_16X(uiSize);
	memset((void *)*puiVirAddr, 0, SIZE_16X(uiSize));
}

//! Clean memory address
/*!
    Set virtual address as null

    @return void
*/
static void cleanMemoryAddr(UINT32 *puiVirAddr)
{
	*puiVirAddr = (UINT32)NULL;
}


//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL   : size is too small or address is null
*/
static INT32 h265Dec_initInternalMemory(H265DEC_INIT *pH265DecInit, H26XDEC_VAR *pH265DecVar)
{
	UINT32 uiVirMemAddr, uiBufSize;
	H265DEC_CTX   *pContext;
	H26XDecAddr   *pH265VirDecAddr;
	int i;

	UINT32 uiSizeContext    = SIZE_32X(sizeof(H265DEC_CTX));
    //UINT32 uiSideInfoOffset = (SIZE_32X((((pH265DecInit->uiWidth + 63) >> 6) << 3)));
	//UINT32 uiSizeSideInfo = uiSideInfoOffset * ((pH265DecInit->uiHeight + 63) >> 6);
	UINT32 uiSizeColMVs   = ((((pH265DecInit->uiWidth + 63) >> 6) * ((pH265DecInit->uiHeight + 63) >> 6) << 6) << 2);
	UINT32 uiSizeBSDMA    = (1 + H26X_MAX_BSDMA_NUM * 2) * 4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();

	// ( rec_ref(ST_0), svc layer(non_ref(layer 1) and  ST_1(layer 2)), LT) //
	UINT32 uiFrmBufNum = FRM_IDX_MAX; //5
	if (uiFrmBufNum > FRM_IDX_MAX) {
		DBG_ERR("[h265Dec_initInternalMemory]H265 Decoder Frame Buffer Too More!\r\n");
	}

	uiVirMemAddr = pH265DecInit->uiDecBufAddr;
	//uiBufSize    = uiSizeContext + (uiSizeSideInfo + uiSizeColMVs) * uiFrmBufNum + uiSizeBSDMA; //total buf size
	uiBufSize    = SIZE_4X(uiSizeContext + (uiSizeColMVs) * uiFrmBufNum + uiSizeBSDMA + 128); //total buf size

	if (pH265DecInit->uiDecBufSize < uiBufSize) {
		DBG_ERR("[h265Dec_initInternalMemory]H265 Decoder Internal Buffer Size not enough!\r\n");
		return H265DEC_FAIL;
	}
    
	SetMemoryAddr(&pH265DecVar->uiAPBAddr, &uiVirMemAddr, uiSizeAPB);
	memset((void *)pH265DecVar->uiAPBAddr, 0, uiSizeAPB); 
    
	setMemoryAddr((UINT32 *)&pH265DecVar->pContext, &uiVirMemAddr, uiSizeContext);
	pContext = (H265DEC_CTX *)pH265DecVar->pContext;
	memset(pContext, 0, uiSizeContext);
	pH265VirDecAddr = &pContext->sH265VirDecAddr;
	pH265VirDecAddr->uiFrmBufNum = uiFrmBufNum;

	for (i = 0; i < (int)uiFrmBufNum; i++) {
		//setMemoryAddr(&pH265VirDecAddr->uiIlfSideInfoAddr[i], &uiVirMemAddr, uiSizeSideInfo);
		setMemoryAddr(&pH265VirDecAddr->uiColMvsAddr[i], &uiVirMemAddr, uiSizeColMVs);
	}
	setMemoryAddr(&pH265VirDecAddr->uiCMDBufAddr, &uiVirMemAddr, uiSizeBSDMA);

	for (i = 0; i < (FRM_IDX_MAX); i++) {
		pH265VirDecAddr->uiRefAndRecYAddr[i] = 0;
		pH265VirDecAddr->uiRefAndRecUVAddr[i] = 0;
		pH265VirDecAddr->iRefAndRecPOC[i] = -1;
	}

	return H265DEC_SUCCESS;
}

//! Reset internal memory
/*!
    Clear buffers address

    @return void
*/
static void h265Dec_resetInternalMemory(H26XDEC_VAR *pH265DecVar)
{
	H265DEC_CTX   *pContext = pH265DecVar->pContext;
	H26XDecAddr   *pH265VirDecAddr = &pContext->sH265VirDecAddr;
	int i;

	for (i = 0; i < (int)pH265VirDecAddr->uiFrmBufNum; i++) {
		//cleanMemoryAddr(&pH265VirDecAddr->uiIlfSideInfoAddr[i]);
		cleanMemoryAddr(&pH265VirDecAddr->uiColMvsAddr[i]);
	}
	cleanMemoryAddr(&pH265VirDecAddr->uiCMDBufAddr);

	cleanMemoryAddr((UINT32 *)pContext);
}

//! API : initialize decoder
/*!
    Initialize config, buffer and init HW

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL: init fail
*/
INT32 h265Dec_initDecoder(H265DEC_INIT *pH265DecInit, H26XDEC_VAR *pH265DecVar)
{
	H265DEC_CTX   *pContext;

	H265DecSeqCfg *pH265DecSeqCfg;
	H265DecPicCfg *pH265DecPicCfg;
	H265DecHdrObj *pH265DecHdrObj;

	H26XDecAddr   *pH265VirDecAddr;
    UINT8 i;

#if 0
	if (dma_isCacheAddr(pH265DecInit->uiDecBufAddr)) {
		DBG_ERR("[H265Dec] Decode Buf Addr (%x) should be non-cachable !\r\n", (unsigned int)(pH265DecInit->uiDecBufAddr));
		return H265DEC_FAIL;
	}
#endif

	if (h265Dec_initInternalMemory(pH265DecInit, pH265DecVar) != H265DEC_SUCCESS) {
		return H265DEC_FAIL;
	}

	pContext = pH265DecVar->pContext;
	pH265DecSeqCfg = &pContext->sH265DecSeqCfg;
	pH265DecPicCfg = &pContext->sH265DecPicCfg;
	pH265DecHdrObj = &pContext->sH265DecHdrObj;
	pH265VirDecAddr = &pContext->sH265VirDecAddr;

	for (i = 0; i < FRM_IDX_MAX; i++) {
		pH265VirDecAddr->iRefAndRecPOC[i] = -1;//initial
		pH265VirDecAddr->uiRefAndRecIsLT[i] = 0;
	}

	if (H265DecSeqHeader(pH265DecInit, pH265DecHdrObj, &pH265DecVar->stVUI) != H265DEC_SUCCESS) {
		return H265DEC_FAIL;
	}

	h265Dec_initCfg(pH265DecInit, pH265DecSeqCfg, pH265DecPicCfg, pH265DecHdrObj);

	H265Dec_initRegSet((H26XRegSet *)pH265DecVar->uiAPBAddr, pH265VirDecAddr);    // Init H265 RegSet //

	return H265DEC_SUCCESS;
}

//! API : stop decoder
/*!
    reset decoder

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL: init fail
*/
void h265Dec_stopDecoder(H26XDEC_VAR *pH265DecVar)
{

	h265Dec_resetInternalMemory(pH265DecVar);

	H265Dec_stopRegSet();
}

//! API :  H265 Pre-decode
/*!
    prepare deoder config

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL: init fail
*/
INT32 h265Dec_prepareOnePicture(H265DEC_INFO *pH265DecInfo, H26XDEC_VAR *pH265DecVar)
{
	H265DEC_CTX   *pContext = pH265DecVar->pContext;
	H265DecHdrObj *pH265DecHdrObj = &pContext->sH265DecHdrObj;
	H26XRegSet    *pH265DecRegSet = (H26XRegSet*)pH265DecVar->uiAPBAddr;
	H26XDecAddr   *pH265VirDecAddr = &pContext->sH265VirDecAddr;
	H265DecPicCfg *pH265DecPicCfg = &pContext->sH265DecPicCfg;

    UINT32 uiHwHeaderNum, uiHwHeaderAddr, uiHwHeaderSize[100];
	UINT32 uiBsSize =  (pH265DecInfo->uiCurBsSize == 0) ? pH265DecInfo->uiBSSize : pH265DecInfo->uiCurBsSize;
	UINT32 uiIdx;

	h26x_flushCache(pH265DecInfo->uiBSAddr, pH265DecInfo->uiBSSize);
	
	if (H265DecSlice(pH265DecInfo, pH265DecHdrObj, pH265DecInfo->uiBSAddr, pH265DecInfo->uiBSSize, 0) 
                        != H265DEC_SUCCESS) {
		DBG_ERR("[H265Dec] decode slice header error\r\n");
		return H265DEC_FAIL;
	}

	pH265VirDecAddr->uiHwBsAddr = pH265DecInfo->uiBSAddr;

	pH265DecRegSet->NAL_HDR_LEN_TOTAL_LEN = pH265DecInfo->uiBSSize;
	uiHwHeaderNum = (pH265DecInfo->uiCurBsSize == 0) ? (pH265DecInfo->uiBSSize + 0x7ffff)/0x80000 : (pH265DecInfo->uiCurBsSize + 0x7ffff)/0x80000;
	uiHwHeaderAddr = pH265DecInfo->uiBSAddr;

	for (uiIdx = 0; uiIdx < (uiHwHeaderNum - 1); uiIdx++) {
		uiHwHeaderSize[uiIdx] = 0x80000;
		uiBsSize -= 0x80000;
	}
	uiHwHeaderSize[uiIdx] = uiBsSize;
	// Set BSDMA Buffer //
	h26x_setBSDMA(pH265VirDecAddr->uiCMDBufAddr, uiHwHeaderNum, h26x_getPhyAddr(uiHwHeaderAddr), uiHwHeaderSize);

	{
		INT32 i, j;
		INT32 PocStCurrBefore[2] = {-1, -1};
    	H265DecSpsObj   *sps   = &pH265DecHdrObj->sH265DecSpsObj;
    	H265DecPpsObj   *pps   = &pH265DecHdrObj->sH265DecPpsObj;
		H265DecSliceObj *slice = &pH265DecHdrObj->sH265DecSliceObj;
		UINT32 uiFrmBufNum = pH265VirDecAddr->uiFrmBufNum;
		INT32 iColPOC, iColRefPOC, iCurrPOC, iCurrRefPOC, discalefactor;
        
        UINT32 length=0, numRpsCurrTempList0;
    	UINT32 numCTUs, bitsSliceSegmentAddress = 0, CUWidth;
    	UINT32 MinCbLog2SizeY, CtbLog2SizeY;

		pH265VirDecAddr->uiRecIdx++;

		//set PocStCurrBefore
		for (i = 0; i < (int)slice->short_term_ref_pic_set.NumNegativePics; i++) {
			if (slice->short_term_ref_pic_set.UsedByCurrPicS0 & (1 << i)) {
				PocStCurrBefore[i] = slice->PicOrderCntVal + slice->short_term_ref_pic_set.DeltaPocS0[i];
			}
		}

		//search rec idx if the rec idx is used for st or lt
		//for (j = 0; j < FRM_IDX_MAX; j++) {
			for (i = 0; i < 2; i++) {
				if (PocStCurrBefore[i] != -1 && 
                        pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRecIdx % uiFrmBufNum] == PocStCurrBefore[i]) {
					DBG_DUMP("st jump RecIdx=%d  \r\n", (int)(pH265VirDecAddr->uiRecIdx % uiFrmBufNum));
					pH265VirDecAddr->uiRecIdx++;
				}
			}

			for (i = 0; i < (int)slice->num_long_term_ref_pics; i++) {
				if (slice->pocLt[i] != -1 && 
                        slice->pocLt[i] == pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRecIdx % uiFrmBufNum]) {
					DBG_DUMP("lt jump RecIdx=%d	\r\n", (int)(pH265VirDecAddr->uiRecIdx % uiFrmBufNum));
					pH265VirDecAddr->uiRecIdx++;
				}
			}
		//}
		pH265VirDecAddr->uiRecIdx = pH265VirDecAddr->uiRecIdx % uiFrmBufNum;

		pH265VirDecAddr->uiResYAddr = pH265VirDecAddr->uiRefAndRecYAddr[pH265VirDecAddr->uiRecIdx];
		//set rec addr
		pH265VirDecAddr->uiRefAndRecYAddr[pH265VirDecAddr->uiRecIdx] = pH265DecInfo->uiSrcYAddr;
		pH265VirDecAddr->uiRefAndRecUVAddr[pH265VirDecAddr->uiRecIdx] = pH265DecInfo->uiSrcUVAddr;

		pH265DecRegSet->REC_Y_ADDR = h26x_getPhyAddr(pH265DecInfo->uiSrcYAddr);
		pH265DecRegSet->REC_C_ADDR = h26x_getPhyAddr(pH265DecInfo->uiSrcUVAddr);
		pH265DecRegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiColMvsAddr[pH265VirDecAddr->uiRecIdx]);
		pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRecIdx] = slice->PicOrderCntVal;

		//set ref addr
		pH265VirDecAddr->uiRef0Idx = 0;

		//search st ref frame
		for (i = 0; i < (int)uiFrmBufNum; i++) {
			if (PocStCurrBefore[0] >= 0 && PocStCurrBefore[0] == pH265VirDecAddr->iRefAndRecPOC[i]) {
				pH265VirDecAddr->uiRef0Idx = i;
				pH265DecRegSet->REF_Y_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiRefAndRecYAddr[i]);
				pH265DecRegSet->REF_C_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiRefAndRecUVAddr[i]);
				pH265DecRegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiColMvsAddr[i]);
				//pContext->sH265DecRegSet.uiEdIlfSideInfoAddr = pH265VirDecAddr->uiIlfSideInfoAddr[i];
			}
		}

		//search lt ref frame
		pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx] = 0;
		for (i = 0; i < (int)uiFrmBufNum; i++) {
			for (j = 0; j < (int)slice->num_long_term_ref_pics; j++) {
				if (((slice->UsedByCurrPicLt >> j) & 1) && slice->pocLt[j] != -1 && 
                        slice->pocLt[j] == pH265VirDecAddr->iRefAndRecPOC[i]) {
					pH265VirDecAddr->uiRef0Idx = i;
					pH265DecRegSet->REF_Y_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiRefAndRecYAddr[i]);
					pH265DecRegSet->REF_C_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiRefAndRecUVAddr[i]);
					pH265DecRegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiColMvsAddr[i]);
					//pContext->sH265DecRegSet.uiEdIlfSideInfoAddr = pH265VirDecAddr->uiIlfSideInfoAddr[i];

					//set is LT
					pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx] = 1;
				}
			}
		}

		if (slice->nalType == NAL_UNIT_CODED_SLICE_IDR_W_RADL || slice->nalType == NAL_UNIT_CODED_SLICE_IDR_N_LP) {
			for (i = 0; i < FRM_IDX_MAX; i++) {
				if ((pH265VirDecAddr->uiRecIdx) != (UINT32)i) {
					pH265VirDecAddr->iRefAndRecPOC[i] = -1; //clear all
				}
			}
		}

		//set col ref poc
		pH265VirDecAddr->iColRefAndRecPOC[pH265VirDecAddr->uiRecIdx] = pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRef0Idx];

		//set is intra
		pH265VirDecAddr->uiRefAndRecIsIntra[pH265VirDecAddr->uiRecIdx] = (slice->slice_type == I_SLICE);

		//distScaleFactor
		iColRefPOC = pH265VirDecAddr->iColRefAndRecPOC[pH265VirDecAddr->uiRef0Idx];
		iCurrPOC = slice->PicOrderCntVal;
		iColPOC = iCurrRefPOC = pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRef0Idx];

		if (pH265VirDecAddr->uiRefAndRecIsIntra[pH265VirDecAddr->uiRef0Idx] || 
                pH265VirDecAddr->uiRefAndRecIsIntra[pH265VirDecAddr->uiRecIdx]) {
			discalefactor = 256;
		} else if (pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx] || 
		        pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRef0Idx]) {
			discalefactor = 256; //if  ( bIsCurrRefLongTerm ||  bIsColRefLongTerm )
		} else {
			int iDiffPocD = iColPOC - iColRefPOC;
			int iDiffPocB = iCurrPOC - iCurrRefPOC;
			if (iDiffPocD == iDiffPocB) {
				discalefactor = 256;
			} else {
				int iTDB      = Clip3_JM(-128, 127, iDiffPocB);
				int iTDD      = Clip3_JM(-128, 127, iDiffPocD);
				//int iX        = (0x4000 + abs(iTDD / 2)) / iTDD;
				int iX;

                if (iTDD == 0)
                    return H265DEC_FAIL;
                else
                    iX = (0x4000 + abs(iTDD / 2)) / iTDD;
				discalefactor = Clip3_JM(-4096, 4095, (iTDB * iX + 32) >> 6);
			}
		}

		slice->discalefactor = discalefactor;

		DBG_IND("DistFactor=%d CurrPoc=%d CurrRefPOC=%d ColPoc=%d ColRefPoc=%d\r\n", (int)discalefactor, (int)iCurrPOC,
                    (int)iCurrRefPOC, (int)iColPOC, (int)iColRefPOC);
		DBG_IND("bIsCurrRefLT=%d bIsColRefLT=%d\r\n", (int)pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx],
                    (int)pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRef0Idx]);

		DBG_IND("refPoc=%d Poc=%d refIdx=%d, recIdx=%d TLid=%d\r\n",
                    (int)pH265VirDecAddr->iRefAndRecPOC[pH265VirDecAddr->uiRef0Idx], (int)slice->PicOrderCntVal,
                    (int)pH265VirDecAddr->uiRef0Idx, (int)pH265VirDecAddr->uiRecIdx, (int)slice->uiTemporalId);
		for (i = 0; i < (int)uiFrmBufNum; i++) {
			DBG_IND("Frame buffer[%d] POC=%d  \r\n", (int)(i), (int)(pH265VirDecAddr->iRefAndRecPOC[i]));
		}

        //pic_len_ref_list
    	numRpsCurrTempList0 = getNumRpsCurrTempList(slice);
    	if (numRpsCurrTempList0 > 1) {
    		length = 1;
    		numRpsCurrTempList0 --;
    		while (numRpsCurrTempList0 >>= 1) {
    			length ++;
    		}
    	}
    	slice->pic_len_ref_list = length;

        //pic_len_slice_addr
    	CUWidth = 1 << (sps->log2_min_luma_coding_block_size_minus3 + 3 + sps->log2_diff_max_min_luma_coding_block_size);
    	numCTUs = ((sps->pic_width_in_luma_samples + CUWidth - 1) / CUWidth) * ((sps->pic_height_in_luma_samples + CUWidth - 1) / CUWidth);
    	while (numCTUs > (UINT32)(1 << bitsSliceSegmentAddress)) {
    		bitsSliceSegmentAddress++;
    	}
    	sps->pic_len_slice_addr = bitsSliceSegmentAddress;

        //Log2MinCuQpDeltaSize
    	MinCbLog2SizeY = sps->log2_min_luma_coding_block_size_minus3 + 3;
    	CtbLog2SizeY = MinCbLog2SizeY + sps->log2_diff_max_min_luma_coding_block_size;
    	sps->Log2MinCuQpDeltaSize = CtbLog2SizeY - pps->diff_cu_qp_delta_depth;
	}

	H265Dec_prepareRegSet(pH265DecRegSet, pH265VirDecAddr, pContext);
    pH265DecPicCfg->uiPicDecNum++;

	return H265DEC_SUCCESS;
}

INT32 h265Dec_setNextBsBuf(H26XDEC_VAR *pH265DecVar, UINT32 uiBsAddr, UINT32 uiBsSize, UINT32 uiTotalBsSize)
{
	H265DEC_CTX   *pContext        = pH265DecVar->pContext;
	H26XDecAddr   *pH265VirDecAddr = &pContext->sH265VirDecAddr;

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
	if (uiTotalBsSize != 0)
		h26x_setBsLen(uiTotalBsSize);

	// Set BSDMA Buffer //
	h26x_setBSDMA(pH265VirDecAddr->uiCMDBufAddr, uiHwHeaderNum, h26x_getPhyAddr(uiBsAddr), uiHwHeaderSize);
	h26x_setNextBsDmaBuf(h26x_getPhyAddr(pH265VirDecAddr->uiCMDBufAddr));

	return H265DEC_SUCCESS;
}

//! API : H265 check decoding finished and get result
/*!
    wait decode finish

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL: init fail
*/
INT32 h265Dec_IsDecodeFinish(void)
{
	return h26x_waitINT();
}


//! API : H265 reset hw
/*!
    call swrest to fix hang issue

    @return void
*/
void h265Dec_reset(void)
{
	h26x_resetHW();
}

static UINT32 query_dec_mem_size(UINT32 uiWidth, UINT32 uiHeight)
{		
	UINT32 uiSizeContext  = SIZE_32X(sizeof(H265DEC_CTX));
	//UINT32 uiSizeSideInfo = ( (((((uiWidth + 63)>>6)<<3) + 31)<<5) * ((uiHeight + 63)>>6) )<<5;
	UINT32 uiSizeColMVs   = ((((uiWidth + 63) >> 6) * ((uiHeight + 63) >> 6) << 6) << 2);
	UINT32 uiSizeBSDMA    = (1 + H26X_MAX_BSDMA_NUM*2)*4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();

	return SIZE_4X(uiSizeContext/* + uiSizeSideInfo*2*/ + uiSizeColMVs*FRM_IDX_MAX + uiSizeBSDMA + uiSizeAPB + 128);
}


INT32 h265Dec_queryMemSize(H265DecMemInfo *pMemInfo)
{
	UINT8 buf[64];
	UINT32 buf_size;

	H265DecVpsObj vps;
	H265DecSpsObj sps;
	H26XDEC_VUI   vui;

	bstream sBstream;
    bstream *pBstream = &sBstream;

	if (pMemInfo->uiWidth == 0 || pMemInfo->uiHeight == 0) {
		buf_size = ebspTorbsp((UINT8 *)pMemInfo->uiHdrBsAddr, buf, ((pMemInfo->uiHdrBsLen > 128) ? 128 : pMemInfo->uiHdrBsLen));

		init_parse_bitstream(pBstream, buf, buf_size);

		if (H265DecVps(&vps, pBstream) != H265DEC_SUCCESS) {
			return H265DEC_FAIL;
		}

		if (H265DecSps(&sps, &vps, pBstream, 0, &vui) != H265DEC_SUCCESS) {
			return H265DEC_FAIL;
		}
		pMemInfo->uiWidth = sps.pic_width_in_luma_samples;
		pMemInfo->uiHeight = sps.pic_height_in_luma_samples;
	}
	pMemInfo->uiMemSize = query_dec_mem_size(pMemInfo->uiWidth, pMemInfo->uiHeight);

	return H265DEC_SUCCESS;
}

UINT32 h265Dec_getResYAddr(H26XDEC_VAR *pH265DecVar)
{
	H265DEC_CTX *pContext = pH265DecVar->pContext;

	return pContext->sH265VirDecAddr.uiResYAddr;
}
