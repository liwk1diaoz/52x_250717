/*
    H264ENC module driver for NT96660

    Interface for Init and Info

    @file       h264enc_api.c
    @ingroup    mICodecH264

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "kdrv_vdocdc_dbg.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_int.h"
#include "h26xenc_wrap.h"

#include "h264enc_api.h"
#include "h264enc_int.h"
#include "h264enc_wrap.h"
#include "h264enc_header.h"

#if USE_JND_DEFAULT_VALUE
    extern H26XEncJndCfg gEncJNDCfg;
    extern int gUseDefaultJNDCfg;
#endif


//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H264ENC_SUCCESS: init success
        - @b H264ENC_FAIL: size is too small or address is null
*/
static INT32 h264Enc_initInternalMemory(H264ENC_INIT *pInit, H26XENC_VAR *pVar)
{
	//H26XCOMN_CTX *pComnCtx;
	H26XEncAddr  *pAddr;

	UINT32 uiFrmBufNum    = 1 + (pInit->ucSVCLayer == 2) + (pInit->uiLTRInterval != 0);		// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiVirMemAddr   = pInit->uiEncBufAddr;
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = pInit->uiRecLineOffset*SIZE_16X(pInit->uiHeight);
	//UINT32 uiFrmSize      = SIZE_64X(pInit->uiWidth)*SIZE_16X(pInit->uiHeight);
	UINT32 uiSizeSideInfo = ((((pInit->uiWidth+63)/64)*4+31)/32)*32*((pInit->uiHeight+15)/16);
	UINT32 uiSizeColMVs   = ((pInit->uiWidth+63)/64)*((pInit->uiHeight+15)/16)*64;
	UINT32 uiSizeRcRef    = (((((pInit->uiHeight+15)/16)+3)/4)*4)*16;
	UINT32 uiSizeBSDMA    = (((pInit->uiHeight+15)/16)*2+1)*4;
	UINT32 uiSizeUsrQpMap = sizeof(UINT16)*(((SIZE_32X(pInit->uiWidth)+15)/16)*((pInit->uiHeight+15)/16));//hk
	//UINT32 uiSizeNaluLen  = (pInit->uiHeight+15)/16;
	UINT32 uiSizeOsgGcac  = 256;
	//UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	//UINT32 uiSizeLLC	  = h26x_getHwRegSize() + 64;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H264ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	//UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));

	//uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*4 + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC +
	//	uiSizeNaluLen + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + H264ENC_HEADER_MAXSIZE*2 + 256;		// 256 for 16x alignment
	uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + (uiSizeRcRef * 4) + uiSizeBSDMA + uiSizeColMVs +
		uiSizeUsrQpMap + uiSizeOsgGcac + uiSizeVdoCtx + uiSizeFuncCtx + H264ENC_HEADER_MAXSIZE;

	if (uiTotalBufSize > pInit->uiEncBufSize)
	{
		DBG_ERR("EncBufSize too small: 0x%08x < 0x%08x\r\n", (int)pInit->uiEncBufSize, (int)uiTotalBufSize);

        return H26XENC_FAIL;
	}

	if (pInit->uiEncBufAddr == 0)
	{
		DBG_ERR("EncBufAddr is NULL: 0x%08x\r\n", (int)pInit->uiEncBufAddr);
        return H26XENC_FAIL;
	}

	pVar->uiAPBAddr = pInit->uiAPBAddr;
	memset((void *)pVar->uiAPBAddr, 0, h26x_getHwRegSize());
	h26x_flushCache(pVar->uiAPBAddr, h26x_getHwRegSize());
/*
	DBG_IND("uiWidth           : 0x%08x\r\n", (int)pInit->uiWidth);
    DBG_IND("uiHeight          : 0x%08x\r\n", (int)pInit->uiHeight);
    DBG_IND("uiFrmSize         : 0x%08x\r\n", (int)uiFrmSize);
	DBG_IND("uiSizeSideInfo    : 0x%08x\r\n", (int)uiSizeSideInfo);
    DBG_IND("uiSizeColMVs      : 0x%08x\r\n", (int)uiSizeColMVs);
	DBG_IND("uiSizeRcRef       : 0x%08x\r\n", (int)uiSizeRcRef);
	DBG_IND("uiSizeBSDMA       : 0x%08x\r\n", (int)uiSizeBSDMA);
	DBG_IND("uiSizeUsrQpMap    : 0x%08x\r\n", (int)uiSizeUsrQpMap);
	DBG_IND("uiSizeOsgGcac     : 0x%08x\r\n", (int)uiSizeOsgGcac);
    DBG_IND("uiSizeVdoCtx      : 0x%08x\r\n", (int)uiSizeVdoCtx);
	DBG_IND("uiSizeFuncCtx     : 0x%08x\r\n", (int)uiSizeFuncCtx);
*/

	// virtaul buffer management //
		SetMemoryAddr((UINT32 *)&pVar->pVdoCtx, &uiVirMemAddr, uiSizeVdoCtx);
		memset(pVar->pVdoCtx, 0, uiSizeVdoCtx);
		SetMemoryAddr((UINT32 *)&pVar->pFuncCtx, &uiVirMemAddr, uiSizeFuncCtx);
		memset(pVar->pFuncCtx, 0, uiSizeFuncCtx);
		//SetMemoryAddr((UINT32 *)&pVar->pComnCtx, &uiVirMemAddr, uiSizeComnCtx);
		//memset(pVar->pComnCtx, 0, uiSizeComnCtx);
		//pComnCtx = pVar->pComnCtx;
		H264ENC_CTX *pVdoCtx  = pVar->pVdoCtx;

		// rate control config //
		/*
		pComnCtx->stRc.m_maxIQp = pInit->ucMaxIQp;
		pComnCtx->stRc.m_minIQp = pInit->ucMinIQp;
		pComnCtx->stRc.m_maxPQp	= pInit->ucMaxPQp;
		pComnCtx->stRc.m_minPQp = pInit->ucMinPQp;

		pComnCtx->uiWidth  = pInit->uiWidth;
		pComnCtx->uiHeight = pInit->uiHeight;
		pComnCtx->uiPicCnt = 0;
		pComnCtx->uiLTRInterval = pInit->uiLTRInterval;
		pComnCtx->ucSVCLayer = pInit->ucSVCLayer;
		*/

	// physical and veritaul buffer mangement //
		//pAddr = &pComnCtx->stVaAddr;

		//pAddr->uiFrmBufNum = uiFrmBufNum;
		pAddr = &pVdoCtx->stAddr;

		SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize);
    if(pInit->bGrayEn == 0 || pInit->bGrayColorEn == 1)
    	SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize/2);
    else
        pAddr->uiRecRefC[FRM_IDX_ST_0] = 0;
		SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_0], &uiVirMemAddr, uiSizeSideInfo);

		if (pInit->ucSVCLayer == 2)
		{
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize);
        if(pInit->bGrayEn == 0 || pInit->bGrayColorEn == 1)
		SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize/2);
        else
            pAddr->uiRecRefC[FRM_IDX_ST_1] = 0;
		SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiSizeSideInfo);
		}

		if (pInit->uiLTRInterval != 0)
		{
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize);
        if(pInit->bGrayEn == 0 || pInit->bGrayColorEn == 1)
		    SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize/2);
        else
            pAddr->uiRecRefC[FRM_IDX_LT_0] = 0;
		SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_LT_0], &uiVirMemAddr, uiSizeSideInfo);
		}

		pAddr->uiRecRefY[FRM_IDX_NON_REF] = pAddr->uiRecRefY[FRM_IDX_ST_0];
		pAddr->uiRecRefC[FRM_IDX_NON_REF] = pAddr->uiRecRefC[FRM_IDX_ST_0];
		pAddr->uiSideInfo[0][FRM_IDX_NON_REF] = pAddr->uiSideInfo[0][FRM_IDX_ST_0];

		SetMemoryAddr(&pAddr->uiColMvs[0], &uiVirMemAddr, uiSizeColMVs);	// h264 only use 1 buffer //
		memset((void *)pAddr->uiColMvs[0], 0, uiSizeColMVs);//HACK
		h26x_flushCache(pAddr->uiColMvs[0], uiSizeColMVs);

		SetMemoryAddr(&pAddr->uiBsdma, &uiVirMemAddr, uiSizeBSDMA);
		//SetMemoryAddr(&pAddr->uiNaluLen, &uiVirMemAddr, uiSizeNaluLen);
		SetMemoryAddr(&pAddr->uiSeqHdr, &uiVirMemAddr, H264ENC_HEADER_MAXSIZE);
		SetMemoryAddr(&pAddr->uiSliceHdr, &uiVirMemAddr, H264ENC_HEADER_MAXSIZE);
		//SetMemoryAddr(&pAddr->uiAPBAddr, &uiVirMemAddr, uiSizeAPB);
		//memset((void *)pAddr->uiAPBAddr, 0, uiSizeAPB);
		//SetMemoryAddr(&pAddr->uiLLCAddr, &uiVirMemAddr, uiSizeLLC);
		//memset((void *)pAddr->uiLLCAddr, 0, uiSizeLLC);

		SetMemoryAddr(&pAddr->uiRRC[0][0], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[0][1], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[1][0], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[1][1], &uiVirMemAddr, uiSizeRcRef);

		// malloc and set function context //
		H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;
		SetMemoryAddr(&pFuncCtx->stUsrQp.uiIntQpMapAddr, &uiVirMemAddr, uiSizeUsrQpMap);

		//pFuncCtx->stSliceSplit.uiNaluNum = 1;
		pFuncCtx->stUsrQp.uiUsrQpMapSize = uiSizeUsrQpMap;

		//SetMemoryAddr(&pFuncCtx->stUsrQp.uiQpMapAddr, &uiVirMemAddr, uiSizeUsrQpMap);

		SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr0, &uiVirMemAddr, uiSizeOsgGcac);
		memset((void *)pFuncCtx->stOsg.uiGcacStatAddr0, 0, uiSizeOsgGcac);
		h26x_flushCache(pFuncCtx->stOsg.uiGcacStatAddr0, uiSizeOsgGcac);
		SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr1, &uiVirMemAddr, uiSizeOsgGcac);
		memset((void *)pFuncCtx->stOsg.uiGcacStatAddr1, 0, uiSizeOsgGcac);
		h26x_flushCache(pFuncCtx->stOsg.uiGcacStatAddr1, uiSizeOsgGcac);
		// motion aq //
		pFuncCtx->stMAq.uiMotLineOffset = (pInit->uiWidth+511)/512;
		pFuncCtx->stMAq.uiMotSize = (pFuncCtx->stMAq.uiMotLineOffset * 4) * (SIZE_16X(pInit->uiHeight)/16);
		SetMemoryAddr(&pFuncCtx->stMAq.uiMotAddr[0], &uiVirMemAddr, pFuncCtx->stMAq.uiMotSize);
		SetMemoryAddr(&pFuncCtx->stMAq.uiMotAddr[1], &uiVirMemAddr, pFuncCtx->stMAq.uiMotSize);
		SetMemoryAddr(&pFuncCtx->stMAq.uiMotAddr[2], &uiVirMemAddr, pFuncCtx->stMAq.uiMotSize);
		memset((void *)pFuncCtx->stMAq.uiMotAddr[0], 0, pFuncCtx->stMAq.uiMotSize);
		memset((void *)pFuncCtx->stMAq.uiMotAddr[1], 0, pFuncCtx->stMAq.uiMotSize);
		memset((void *)pFuncCtx->stMAq.uiMotAddr[2], 0, pFuncCtx->stMAq.uiMotSize);
		h26x_flushCache(pFuncCtx->stMAq.uiMotAddr[0], pFuncCtx->stMAq.uiMotSize);
		h26x_flushCache(pFuncCtx->stMAq.uiMotAddr[1], pFuncCtx->stMAq.uiMotSize);
		h26x_flushCache(pFuncCtx->stMAq.uiMotAddr[2], pFuncCtx->stMAq.uiMotSize);
		pVar->uiCtxSize = uiVirMemAddr - pInit->uiEncBufAddr;

	if (pVar->uiCtxSize > pInit->uiEncBufSize) {
		DBG_ERR("End check encBufSize too small: 0x%08x < 0x%08x\r\n", (int)pInit->uiEncBufSize, (int)pVar->uiCtxSize);

        return H26XENC_FAIL;
	}

    return H26XENC_SUCCESS;
}

