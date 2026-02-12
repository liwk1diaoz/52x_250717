/*
    H264ENC module driver for NT96660

    Interface for Init and Info

    @file       h264enc_api.c
    @ingroup    mICodecH264

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "nvt_vdocdc_dbg.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_int.h"
#include "h26xenc_wrap.h"

#include "h264enc_api.h"
#include "h264enc_int.h"
#include "h264enc_wrap.h"
#include "h264enc_header.h"

#if H264_FRO_IP_SYNC
int gH264FroSync = 0;
#endif
#if JND_DEFAULT_ENABLE
    extern H26XEncJndCfg gEncJNDCfg;
#endif
#if H26X_SET_PROC_PARAM
extern int gH264RowRCStopFactor;
extern int gH264PReduce16Planar;
extern int gH264MaxDecFrameBuf;
#endif
extern int gRRCNDQPStep;
extern int gRRCNDQPRange;
#if H26X_MEM_USAGE
extern H26XMemUsage g_mem_usage[2][16];
#endif
extern int gH264FrameNumGapAllow;
extern int gFixSPSLog2Poc;
//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H264ENC_SUCCESS: init success
        - @b H264ENC_FAIL: size is too small or address is null
*/
static INT32 h264Enc_initInternalMemory(H264ENC_INIT *pInit, H26XENC_VAR *pVar)
{
	H26XCOMN_CTX *pComnCtx;
	H26XEncAddr  *pAddr;

	UINT32 uiFrmBufNum    = 1 + (pInit->ucSVCLayer == 2) + (pInit->uiLTRInterval != 0);		// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiVirMemAddr   = pInit->uiEncBufAddr;
	UINT32 uiTotalBufSize = 0;

	//UINT32 uiFrmSize      = pInit->uiRecLineOffset*SIZE_16X(pInit->uiHeight);
	UINT32 uiFrmSize      = SIZE_64X(pInit->uiWidth)*SIZE_16X(pInit->uiHeight);
	UINT32 uiSizeSideInfo = ((((pInit->uiWidth+63)/64)*4+31)/32)*32*((pInit->uiHeight+15)/16);
	UINT32 uiSizeColMVs   = (pInit->bColMvEn==0)? 0: ((pInit->uiWidth+63)/64)*((pInit->uiHeight+15)/16)*64;
	UINT32 uiSizeRcRef    = (((((pInit->uiHeight+15)/16)+3)/4)*4)*16;
	UINT32 uiSizeBSDMA    = (((pInit->uiHeight+15)/16)*2+1)*4;
	UINT32 uiSizeUsrQpMap = sizeof(UINT16)*(((SIZE_32X(pInit->uiWidth)+15)/16)*((pInit->uiHeight+15)/16));//hk
	UINT32 uiSizeMDMap    = ((SIZE_512X(pInit->uiWidth) >> 4) * (SIZE_16X(pInit->uiHeight) >> 4)) >> 3;
	UINT32 uiSizeNaluLen  = SIZE_256X(((pInit->uiHeight+15)/16)*4) + 16;
	UINT32 uiSeqHdrSize   = 256;
	UINT32 uiPicHdrSize   = (((pInit->uiHeight+15)/16)*20) + uiSeqHdrSize + 128;	// each slice header + sequence header + sei(128) //
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = (H26X_ENC_MODE == 0) ? h26x_getHwRegSize() + 64 : 0;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H264ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));
	#if RRC_BY_FRAME_LEVEL
	int i;
	#endif
	UINT8 k=0;
	UINT32 uiRecBufAddr=0;
	UINT32 slice_num = 0;
	UINT32 special_p_bs_size = ((pInit->bGDRIFrm == 1)? uiFrmSize: 0);
	UINT32 special_i_nalu_len = ((pInit->bGDRIFrm == 1)? SIZE_256X(((pInit->uiHeight+15)/16)*4) + 16: 0);

	slice_num = ((pInit->uiHeight+15)/16);
	if(slice_num % 64 == 0){
		//ic limitaion: 64X need more 256 byte
		uiSizeNaluLen += 256;
	}

#if H26X_USE_DIFF_MAQ
	if (pInit->bRecBufComm) {
		uiTotalBufSize = (uiSizeSideInfo *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignment
	} else {
		uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignment
	}
#else
	if (pInit->bRecBufComm) {
		uiTotalBufSize = (uiSizeSideInfo *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*3 +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignment
	} else {
		uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*3 +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignment
	}
#endif
	uiTotalBufSize += (special_p_bs_size + special_i_nalu_len);

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

	DBG_IND("uiWidth           : 0x%08x\r\n", (int)pInit->uiWidth);
	DBG_IND("uiHeight          : 0x%08x\r\n", (int)pInit->uiHeight);
	DBG_IND("uiFrmSize         : 0x%08x\r\n", (int)uiFrmSize);
	DBG_IND("uiSizeSideInfo    : 0x%08x\r\n", (int)uiSizeSideInfo);
	DBG_IND("uiSizeColMVs      : 0x%08x\r\n", (int)uiSizeColMVs);
	DBG_IND("uiSizeRcRef       : 0x%08x\r\n", (int)uiSizeRcRef);
	DBG_IND("uiSizeBSDMA       : 0x%08x\r\n", (int)uiSizeBSDMA);
	DBG_IND("uiSizeUsrQpMap    : 0x%08x\r\n", (int)uiSizeUsrQpMap);
	DBG_IND("uiSizeMDMap       : 0x%08x\r\n", (int)uiSizeMDMap);
	DBG_IND("uiSizeOsgGcac     : 0x%08x\r\n", (int)uiSizeOsgGcac);
	DBG_IND("uiSizeAPB         : 0x%08x\r\n", (int)uiSizeAPB);
	DBG_IND("uiSizeVdoCtx      : 0x%08x\r\n", (int)uiSizeVdoCtx);
	DBG_IND("uiSizeFuncCtx     : 0x%08x\r\n", (int)uiSizeFuncCtx);
	DBG_IND("uiSizeComnCtx     : 0x%08x\r\n", (int)uiSizeComnCtx);


	// virtaul buffer management //
	{
		SetMemoryAddr((UINT32 *)&pVar->pVdoCtx, &uiVirMemAddr, uiSizeVdoCtx);
		memset(pVar->pVdoCtx, 0, uiSizeVdoCtx);
		SetMemoryAddr((UINT32 *)&pVar->pFuncCtx, &uiVirMemAddr, uiSizeFuncCtx);
		memset(pVar->pFuncCtx, 0, uiSizeFuncCtx);
		SetMemoryAddr((UINT32 *)&pVar->pComnCtx, &uiVirMemAddr, uiSizeComnCtx);
		memset(pVar->pComnCtx, 0, uiSizeComnCtx);

		pComnCtx = pVar->pComnCtx;

		// rate control config //
		pComnCtx->stRc.m_maxIQp = pInit->ucMaxIQp;
		pComnCtx->stRc.m_minIQp = pInit->ucMinIQp;
		pComnCtx->stRc.m_maxPQp	= pInit->ucMaxPQp;
		pComnCtx->stRc.m_minPQp = pInit->ucMinPQp;

		pComnCtx->uiWidth  = pInit->uiWidth;
		pComnCtx->uiHeight = pInit->uiHeight;
		pComnCtx->uiPicCnt = 0;
		pComnCtx->uiLTRInterval = pInit->uiLTRInterval;
		pComnCtx->ucSVCLayer = pInit->ucSVCLayer;

		// header buffer size //
		pComnCtx->uiPicHdrBufSize = uiPicHdrSize;
	}

	// physical and veritaul buffer mangement //
	{
		pAddr = &pComnCtx->stVaAddr;

		pAddr->uiFrmBufNum = uiFrmBufNum;

		if (pInit->bRecBufComm) {
			uiRecBufAddr = pInit->uiRecBufAddr[k++];
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiRecBufAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiRecBufAddr, uiFrmSize/2);
		} else {
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize/2);
		}
		SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_0], &uiVirMemAddr, uiSizeSideInfo);

		if (pInit->ucSVCLayer == 2)
		{
			if (pInit->bRecBufComm) {
				uiRecBufAddr = pInit->uiRecBufAddr[k++];
				SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiRecBufAddr, uiFrmSize);
				SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiRecBufAddr, uiFrmSize/2);
			} else {
				SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize);
				SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize/2);
			}
			SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiSizeSideInfo);
		}

		if (pInit->uiLTRInterval != 0)
		{
			if (pInit->bRecBufComm) {
				uiRecBufAddr = pInit->uiRecBufAddr[k++];
				SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiRecBufAddr, uiFrmSize);
				SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiRecBufAddr, uiFrmSize/2);
			} else {
				SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize);
				SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize/2);
			}
			SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_LT_0], &uiVirMemAddr, uiSizeSideInfo);
		}

		pAddr->uiRecRefY[FRM_IDX_NON_REF] = pAddr->uiRecRefY[FRM_IDX_ST_0];
		pAddr->uiRecRefC[FRM_IDX_NON_REF] = pAddr->uiRecRefC[FRM_IDX_ST_0];
		pAddr->uiSideInfo[0][FRM_IDX_NON_REF] = pAddr->uiSideInfo[0][FRM_IDX_ST_0];

		SetMemoryAddr(&pAddr->uiColMvs[0], &uiVirMemAddr, uiSizeColMVs);	// h264 only use 1 buffer //
		memset((void *)pAddr->uiColMvs[0], 0, uiSizeColMVs);
		h26x_cache_clean(pAddr->uiColMvs[0], uiSizeColMVs);
		SetMemoryAddr(&pAddr->uiBsdma, &uiVirMemAddr, uiSizeBSDMA);
		SetMemoryAddr(&pAddr->uiNaluLen, &uiVirMemAddr, uiSizeNaluLen);
		SetMemoryAddr(&pAddr->uiSeqHdr, &uiVirMemAddr, uiSeqHdrSize);
		SetMemoryAddr(&pAddr->uiPicHdr, &uiVirMemAddr, uiPicHdrSize);
		SetMemoryAddr(&pAddr->uiAPBAddr, &uiVirMemAddr, uiSizeAPB);
		memset((void *)pAddr->uiAPBAddr, 0, uiSizeAPB);
		if (uiSizeLLC) {
			SetMemoryAddr(&pAddr->uiLLCAddr, &uiVirMemAddr, uiSizeLLC);
			memset((void *)pAddr->uiLLCAddr, 0, uiSizeLLC);
		}

		#if RRC_BY_FRAME_LEVEL
		for (i = 0; i < MAX_RRC_FRAME_LEVEL; i++) {
			SetMemoryAddr(&pAddr->uiRRC[i][0], &uiVirMemAddr, uiSizeRcRef);
			SetMemoryAddr(&pAddr->uiRRC[i][1], &uiVirMemAddr, uiSizeRcRef);
		}
		#else
		SetMemoryAddr(&pAddr->uiRRC[0][0], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[0][1], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[1][0], &uiVirMemAddr, uiSizeRcRef);
		SetMemoryAddr(&pAddr->uiRRC[1][1], &uiVirMemAddr, uiSizeRcRef);
		#endif

		// malloc and set function context //
		{
			H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

			pFuncCtx->stSliceSplit.uiNaluNum = 1;
			pFuncCtx->stUsrQp.uiUsrQpMapSize = uiSizeUsrQpMap;

			SetMemoryAddr(&pFuncCtx->stUsrQp.uiQpMapAddr, &uiVirMemAddr, uiSizeUsrQpMap);

            #if H26X_USE_DIFF_MAQ
            pFuncCtx->stMAq.stAddrCfg.ucMotBufNum = 2; // MAQ and MAQ diff
            #else
			pFuncCtx->stMAq.stAddrCfg.ucMotBufNum = 1;
            #endif
		pFuncCtx->stMAq.stAddrCfg.uiMotLineOffset = (SIZE_512X(pInit->uiWidth) >> 4);
		SetMemoryAddr(&pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0], &uiVirMemAddr, uiSizeMDMap);
		memset((void *)pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0], 0, uiSizeMDMap);
		h26x_cache_clean(pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0], uiSizeMDMap);
		//TBD
		SetMemoryAddr(&pFuncCtx->stMAq.stAddrCfg.uiMotAddr[1], &uiVirMemAddr, uiSizeMDMap);
		memset((void *)pFuncCtx->stMAq.stAddrCfg.uiMotAddr[1], 0, uiSizeMDMap);
		h26x_cache_clean(pFuncCtx->stMAq.stAddrCfg.uiMotAddr[1], uiSizeMDMap);
		SetMemoryAddr(&pFuncCtx->stMAq.stAddrCfg.uiMotAddr[2], &uiVirMemAddr, uiSizeMDMap);
		memset((void *)pFuncCtx->stMAq.stAddrCfg.uiMotAddr[2], 0, uiSizeMDMap);
		h26x_cache_clean(pFuncCtx->stMAq.stAddrCfg.uiMotAddr[2], uiSizeMDMap);
            #if H26X_USE_DIFF_MAQ
            pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx = 0;
            pFuncCtx->stMAq.stMAQDiffInfo.uiMotFrmCnt = 0;
            {
                int i;
                for (i = 0; i < H26X_MOTION_BITMAP_NUM+1; i++) {
                    SetMemoryAddr(&pFuncCtx->stMAq.stMAQDiffInfo.uiHistMotAddr[i], &uiVirMemAddr, uiSizeMDMap);
                }
            }
            #endif

			SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr0, &uiVirMemAddr, uiSizeOsgGcac);
			memset((void *)pFuncCtx->stOsg.uiGcacStatAddr0, 0, uiSizeOsgGcac);
			h26x_cache_clean(pFuncCtx->stOsg.uiGcacStatAddr0, uiSizeOsgGcac);
			SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr1, &uiVirMemAddr, uiSizeOsgGcac);
			memset((void *)pFuncCtx->stOsg.uiGcacStatAddr1, 0, uiSizeOsgGcac);
			h26x_cache_clean(pFuncCtx->stOsg.uiGcacStatAddr1, uiSizeOsgGcac);
		}

		pVar->uiCtxSize = uiVirMemAddr - pInit->uiEncBufAddr;
	}
	SetMemoryAddr(&pAddr->uiSPFrmBsOutAddr, &uiVirMemAddr, special_p_bs_size);
	SetMemoryAddr(&pAddr->uiSIFrmNaluAddr, &uiVirMemAddr, special_i_nalu_len);