void _h264Enc_initEncoderDump(H264ENC_INIT *pInit)
{
	DBG_IND("[KDRV_H264ENC] W = %u, H = %u, DisW = %u, buf = (0x%08x, %d), GOP = %u, DB = (%u, %d, %d), Qpoff = (%d, %d), svc = %u, ltr = (%u, %u), bitrate = %u, fr = %u, I_QP = (%u, %u, %u), P_QP = (%u, %u, %u), ip_wei = %d, sta = %u, FBCEn = %u, Gray = %u, FastSear = %u, HwPad = %u, rotate = %u, d2d = %u, profile = %u, level = %u,  entropy = %u, ucTrans8x8 = %u, bForwardRecChrmEn = %u, VUI = (%u, %u, %u, %u, %u, %u, %u, %u, %u)\r\n",
		(int)pInit->uiWidth,
		(int)pInit->uiHeight,
		(int)pInit->uiDisplayWidth,
		(int)pInit->uiEncBufAddr,
		(int)pInit->uiEncBufSize,
		(int)pInit->uiGopNum,
		(int)pInit->ucDisLFIdc,
		(int)pInit->cDBAlpha,
		(int)pInit->cDBBeta,
		(int)pInit->cChrmQPIdx,
		(int)pInit->cSecChrmQPIdx,
		(int)pInit->ucSVCLayer,
		(int)pInit->uiLTRInterval,
		(int)pInit->bLTRPreRef,
		(int)pInit->uiBitRate,
		(int)pInit->uiFrmRate,
		(int)pInit->ucIQP,
		(int)pInit->ucMinIQp,
		(int)pInit->ucMaxIQp,
		(int)pInit->ucPQP,
		(int)pInit->ucMinPQp,
		(int)pInit->ucMaxPQp,
		(int)pInit->iIPWeight,
		(int)pInit->uiStaticTime,
		(int)pInit->bFBCEn,
		(int)pInit->bGrayEn,
		(int)pInit->bFastSearchEn,
		(int)pInit->bHwPaddingEn,
		(int)pInit->ucRotate,
		(int)pInit->bD2dEn,
		(int)pInit->eProfile,
		(int)pInit->ucLevelIdc,
		(int)pInit->eEntropyMode,
		(int)pInit->bTrans8x8En,
		(int)pInit->bForwardRecChrmEn,
		(int)pInit->bVUIEn,
		(int)pInit->usSarWidth,
		(int)pInit->usSarHeight,
		(int)pInit->ucMatrixCoef,
		(int)pInit->ucTransferCharacteristics,
		(int)pInit->ucColourPrimaries,
		(int)pInit->ucVideoFormat,
		(int)pInit->ucColorRange,
		(int)pInit->bTimeingPresentFlag);
}