#if H26X_MEM_USAGE
	{
		g_mem_usage[1][pVar->uiEncId].cxt_size = pVar->uiCtxSize;

		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_VDO_CTX].st_adr = (UINT32)pVar->pVdoCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_VDO_CTX].size = uiSizeVdoCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FUNC_CTX].st_adr = (UINT32)pVar->pFuncCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FUNC_CTX].size = uiSizeFuncCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_COMN_CTX].st_adr = (UINT32)pVar->pComnCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_COMN_CTX].size = uiSizeComnCtx;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST0].st_adr = pAddr->uiRecRefY[FRM_IDX_ST_0];
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST0].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_0];
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].size = uiSizeSideInfo;
		if (pAddr->uiRecRefY[FRM_IDX_ST_1]) {
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST1].st_adr = pAddr->uiRecRefY[FRM_IDX_ST_1];
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST1].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_1];;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].size = uiSizeSideInfo;
		}
		if (pAddr->uiRecRefY[FRM_IDX_LT_0]) {
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_LT].st_adr = pAddr->uiRecRefY[FRM_IDX_LT_0];
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_LT].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].st_adr = pAddr->uiSideInfo[0][FRM_IDX_LT_0];;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].size = uiSizeSideInfo;
		}
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV].st_adr = pAddr->uiColMvs[0];
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV].size = uiSizeColMVs;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_BSDMA].st_adr = pAddr->uiBsdma;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_BSDMA].size = uiSizeBSDMA;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_NALU_LEN].st_adr = pAddr->uiNaluLen;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_NALU_LEN].size = uiSizeNaluLen;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SEQ_HDR].st_adr = pAddr->uiSeqHdr;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_SEQ_HDR].size = uiSeqHdrSize;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_PIC_HDR].st_adr = pAddr->uiPicHdr;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_PIC_HDR].size = uiPicHdrSize;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_APB].st_adr = pAddr->uiAPBAddr;
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_APB].size = uiSizeAPB;
		if (uiSizeLLC) {
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_LL].st_adr = pAddr->uiLLCAddr;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_LL].size = uiSizeLLC;
		}

		#if RRC_BY_FRAME_LEVEL
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].st_adr = pAddr->uiRRC[0][0];
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*2*MAX_RRC_FRAME_LEVEL;
		#else
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].st_adr = pAddr->uiRRC[0];
		g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*4;
		#endif
		{
			H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_USR_QP].st_adr = pFuncCtx->stUsrQp.uiQpMapAddr;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_USR_QP].size = uiSizeUsrQpMap;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_MD].st_adr = pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0];
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_MD].size = uiSizeMDMap*3;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_HIST].st_adr = pFuncCtx->stMAq.stAddrCfg.uiHistMotAddr[0];
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_HIST].size = uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+1);
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_GCAC].st_adr = pFuncCtx->stOsg.uiGcacStatAddr0;
			g_mem_usage[1][pVar->uiEncId].buf_cmd[H26X_MEM_GCAC].size = uiSizeOsgGcac*2;
		}
	}
#endif

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
	H26XCOMN_CTX *pComnCtx;

	/* dump param settings */
	_h264Enc_initEncoderDump(pInit);

    /* check horizontal extension */
    if(pInit->uiWidth % 16) {
        DBG_ERR("[H264Enc] encode width %d should be 16X !\r\n ", (int)(pInit->uiWidth));

        return H26XENC_FAIL;
    }

    if (h264Enc_initInternalMemory(pInit, pVar) == H26XENC_FAIL) {
    	DBG_ERR("Error : h264Enc_initInternalMemory fail. \r\n");

		return H26XENC_FAIL;
    }

	h264Enc_initCfg(pInit, pVar);

	h264Enc_encSeqHeader(pVar->pVdoCtx, pVar->pComnCtx);

	pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	h264Enc_wrapSeqCfg((H264ENC_CTX *)pVar->pVdoCtx, (H26XEncAddr *)&pComnCtx->stVaAddr);

	// init fro //
	{
		H264EncFroCfg sFroAttr = {
			1,
			{{243, 250, 255, 243}, {140, 210, 255, 210}, {80, 240, 511, 150}}, // dc
			{{219, 238, 227, 219}, {60, 85, 85, 85}, {60, 85, 255, 75}}, // ac
			{{24, 12, 28, 24}, {0, 0, 0, 0}, {12, 6, 63, 0}}, // st
			{{2, 6, 2, 2}, {0, 0, 0, 0}, {3, 7, 3, 0}} // mx
		};
		#if H264_FRO_IP_SYNC
		if (gH264FroSync) {
			memcpy(sFroAttr.uiDC[0], sFroAttr.uiDC[1], sizeof(UINT32)*4);
			memcpy(sFroAttr.ucAC[0], sFroAttr.ucAC[1], sizeof(UINT8)*4);
			memcpy(sFroAttr.ucST[0], sFroAttr.ucST[1], sizeof(UINT8)*4);
			memcpy(sFroAttr.ucMX[0], sFroAttr.ucMX[1], sizeof(UINT8)*4);
			if (2 == gH264FroSync) {
				sFroAttr.uiDC[2][0] = sFroAttr.uiDC[1][0];
				sFroAttr.uiDC[2][2] = sFroAttr.uiDC[1][1];
				sFroAttr.uiDC[2][1] = sFroAttr.uiDC[1][2];
				sFroAttr.uiDC[2][3] = sFroAttr.uiDC[1][3];

				sFroAttr.ucAC[2][0] = sFroAttr.ucAC[1][0];
				sFroAttr.ucAC[2][2] = sFroAttr.ucAC[1][1];
				sFroAttr.ucAC[2][1] = sFroAttr.ucAC[1][2];
				sFroAttr.ucAC[2][3] = sFroAttr.ucAC[1][3];

				sFroAttr.ucST[2][0] = sFroAttr.ucST[1][0];
				sFroAttr.ucST[2][2] = sFroAttr.ucST[1][1];
				sFroAttr.ucST[2][1] = sFroAttr.ucST[1][2];
				sFroAttr.ucST[2][3] = sFroAttr.ucST[1][3];

				sFroAttr.ucMX[2][0] = sFroAttr.ucMX[1][0];
				sFroAttr.ucMX[2][2] = sFroAttr.ucMX[1][1];
				sFroAttr.ucMX[2][1] = sFroAttr.ucMX[1][2];
				sFroAttr.ucMX[2][3] = sFroAttr.ucMX[1][3];
			}
		}
		#endif
		h264Enc_setFroCfg(pVar, &sFroAttr);
	}

	// init rdo //
	{
		H264EncRdo sRdoAttr = {
			1,
			{1,
			8, 8, 8,
			8, 8,
			8,
			5, 5},
			//{{5.0, 5.0, 5.0, 0.0}, {5.0, 5.0, 5.0, 5.0}, {5.0, 5.0, 0.0, 5.0}},
			{{10, 10, 10, 0}, {10, 10, 10, 10}, {10, 10, 0, 10}},
			8, 8, 8,
			1, 1, 1, 0,
			0, 0, 0, 0,
		};
		h264Enc_InitRdoCfg(pVar, &sRdoAttr);
	}

	// init sde //
	h26XEnc_setInitSde((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr);

	// init rrc //
	{
		H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

		pFuncCtx->stRowRc.ucMinCostTh = 9;
		if (h26x_getChipId() == 98528) {
			pFuncCtx->stRowRc.bRRCMode = 0;
			pFuncCtx->stRowRc.ucTileQPStep[0] = 0;
			pFuncCtx->stRowRc.ucTileQPStep[1] = 0;
			pFuncCtx->stRowRc.bTileQPRst = 0;
			pFuncCtx->stRowRc.ucZeroBitMod = 4;
		}
		pFuncCtx->stRowRc.ucNDQPStep = (UINT8)gRRCNDQPStep;
		pFuncCtx->stRowRc.ucNDQPRange = (UINT8)gRRCNDQPRange;
	}

	// enable psnr //
	H26XEnc_setPSNRCfg(pVar, TRUE);

	// init isp ratio //
	{
		H26XEncIspRatioCfg sIspRatioCfg={100, 100, 100, 100};

		h26XEnc_SetIspRatio(pVar, &sIspRatioCfg);
	}

	// init rrc
	{
		H26XEncRowRcCfg sRrcCfg = {1,
			{0, 4},
			0,
			{2, 4},
			{1, 1},
			{1, 1},
			{51, 51},
			0};

		h26XEnc_setRowRcCfg(pVar, &sRrcCfg);
	}

	// init aq
	{
		H26XEncAqCfg sAqCfg = {1,
			0,
			3, 3,
			8, -8,
			36, 2, 2, 2,
			{-120,-112,-104,-96,-88,-80,-72,-64,-56,-48,-40,-32,-24,-16,-8,7,15,23,31,39,47,55,63,71,79,87,95,103,111,119},
			0,
			3, 3,
			1, 1
		};

		h26XEnc_setAqCfg(pVar, &sAqCfg);
	}

	// init jnd
	{
		H26XEncJndCfg sJndCfg = {0,
			10, 12, 25,
			10,
			0,
			10,
			10
		};

		h26XEnc_setJndCfg(pVar, &sJndCfg);
	}

	if (h26x_getChipId() == 98528) {
		// init intra rmd cost tweak
		{
			H26XEncRmdCfg sRmdCfg = {3, 6, 6, 4, 6, 12};

			h26XEnc_setRmdCfg(pVar, &sRmdCfg);
		}

		// init bgr
		{
			H26XEncBgrCfg sBgrCfg = {0,
				2,
				{65535, 65535},
				{51, 51},
				{43, 43},
				{4, 4},
				{0, 0},
				{65535, 65535}};

			h26XEnc_setBgrCfg(pVar, &sBgrCfg);
		}
	}

	return H26XENC_SUCCESS;
}