//! API : initialize encoder
/*!
    Initialize config, buffer and write header, and init HW

    @return
        - @b H264ENC_SUCCESS: init success
        - @b H264ENC_FAIL: init fail
*/
INT32  h264Enc_initEncoder(H264ENC_INIT *pInit, H26XENC_VAR *pVar)
{
	//H26XCOMN_CTX *pComnCtx;

	/* dump param settings */
	_h264Enc_initEncoderDump(pInit);

    /* check horizontal extension */
    if(pInit->uiWidth % 16 && pInit->uiWidth != 1080) {
        DBG_ERR("[H264Enc] encode width %d should be 16X !\r\n ", (int)(pInit->uiWidth));

        return H26XENC_FAIL;
    }

    if (h264Enc_initInternalMemory(pInit, pVar) == H26XENC_FAIL) {
    	DBG_ERR("Error : h264Enc_initInternalMemory fail. \r\n");

		return H26XENC_FAIL;
    }

	h264Enc_initCfg(pInit, pVar);

	//h264Enc_encSeqHeader(pVar->pVdoCtx, pVar->pComnCtx);

	//pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	//h264Enc_wrapSeqCfg((H264ENC_CTX *)pVar->pVdoCtx, (H26XEncAddr *)&pComnCtx->stVaAddr);
	h264Enc_wrapSeqCfg((H26XRegSet *)pVar->uiAPBAddr, (H264ENC_CTX *)pVar->pVdoCtx);
#if 0
	// init fro //
	{
		H264EncFroCfg sFroAttr = {
			1,
			{{243, 250, 255, 243}, {140, 210, 255, 210}, {80, 240, 511, 150}}, // dc
			{{219, 238, 227, 219}, {60, 85, 85, 85}, {60, 85, 255, 75}}, // ac
			{{24, 12, 28, 24}, {0, 0, 0, 0}, {12, 6, 63, 0}}, // st
			{{2, 6, 2, 2}, {0, 0, 0, 0}, {3, 7, 3, 0}} // mx
		};
		h264Enc_setFroCfg(pVar, &sFroAttr);
	}

	// init rdo //
	{
		H264EncRdoCfg sRdoAttr = {
			1,
			//{{5.0, 5.0, 5.0, 0.0}, {5.0, 5.0, 5.0, 5.0}, {5.0, 5.0, 0.0, 5.0}},
			{{10, 10, 10, 0}, {10, 10, 10, 10}, {10, 10, 0, 10}},
			8, 8, 8, 1, 1, 1, 0,
			8, 8, 0, 0, 0, 0,
			5, 5, 8
		};
		h264Enc_setRdoCfg(pVar, &sRdoAttr);
	}

	// init sde //
	h26XEnc_setInitSde((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr);

	// init rrc //
	{
		H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

		pFuncCtx->stRowRc.ucMinCostTh = 9;
	}

	// enable psnr //
	H26XEnc_setPSNRCfg(pVar, TRUE);

	// init isp ratio //
    {
        H26XEncIspRatioCfg sIspRatioCfg={100, 100, 100, 100};

        h26XEnc_SetIspRatio(pVar, &sIspRatioCfg);
    }

#endif
	return H26XENC_SUCCESS;
}

void _h264Enc_prepareOnePictureDump(H264ENC_INFO *pInfo)
{
//	DBG_MSG("[KDRV_H264ENC] PicType = %d, Src = (0x%08x, 0x%08x, %u, %u), SrcCIv = %u, SrcOut = (%u, %u, 0x%08x, 0x%08x, %u, %u), SrcD2D =(%d, %d), BsOut = (0x%08x, %u), NalLenAddr = 0x%08x, timestamp = %u, TemId = %u, SkipFrmEn = %u, SeqHdrEn = %u, SdeCfg = (%u, %u, %u, %u, %u)\r\n",
    DBG_MSG("[KDRV_H264ENC] PicType = %d, Src = (0x%08x, 0x%08x, %u, %u), SrcCIv = %u, SrcOut = (%u, %u, 0x%08x, 0x%08x, %u, %u), SrcD2D =(%d, %d), BsOut = (0x%08x, %u), NalLenAddr = 0x%08x, timestamp = %u, TemId = %u, SkipFrmEn = %u, SeqHdrEn = %u\r\n",
		(int)pInfo->ePicType,
		(int)pInfo->uiSrcYAddr,
		(int)pInfo->uiSrcCAddr,
		(int)pInfo->uiSrcYLineOffset,
		(int)pInfo->uiSrcCLineOffset,
		(int)pInfo->bSrcCbCrIv,
		(int)pInfo->bSrcOutEn,
		(int)pInfo->ucSrcOutMode,
		(int)pInfo->uiSrcOutYAddr,
		(int)pInfo->uiSrcOutCAddr,
		(int)pInfo->uiSrcOutYLineOffset,
		(int)pInfo->uiSrcOutCLineOffset,
		(int)pInfo->bSrcD2DEn,
		(int)pInfo->ucSrcD2DMode,
		(int)pInfo->uiBsOutBufAddr,
		(int)pInfo->uiBsOutBufSize,
		(int)pInfo->uiNalLenOutAddr,
		(int)pInfo->uiSrcTimeStamp,
		(int)pInfo->uiTemporalId,
		(int)pInfo->bSkipFrmEn,
		(int)pInfo->bGetSeqHdrEn//,
		//(int)pInfo->SdeCfg.bEnable,
		//(int)pInfo->SdeCfg.uiWidth,
		//(int)pInfo->SdeCfg.uiHeight,
		//(int)pInfo->SdeCfg.uiYLofst,
		//(int)pInfo->SdeCfg.uiCLofst
		);
}

//! API : encode picture for encoder
/*!
    Call prepare function and start HW to encode,

        wait for interrupt and get result

    @return
        - @b H264ENC_SUCCESS: encode success
        - @b H264ENC_FAIL   : encode fail
*/
INT32 h264Enc_prepareOnePicture(H264ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet    *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	H264EncPicCfg *pPicCfg  = &pVdoCtx->stPicCfg;

	// check info data correct not ready //

	// set data to picture config //
	pPicCfg->ePicType = pInfo->ePicType;

    h264Enc_initRrc( pVdoCtx, &pFuncCtx->stRowRc, IS_PSLICE(pPicCfg->ePicType), pPicCfg->uiPicCnt, pFuncCtx->stRowRc.usInitQp, pPicCfg->uiAvgQP);
	h264Enc_wrapPicCfg(pRegSet, pInfo, pVar);

	h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr);

    h264Enc_wrapRdoCfg2(pRegSet, pVdoCtx);

	h26xEnc_wrapRowRcPicUpdate(pRegSet, &pFuncCtx->stRowRc, pPicCfg->uiPicCnt, IS_PSLICE(pPicCfg->ePicType));


    return H26XENC_SUCCESS;
}