void _h264Enc_prepareOnePictureDump(H264ENC_INFO *pInfo)
{
	DBG_MSG("[KDRV_H264ENC] PicType = %d, Src = (0x%08x, 0x%08x, %u, %u), SrcCIv = %u, SrcOut = (%u, %u, 0x%08x, 0x%08x, %u, %u), SrcD2D =(%d, %d), BsOut = (0x%08x, %u), NalLenAddr = 0x%08x, timestamp = %u, TemId = %u, SkipFrmEn = %u, SeqHdrEn = %u, SdeCfg = (%u, %u, %u, %u, %u)\r\n",
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
		(int)pInfo->bGetSeqHdrEn,
		(int)pInfo->SdeCfg.bEnable,
		(int)pInfo->SdeCfg.uiWidth,
		(int)pInfo->SdeCfg.uiHeight,
		(int)pInfo->SdeCfg.uiYLofst,
		(int)pInfo->SdeCfg.uiCLofst);
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
	H264ENC_CTX  *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg  = &pVdoCtx->stPicCfg;
	BOOL bRecFBCEn = ((pInfo->bSrcOutEn == 1) && (pInfo->uiSrcOutYAddr == 0 || pInfo->uiSrcOutCAddr == 0)) ? FALSE : pSeqCfg->bFBCEn;
	// TODO //
	if (pInfo->bSrcD2DEn) {
		if (pInfo->ucSrcD2DMode > 1) {
			DBG_ERR("SrcD2DMode(%d) cannot shall be 0 ~ 1\r\n", pInfo->ucSrcD2DMode);
			return H26XENC_FAIL;
		}
	}
	pInfo->uiBsOutBufSize = ((pInfo->uiBsOutBufSize>>7)<<7);

	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		//for special i frame
		pComnCtx->stVaAddr.bs_buf_size = pInfo->uiBsOutBufSize;
	}
	/* dump param settings */
	_h264Enc_prepareOnePictureDump(pInfo);

	// check info data correct not ready //

	// set data to picture config //
	pPicCfg->ePicType = pInfo->ePicType;
	pComnCtx->stVaAddr.uiBsOutAddr = pInfo->uiBsOutBufAddr;

	if (pComnCtx->ucRcMode == 0)
		pPicCfg->ucSliceQP = IS_ISLICE(pInfo->ePicType) ? pSeqCfg->ucInitIQP : pSeqCfg->ucInitPQP;
	else {
		if (0 == pInfo->bSkipFrmEn || IS_ISLICE(pInfo->ePicType)) {
			pPicCfg->ucSliceQP = h26XEnc_getRcQuant(pVar, pPicCfg->ePicType, ((pSeqCfg->uiLTRInterval > 1) && (pPicCfg->sPoc % pSeqCfg->uiLTRInterval) == 0), pInfo->uiBsOutBufSize);
			if (pPicCfg->ucSliceQP > 51)
				pPicCfg->ucSliceQP = IS_ISLICE(pInfo->ePicType) ? pSeqCfg->ucInitIQP : pSeqCfg->ucInitPQP;
		} else {
			pFuncCtx->stRowRc.uiPredBits = 0;
#if RRC_BY_FRAME_LEVEL
			//pFuncCtx->stRowRc.ucFrameLevel = RC_P3_FRAME_LEVEL;
#endif
			DBG_IND("skip frame qp %d\n", pPicCfg->ucSliceQP);
		}
	}
#if H26X_AUTO_RND
	H26XEnc_setAutoRnd(pVar, pPicCfg->ucSliceQP);
#endif
	if (pFuncCtx->stGdr.stCfg.bEnable) {
		if (pFuncCtx->stGdr.stCfg.bForceGDRQpEn == 0) {
			if (IS_ISLICE(pInfo->ePicType)) {
				pFuncCtx->stGdr.stCfg.ucGDRQp = pPicCfg->ucSliceQP;
			}
		}
	}

	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		if (pPicCfg->ePicType == I_SLICE) {
			pFuncCtx->stSliceSplit.stCfg.bEnable = 1;
			pFuncCtx->stSliceSplit.stCfg.uiSliceRowNum = ((pVar->eCodecType==VCODEC_H264)? 9: 3);
		} else {
			pFuncCtx->stSliceSplit.stCfg.bEnable = 0;
			pFuncCtx->stSliceSplit.uiNaluNum = 1;
			pFuncCtx->stSliceSplit.stExe.uiSliceRowNum = 0;
		}
	}
	if (pFuncCtx->stSliceSplit.stCfg.bEnable == 1) {
		h26XEnc_setSliceSplitCfg(pVar, &pFuncCtx->stSliceSplit.stCfg);
	}
	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		//pPicCfg->ePicType = P_SLICE;
		//special p frame
		h264Enc_encSliceHeader(pVdoCtx, pFuncCtx, pComnCtx, 0, (pPicCfg->ePicType == I_SLICE));
		//if ((pComnCtx->uiPicCnt % pVdoCtx->stSeqCfg.uiGopNum) == 0) {
		//	pPicCfg->ePicType = I_SLICE;
		//}
	} else {
		h264Enc_encSliceHeader(pVdoCtx, pFuncCtx, pComnCtx, pInfo->bGetSeqHdrEn, 0);
	}
	h264Enc_modifyRefFrm(pVdoCtx, pComnCtx->uiPicCnt, bRecFBCEn);
	h264Enc_updateRowRcCfg(pPicCfg->ePicType, &pFuncCtx->stRowRc);

	h264Enc_wrapPicCfg(pRegSet, pInfo, pVar);
	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		if (pPicCfg->ePicType == I_SLICE) {
			h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr, pSeqCfg->usMbHeight, pComnCtx->uiPicCnt, 1);
		} else {
			h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr, pSeqCfg->usMbHeight, pComnCtx->uiPicCnt, 0);
		}
	} else {
		h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr, pSeqCfg->usMbHeight, pComnCtx->uiPicCnt, IS_ISLICE(pPicCfg->ePicType));
	}
	h26xEnc_wrapRowRcPicUpdate(pRegSet, &pFuncCtx->stRowRc, pComnCtx->uiPicCnt, IS_PSLICE(pPicCfg->ePicType), pPicCfg->ucSliceQP, pVdoCtx->uiSvcLable, pComnCtx);
	h264Enc_wrapRdoCfgUpdate(pRegSet, &pVdoCtx->stRdo, IS_ISLICE(pPicCfg->ePicType));
#if H26X_KEYP_DISABLE_TNR
	if (h26x_getChipId() == 98528) {
		if (IS_ISLICE(pInfo->ePicType) || (pSeqCfg->uiLTRInterval && (pPicCfg->sPoc % pSeqCfg->uiLTRInterval) == 0))
			h26xEnc_wrapTnrUpdate(pRegSet, &pFuncCtx->stTnr, TRUE);
		else
			h26xEnc_wrapTnrUpdate(pRegSet, &pFuncCtx->stTnr, FALSE);
	}
#endif
#if JND_DEFAULT_ENABLE
	h26XEnc_setJndCfg(pVar, &gEncJNDCfg);
#endif

	if (H26X_ENC_MODE == 0) {
		h26x_setLLCmd(pVar->uiEncId, pComnCtx->stVaAddr.uiAPBAddr, pComnCtx->stVaAddr.uiLLCAddr, 0, h26x_getHwRegSize() + 64);
	}
	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		if (pPicCfg->ePicType == I_SLICE) {
			h264Enc_wrapSPFrmCfg((H264ENC_CTX *)pVar->pVdoCtx, (H26XEncAddr *)&pComnCtx->stVaAddr);
		} else if ((pPicCfg->sPoc % pSeqCfg->uiGopNum) == 1) { //first p in gop
			h264Enc_wrapFirstPFrmCfg((H264ENC_CTX *)pVar->pVdoCtx, (H26XEncAddr *)&pComnCtx->stVaAddr, pInfo->uiBsOutBufSize);
		}
	}

	h26x_setChkSumEn(pVdoCtx->bSEIPayloadBsChksum==1);

    return H26XENC_SUCCESS;
}

INT32 h264Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt, UINT32 retri, H264ENC_INFO *pInfo, UINT32 bsout_buf_addr_2)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	//fix cim
	if (pVdoCtx->stSeqCfg.uiTotalMBs == 0 || pVdoCtx->stSeqCfg.uiGopNum == 0) {
		DBG_ERR("wrong total MB/GOP number:%d,%d", (int)pVdoCtx->stSeqCfg.uiTotalMBs, (int)pVdoCtx->stSeqCfg.uiGopNum);
		return H26XENC_FAIL;
	} else {
		if (enc_mode) // direct mode //
			h26x_getEncReport(pRegSet->CHK_REPORT);	// shall remove while link-list //
		else
			h26x_cache_invalidate((UINT32)pRegSet->CHK_REPORT, sizeof(UINT32)*H26X_REPORT_NUMBER);

		pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;
		pVdoCtx->uiBsCheckSum = pRegSet->CHK_REPORT[H26X_BS_CHKSUM];

		pResult->uiBSLen = pRegSet->CHK_REPORT[H26X_BS_LEN];
		pResult->uiBSChkSum = pRegSet->CHK_REPORT[H26X_BS_CHKSUM];
		pResult->uiRDOCost[0] = pRegSet->CHK_REPORT[H26X_RRC_RDOPT_COST_LSB];
		pResult->uiRDOCost[1] = pRegSet->CHK_REPORT[H26X_RRC_RDOPT_COST_MSB];
		pResult->uiAvgQP = (pRegSet->CHK_REPORT[H26X_RRC_QP_SUM] + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;

		pResult->uiYPSNR[0] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_Y_LSB];
		pResult->uiYPSNR[1] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_Y_MSB];
		pResult->uiUPSNR[0] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_U_LSB];
		pResult->uiUPSNR[1] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_U_MSB];
		pResult->uiVPSNR[0] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_V_LSB];
		pResult->uiVPSNR[1] = pRegSet->CHK_REPORT[H26X_PSNR_FRM_V_MSB];

		pResult->uiRecYHitCnt = pRegSet->CHK_REPORT[H26X_CRC_HIT_Y_CNT];
		pResult->uiRecCHitCnt = pRegSet->CHK_REPORT[H26X_CRC_HIT_C_CNT];
		//pResult->uiInterCnt = pRegSet->CHK_REPORT[H26X_STATS_INTER_CNT];
		//pResult->uiSkipCnt = pRegSet->CHK_REPORT[H26X_STATS_SKIP_CNT];
		if (pComnCtx->uiPicCnt)
			pResult->uiMotionRatio = (((pRegSet->CHK_REPORT[H26X_SCD_REPORT]>>16)&0xffff)*100 + (pVdoCtx->stSeqCfg.uiTotalMBs>>1))/pVdoCtx->stSeqCfg.uiTotalMBs;
		else
			pResult->uiMotionRatio = 0;

		pResult->iVPSHdrLen = 0;
		pResult->iSPSHdrLen = pVdoCtx->uiSPSHdrLen;
		pResult->iPPSHdrLen = pVdoCtx->uiPPSHdrLen;

		pResult->uiInterCnt = pRegSet->CHK_REPORT[H26X_STATS_INTER_CNT];
		pResult->uiSkipCnt = pRegSet->CHK_REPORT[H26X_STATS_SKIP_CNT];
		pResult->uiMergeCnt = pRegSet->CHK_REPORT[H26X_STATS_MERGE_CNT];
		pResult->uiIntra4Cnt = pRegSet->CHK_REPORT[H26X_STATS_IRA4_CNT];
		pResult->uiIntra8Cnt = pRegSet->CHK_REPORT[H26X_STATS_IRA8_CNT];
		pResult->uiIntra16Cnt = pRegSet->CHK_REPORT[H26X_STATS_IRA16_CNT];
		pResult->uiIntra32Cnt = pRegSet->CHK_REPORT[H26X_STATS_IRA32_CNT];
		pResult->uiCU64Cnt = pRegSet->CHK_REPORT[H26X_STATS_CU64_CNT];
		pResult->uiCU32Cnt = pRegSet->CHK_REPORT[H26X_STATS_CU32_CNT];
		pResult->uiCU16Cnt = pRegSet->CHK_REPORT[H26X_STATS_CU16_CNT];

		pResult->uiSvcLable = pVdoCtx->uiSvcLable;
		pResult->bRefLT = (pVdoCtx->stSeqCfg.uiLTRInterval && (pVdoCtx->stPicCfg.sRefPoc == 0));

		if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
			if (pVdoCtx->stPicCfg.ePicType == I_SLICE) {
				h26x_cache_invalidate(pComnCtx->stVaAddr.uiSPFrmBsOutAddr, pResult->uiBSLen);
				{
					//H264EncPicCfg *pPicCfg  = &pVdoCtx->stPicCfg;
					H26XEncAddr *pAddr = &pComnCtx->stVaAddr;
					//unsigned int interrupt, si_size;
					unsigned int si_size;
					//int i;
					H26XEncNaluLenResult nalu_len_result = {0};
					unsigned int sp_used_size, sp_dummy_size;

					//save sp used size for picking up si bs
					h26xEnc_getNaluLenResult(pVar, &nalu_len_result);
					sp_dummy_size = *(unsigned int* )nalu_len_result.uiVaAddr;
					sp_used_size = pResult->uiBSLen - sp_dummy_size;

					//prepare si and trigger
					h264Enc_prepareSpecialIFrame(pVar, pAddr->bs_buf_size);
					h26x_setEncDirectRegSet(h26xEnc_getVaAPBAddr(pVar));
					h26x_start();

					h26x_waitINT();
					//interrupt = h26x_waitINT();

					h26x_getEncReport(pRegSet->CHK_REPORT);
					si_size = pRegSet->CHK_REPORT[H26X_BS_LEN];
					h26x_cache_invalidate(pComnCtx->stVaAddr.uiBsOutAddr, si_size);

					//pick up si and sp bs
					memcpy((void*)(pAddr->uiBsOutAddr + si_size), (void*)(pAddr->uiSPFrmBsOutAddr + sp_dummy_size), sp_used_size);
					pResult->uiBSLen = si_size + sp_used_size;
					h26x_cache_clean(pComnCtx->stVaAddr.uiBsOutAddr, pResult->uiBSLen);

					//update nalu buf
					*(unsigned int* )pComnCtx->stVaAddr.uiNaluLen = si_size;
					h26x_cache_clean(pComnCtx->stVaAddr.uiNaluLen, 4);
				}
			} else {
				h26x_cache_invalidate(pComnCtx->stVaAddr.uiBsOutAddr, pResult->uiBSLen);
			}
		} else {
			if (retri) {
				// flush bsout buffer separately
				h26x_cache_invalidate(pInfo->uiBsOutBufAddr, pInfo->uiBsOutBufSize);
				h26x_cache_invalidate(bsout_buf_addr_2, pResult->uiBSLen-pInfo->uiBsOutBufSize);
			} else {
				h26x_cache_invalidate(pComnCtx->stVaAddr.uiBsOutAddr, pResult->uiBSLen);
			}
		}

		if (pFuncCtx->stRowRc.uiPredBits)
			h26XEnc_getRowRcState(pVdoCtx->stPicCfg.ePicType, pRegSet, &pFuncCtx->stRowRc, pResult->uiSvcLable);
		if (pComnCtx->ucRcMode != 0) {
			if (0 == get_from_reg(pRegSet->FUNC_CFG[0], 6, 1)) {
				h26XEnc_UpdateRc(pVar, pVdoCtx->stPicCfg.ePicType, pResult);
			} else {
				DBG_IND("skip frame\n");
			}
		}
		h264Enc_updatePicCfg(pVdoCtx);

		pComnCtx->uiPicCnt++;
		pComnCtx->uiLastHwInt = uiHwInt;

		pResult->ucNxtPicType = (pVdoCtx->stPicCfg.sPoc % pVdoCtx->stSeqCfg.uiGopNum) ? P_SLICE : ((pFuncCtx->stGdr.stCfg.bGDRIFrm == 1)? I_SLICE: IDR_SLICE);
		pResult->ucQP = pVdoCtx->stPicCfg.ucSliceQP;
		pResult->ucPicType = (IS_PSLICE(pVdoCtx->stPicCfg.ePicType) && (pVdoCtx->stPicCfg.sRefPoc == 0)) ? 4 : pVdoCtx->stPicCfg.ePicType;
		pResult->uiHwEncTime = pRegSet->CHK_REPORT[H26X_CYCLE_CNT]/h26x_getClk();

		pVdoCtx->stPicCfg.eNxtPicType = pResult->ucNxtPicType;

		return H26XENC_SUCCESS;
	}
}