//INT32 h264Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt)
INT32 h264Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	//H26XRegSet    *pRegSet  = (H26XRegSet *)dma_getNonCacheAddr(pVar->uiAPBAddr);
	H26XRegSet    *pRegSet  = (H26XRegSet *)dma_getCacheAddr(pVar->uiAPBAddr);
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	if (enc_mode)
	{
		// direct mode //
		h26x_getEncReport(pRegSet->CHK_REPORT);	// shall remove while link-list //
		pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;
	}
	else
	{
		// update aq result //
		//******************************************************//
		// in 510 platform, get result need to shit 1 word to get correct               //
		// H26X_SRAQ_ISUM_ACT_LOG  "+ 1 "  is for shit 1 word to get data        //
		// in V7 no need to add 										    //
		// this issue need to check mode                                                       //
		//******************************************************//
		pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;
	}

    pPicCfg->uiAvgQP = (pRegSet->CHK_REPORT[H26X_RRC_QP_SUM] + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;
	h26XEnc_getRowRcState(pVdoCtx->stPicCfg.ePicType, pRegSet, &pFuncCtx->stRowRc, 0);
	h264Enc_updatePicCfg(pVdoCtx);
	return H26XENC_SUCCESS;
}

INT32 h264Enc_setRdoCfg(H26XENC_VAR *pVar, H264EncRdoCfg *pRdo)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	//H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H264EncRdoCfg *pRdoCfg = &pVdoCtx->stRdoCfg;

	// check info data correct not ready //

	memcpy(pRdoCfg, pRdo, sizeof(H264EncRdoCfg));

	//h264Enc_wrapRdoCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);
	h264Enc_wrapRdoCfg((H26XRegSet *)pVar->uiAPBAddr, pVdoCtx);
	return H26XENC_SUCCESS;
}

INT32 h264Enc_setFroCfg(H26XENC_VAR *pVar, H264EncFroCfg *pFro)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	//H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H264EncFroCfg *pFroCfg = &pVdoCtx->stFroCfg;

	// check info data correct not ready //

	memcpy(pFroCfg, pFro, sizeof(H264EncFroCfg));

	//h264Enc_wrapFroCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);
	h264Enc_wrapFroCfg((H26XRegSet *)pVar->uiAPBAddr, pVdoCtx);
	return H26XENC_SUCCESS;
}
// for emualtion only //
void h264Enc_setPicQP(UINT8 ucQP, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	pPicCfg->ucSliceQP = ucQP;
}

#if 0
UINT32 h264Enc_queryMemSize(H26XEncMeminfo *pInfo)
{
	UINT32 uiFrmBufNum    = 1 + (pInfo->ucSVCLayer == 2) + (pInfo->uiLTRInterval != 0);		// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = SIZE_64X(pInfo->uiWidth)*SIZE_16X(pInfo->uiHeight);
	UINT32 uiSizeSideInfo = ((((pInfo->uiWidth+63)/64)*4+31)/32)*32*((pInfo->uiHeight+15)/16);
	UINT32 uiSizeColMVs   = ((pInfo->uiWidth+63)/64)*((pInfo->uiHeight+15)/16)*64;
	UINT32 uiSizeRcRef    = (((((pInfo->uiHeight+15)/16)+3)/4)*4)*16;
	UINT32 uiSizeBSDMA    = (((pInfo->uiHeight+15)/16)*2+1)*4;
	UINT32 uiSizeUsrQpMap = sizeof(UINT16)*(((SIZE_32X(pInfo->uiWidth)+15)/16)*((pInfo->uiHeight+15)/16));//hk
	UINT32 uiSizeNaluLen  = (pInfo->uiHeight+15)/16;
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = h26x_getHwRegSize() + 64;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H264ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));

	uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*4 + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC +
		uiSizeNaluLen + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + H264ENC_HEADER_MAXSIZE*2 + 256;		// 256 for 16x alignement //

	return SIZE_4X(uiTotalBufSize);
}