INT32 h264Enc_InitRdoCfg(H26XENC_VAR *pVar, H264EncRdo *pRdo)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H264EncRdo *pRdoCfg = &pVdoCtx->stRdo;

	// check info data correct not ready //

	memcpy(pRdoCfg, pRdo, sizeof(H264EncRdo));

	h264Enc_wrapRdoCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);

	return H26XENC_SUCCESS;
}

INT32 h264Enc_setRdoCfg(H26XENC_VAR *pVar, H264EncRdoCfg *pRdo)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncRdoCfg *pRdoCfg = &pVdoCtx->stRdo.stRdoCfg;

	// check info data correct not ready //

	memcpy(pRdoCfg, pRdo, sizeof(H264EncRdoCfg));

	return H26XENC_SUCCESS;
}

INT32 h264Enc_setFroCfg(H26XENC_VAR *pVar, H264EncFroCfg *pFro)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H264EncFroCfg *pFroCfg = &pVdoCtx->stFroCfg;

	// check info data correct not ready //

	memcpy(pFroCfg, pFro, sizeof(H264EncFroCfg));

	h264Enc_wrapFroCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);

	return H26XENC_SUCCESS;
}

void h264Enc_setGopNum(H26XENC_VAR *pVar, UINT32 gop)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;

	pVdoCtx->stSeqCfg.uiGopNum = gop;
}

UINT32 h264Enc_queryMemSize(H26XEncMeminfo *pInfo)
{
	UINT32 uiFrmBufNum    = 1 + (pInfo->ucSVCLayer == 2) + (pInfo->uiLTRInterval != 0);		// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = SIZE_64X(pInfo->uiWidth)*SIZE_16X(pInfo->uiHeight);
	UINT32 uiSizeSideInfo = ((((pInfo->uiWidth+63)/64)*4+31)/32)*32*((pInfo->uiHeight+15)/16);
	UINT32 uiSizeColMVs   = (pInfo->bColMvEn==0)? 0: ((pInfo->uiWidth+63)/64)*((pInfo->uiHeight+15)/16)*64;
	UINT32 uiSizeRcRef    = (((((pInfo->uiHeight+15)/16)+3)/4)*4)*16;
	UINT32 uiSizeBSDMA    = (((pInfo->uiHeight+15)/16)*2+1)*4;
	UINT32 uiSizeUsrQpMap = sizeof(UINT16)*(((SIZE_32X(pInfo->uiWidth)+15)/16)*((pInfo->uiHeight+15)/16));//hk
	UINT32 uiSizeMDMap    = ((SIZE_512X(pInfo->uiWidth) >> 4) * (SIZE_16X(pInfo->uiHeight) >> 4) >> 3);
	UINT32 uiSizeNaluLen  = SIZE_256X(((pInfo->uiHeight+15)/16)*4) + 16;
	UINT32 uiSeqHdrSize   = 256;
	UINT32 uiPicHdrSize   = (((pInfo->uiHeight+15)/16)*20) + uiSeqHdrSize + 128;	// each slice header + sequence header + sei(128) //
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = (H26X_ENC_MODE == 0) ? 0 : h26x_getHwRegSize() + 64;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H264ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));
	UINT32 slice_num = 0;
	UINT32 special_p_size = ((pInfo->bGDRIFrm == 1)? uiFrmSize: 0);
	UINT32 special_i_nalu_len = ((pInfo->bGDRIFrm == 1)? SIZE_256X(((pInfo->uiHeight+15)/16)*4) + 16: 0);

	slice_num = ((pInfo->uiHeight+15)/16);
	if(slice_num % 64 == 0){
		//ic limitaion: 64X need more 256 byte
		uiSizeNaluLen += 256;
	}

#if H26X_USE_DIFF_MAQ
	if (pInfo->bCommReconFrmBuf) {
	    uiTotalBufSize = (uiSizeSideInfo *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
	        uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;        // 256 for 16x alignement //
	} else {
	    uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
	        uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;        // 256 for 16x alignement //
	}
#else
	if (pInfo->bCommReconFrmBuf) {
		uiTotalBufSize = (uiSizeSideInfo *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*3 +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignement //
	} else {
		uiTotalBufSize = ((uiFrmSize*3/2 + uiSizeSideInfo) *uiFrmBufNum) + uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2) + uiSizeBSDMA + uiSizeColMVs + uiSizeUsrQpMap + uiSizeLLC + uiSizeMDMap*3 +
			uiSizeNaluLen + uiSeqHdrSize + uiPicHdrSize + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;		// 256 for 16x alignement //
	}
#endif
	uiTotalBufSize += (special_p_size + special_i_nalu_len);

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

void h264Enc_getRdoCfg(H26XENC_VAR *pVar, H264EncRdoCfg *pRdo)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncRdoCfg *pRdoCfg = &pVdoCtx->stRdo.stRdoCfg;

	// check info data correct not ready //

	memcpy(pRdo, pRdoCfg, sizeof(H264EncRdoCfg));
}

UINT32 h264Enc_queryRecFrmSize(const H26XEncMeminfo *pInfo)
{
	UINT32 uiFrmSize = SIZE_64X(pInfo->uiWidth)*SIZE_16X(pInfo->uiHeight);
	return (uiFrmSize*3/2);
}

#if H26X_SET_PROC_PARAM
int h264Enc_getRowRCStopFactor(void)
{
    return gH264RowRCStopFactor;
}
int h264Enc_setRowRCStopFactor(int value)
{
    if (value > 0)
        gH264RowRCStopFactor = value;
    return 0;
}

int h264Enc_getPReduce16Planar(void)
{
    return gH264PReduce16Planar;
}
int h264Enc_setPReduce16Planar(int value)
{
    if (0 == value || 1 == value)
        gH264PReduce16Planar = value;
    return 0;
}

int h264Enc_getMaxDecFrameBuf(void)
{
	return gH264MaxDecFrameBuf;
}
int h264Enc_setMaxDecFrameBuf(int value)
{
	if (value >= 0 && value <= 16)
		gH264MaxDecFrameBuf = value;
	return 0;
}
#endif
#if H264_FRO_IP_SYNC
int h264Enc_setFroSync(int value)
{
	if (value >= 0 && value <= 2)
		gH264FroSync = value;
	return 0;
}
int h264Enc_getFroSync(void)
{
	return gH264FroSync;
}
#endif

int h264Enc_setFrameNumGapAllow(int value)
{
    gH264FrameNumGapAllow = value;
    return 0;
}

int h264Enc_setFixSPSLog2Poc(int value)
{
    gFixSPSLog2Poc = value;
    return 0;
}

UINT32 h264Enc_getEncodeRatio(H26XENC_VAR *pVar)
{
	H264ENC_CTX  *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	UINT32 result = h26x_getDbg1(0);
	UINT32 mb_x, mb_y;
	UINT32 enc_ratio;

	if (pSeqCfg->uiTotalMBs == 0) {
		DBG_ERR("wrong Lcu numbers:%d\r\n", (int)pSeqCfg->uiTotalMBs);
		return 0;
	} else {
		mb_y = (result&0x00ff0000)>>16;
		mb_x = (result&0xff000000)>>24;
		enc_ratio = (mb_y * pSeqCfg->usMbWidth+ mb_x)*100 / pSeqCfg->uiTotalMBs;
		if (0 == enc_ratio)
			enc_ratio = 1;
		return enc_ratio;
	}
}

void h264Enc_prepareSpecialIFrame(H26XENC_VAR *pVar, UINT32 bs_buf_size)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	//H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	//unsigned int* nalu_len;

	//nalu_len = (unsigned int* )nalu_len_result.uiVaAddr;
//printk("nalu:x%x\r\n", nalu_len_result.uiSliceNum);
	//for (i = 0; i < nalu_len_result.uiSliceNum; i++) {
	//	printk("nalu, idx:%d, adr:x%x, s:x%x\r\n", i, (int)(nalu_len+i), nalu_len[i]);
	//}

	//pPicCfg->ePicType= I_SLICE;
	// for bsdma
	pFuncCtx->stSliceSplit.uiNaluNum = 1;
	pFuncCtx->stSliceSplit.stExe.uiSliceRowNum = 0;
	pFuncCtx->stSliceSplit.stCfg.bEnable = 0;
	h264Enc_encSeqHeader(pVar->pVdoCtx, pVar->pComnCtx);
	h264Enc_encSliceHeader(pVdoCtx, pFuncCtx, pComnCtx, 1, 0);
	h264Enc_wrapSIFrmCfg((H264ENC_CTX *)pVar->pVdoCtx, (H26XEncAddr *)&pComnCtx->stVaAddr, bs_buf_size);
	// for kflow
	pFuncCtx->stSliceSplit.uiNaluNum = CEILING_DIV(CEILING_DIV(pComnCtx->uiHeight, 16), H264E_GDRI_MIN_SLICE_HEIGHT/16);
}
// for emualtion only //
void h264Enc_setPicQP(UINT8 ucQP, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	pPicCfg->ucSliceQP = ucQP;
}

int h264Enc_setSEIBsChksumen(H26XENC_VAR *pVar, UINT32 enable)
{
	H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;

	pVdoCtx->bSEIPayloadBsChksum = enable;

	return 0;
}