void h264Enc_getSeqHdr(H26XENC_VAR *pVar, UINT32 *pAddr, UINT32 *pSize)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	*pSize = pVdoCtx->uiSeqHdrLen;
	*pAddr = pComnCtx->stVaAddr.uiSeqHdr;
}

UINT32 h264Enc_getGopNum(H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;

	return pVdoCtx->stSeqCfg.uiGopNum;
}

UINT32 h264Enc_getNxtPicType(H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;

	return (UINT32)pVdoCtx->stPicCfg.eNxtPicType;
}

UINT32 h264Enc_getSVCLabel(H26XENC_VAR *pVar)
{
    H264ENC_CTX  *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
    H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
    UINT32 ref_poc, svc_order, svc_label = 0;

    ref_poc = (pSeqCfg->uiLTRInterval) ? (pPicCfg->sPoc % pSeqCfg->uiLTRInterval) : (UINT32)pPicCfg->sPoc;
	if (pSeqCfg->ucSVCLayer == 2) {
        svc_order = ref_poc%4;
        if (0 == svc_order)
            svc_label = 0;
        else if (2 == svc_order)
            svc_label = 1;
        else
            svc_label = 2;
	}
	else if (pSeqCfg->ucSVCLayer == 1) {
        svc_label = (ref_poc%2);
	}
    return svc_label;
}
#endif

