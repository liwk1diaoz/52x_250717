/*
    H265ENC module driver for NT96660

    Interface for Init and Info

    @file       h265enc_api.c
    @ingroup    mICodecH265

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_int.h"
#include "h26xenc_wrap.h"

#include "h265enc_api.h"
#include "h265enc_int.h"
#include "h265enc_wrap.h"
#include "h265enc_header.h"
#include "h26x_bitstream.h"

#if USE_JND_DEFAULT_VALUE
    extern H26XEncJndCfg gEncJNDCfg;
    extern int gUseDefaultJNDCfg;
#endif

static INT32 set_tile_config(H265EncTileCfg *pTileCfg, BOOL bTile, UINT32 frm_width, BOOL bD2d, BOOL bGdc, UINT32 hw_version)
{
	DBG_IND("[KDRV_H265ENC] bTile = %u, bD2d = %u, bGdc = %u, frm_width = %u\r\n", bTile, bD2d, bGdc, frm_width);
    memset(pTileCfg, 0, sizeof(H265EncTileCfg));
    pTileCfg->bTileEn = bTile;

    if (bD2d==1) {
        if (bTile) {
            if (bGdc == 1) {
                if (frm_width < H265_GDC_MIN_W_1_TILE) {
                    DBG_ERR("Error: tile width is under boundary when enable gdc mode.(w:%d, gdc_min_w:%d)\r\n", frm_width, H265_GDC_MIN_W_1_TILE);
                    return H26XENC_FAIL;
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28)) {// 2816
                    pTileCfg->ucTileNum = 2;
                    pTileCfg->uiTileWidth[0] = SIZE_128X(frm_width/2);
                    pTileCfg->uiTileWidth[1] = frm_width - pTileCfg->uiTileWidth[0];
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28)) {// 4096
                    pTileCfg->ucTileNum = 3;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR28*3)) {// 3840
                        pTileCfg->uiTileWidth[0] = pTileCfg->uiTileWidth[1] = SIZE_128X(frm_width / 3);
                        pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0]*2;
                    } else {
                        pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR28;
                        pTileCfg->uiTileWidth[0] = SIZE_128X((frm_width-pTileCfg->uiTileWidth[1])/2);
                        pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1];
                    }
                } else {
                    DBG_ERR("Error: not support width(%d) when enable gdc mode(max_w:%d)\r\n",frm_width,(H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28));
                    return H26XENC_FAIL;
                }
            } else {
                if (frm_width <= H265_D2D_MIN_W_1_TILE) {
                    DBG_ERR("Error: tile width is under boundary when enable d2d mode.(w:%d, d2d_min_w:%d)\r\n", frm_width, H265_D2D_MIN_W_1_TILE);
                    return H26XENC_FAIL;
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28)) {// 2816
                    pTileCfg->ucTileNum = 2;
                    pTileCfg->uiTileWidth[0] = H26X_FIRST_TILE_MAX_W_SR28;
                    pTileCfg->uiTileWidth[1] = frm_width - pTileCfg->uiTileWidth[0];
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28)) {// 4096
                    pTileCfg->ucTileNum = 3;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR28*3)) {// 3840
                        pTileCfg->uiTileWidth[0] = pTileCfg->uiTileWidth[1] = SIZE_128X(frm_width / 3);
                        pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0]*2;
                    } else {
                        pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR28;
                        pTileCfg->uiTileWidth[0] = SIZE_128X((frm_width-pTileCfg->uiTileWidth[1])/2);
                        pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1];
                    }
                } else {
                    DBG_ERR("Error: not support width(%d) when enable d2d mode(max_w:%d)\r\n", frm_width,(H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28+H26X_LAST_TILE_MAX_W_SR28));
                    return H26XENC_FAIL;
                }
            }
        } else {
            if (frm_width <= H265_D2D_MIN_W_1_TILE) {
                pTileCfg->ucTileNum = 0;
            } else {
                DBG_ERR("Error: frame width(%d) is over boundary when eable d2d mode(max_w:%d)\r\n", frm_width, H265_D2D_MIN_W_1_TILE);
                return H26XENC_FAIL;
            }
        }
    } else {
        if (bTile) {
            if (frm_width < (H26X_MIN_W_TILE)) {// 1024
                DBG_ERR("Error: tile width(%d) is under boundary(min_w:%d)\r\n", frm_width, (H26X_MIN_W_TILE));
                return H26XENC_FAIL;
            } else if (frm_width <= H26X_SR36_MAX_W) {
                if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR36+H26X_LAST_TILE_MAX_W_SR36)) {// 2048
                    pTileCfg->ucTileNum = 2;
                    pTileCfg->uiTileWidth[0] = ((frm_width >> 1) < H26X_FIRST_TILE_MIN_W)? H26X_FIRST_TILE_MIN_W: SIZE_64X(frm_width >> 1);
                    pTileCfg->uiTileWidth[1] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]);
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR36+H26X_MIDDLE_TILE_MAX_W_SR36+H26X_LAST_TILE_MAX_W_SR36)) { // 2944
                    pTileCfg->ucTileNum = 3;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR36*3)) {// 2688
                        pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width / 3);
                        pTileCfg->uiTileWidth[2] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]*2);
                    } else {
                        pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR36;
                        pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]) >> 1);
                        pTileCfg->uiTileWidth[2] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[0]);
                    }
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR36+H26X_MIDDLE_TILE_MAX_W_SR36*2+H26X_LAST_TILE_MAX_W_SR36)) {// 3840
                    pTileCfg->ucTileNum = 4;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR36*4)) {// 3584
                        pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width >> 2);
                        pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
                    } else {
                        pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR36;
                        pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*2) >> 1);
                        pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
                    }
                } else {
                    pTileCfg->ucTileNum = 5;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR36*5)) {// 4480
                        pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1]
                                                    = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width / 5);
                        pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
                    } else {
                        pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR36;
                        pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*3) >> 1);
                        pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
                    }
                }
            } else {
                if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28*2+H26X_LAST_TILE_MAX_W_SR28)) {// 5376
                    pTileCfg->ucTileNum = 4;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR28*4)) {// 5120
                        pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width >> 2);
                        pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
                    } else {
                        pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR28;
                        pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*2) >> 1);
                        pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
                    }
                } else if (frm_width <= (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28*3+H26X_LAST_TILE_MAX_W_SR28) ) {// 6656
                    pTileCfg->ucTileNum = 5;
                    if (frm_width <= (H26X_MIDDLE_TILE_MAX_W_SR28*5)) {// 6400
                        pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1]
                                                    = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width / 5);
                        pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
                    } else {
                        pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = H26X_MIDDLE_TILE_MAX_W_SR28;
                        pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*3) >> 1);
                        pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
                    }
                } else {
                    DBG_ERR("Error: tile width(%d) is over boundary(max_w:%d)\r\n", frm_width, (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28*3+H26X_LAST_TILE_MAX_W_SR28));
                    return H26XENC_FAIL;
                }
            }
        } else {
            pTileCfg->ucTileNum = 0;
            if (hw_version == HW_VERSION_520) {
                if (frm_width > H26X_MAX_W_WITHOUT_TILE_V20) { // 2048
                    DBG_ERR("Error: frame width(%d) is over boundary(max_w:%d)\r\n", frm_width, H26X_MAX_W_WITHOUT_TILE_V20);
                    return H26XENC_FAIL;
                }
            } else {
                if (frm_width > H26X_MAX_W_WITHOUT_TILE_V20_528) { // 4096
                    DBG_ERR("Error: frame width(%d) is over boundary(max_w:%d)\r\n", frm_width, H26X_MAX_W_WITHOUT_TILE_V20_528);
                    return H26XENC_FAIL;
                }
            }
        }
    }
	DBG_IND("[KDRV_H265ENC] tile num = %u, width = (%u, %u, %u, %u, %u)\r\n", pTileCfg->ucTileNum, pTileCfg->uiTileWidth[0], pTileCfg->uiTileWidth[1], pTileCfg->uiTileWidth[2], pTileCfg->uiTileWidth[3], pTileCfg->uiTileWidth[4]);
	return H26XENC_SUCCESS;
}

//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H265ENC_SUCCESS: init success
        - @b H265ENC_FAIL: size is too small or address is null
*/
static INT32 h265Enc_initInternalMemory(H265ENC_INIT *pInit, H26XENC_VAR *pVar, H265EncTileCfg *pTileCfg)
{
	H26XFUNC_CTX *pFuncCtx;
	H26XCOMN_CTX *pComnCtx;
	H26XEncAddr  *pAddr;

	// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiFrmBufNum = 1 + (pInit->ucSVCLayer == 2) + (pInit->uiLTRInterval != 0);
	UINT32 uiVirMemAddr   = pInit->uiEncBufAddr;	
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = SIZE_64X(pInit->uiWidth) * SIZE_64X(pInit->uiHeight);
    UINT32 uiSizeSideInfoLof[H26X_MAX_TILE_NUM];
    UINT32 uiSizeSideInfo[H26X_MAX_TILE_NUM];
	UINT32 uiSizeColMVs   = SIZE_16X((((((pInit->uiWidth + 63) >> 6) * ((pInit->uiHeight + 63) >> 6)) << 6) << 2));
    UINT32 uiExtraRecSize = SIZE_64X(pInit->uiHeight) << 7;

    UINT32 uiSizeBSDMA    = ((((pInit->uiHeight + 63)>>6)<<1) + 1 + 4) << 2;
	UINT32 uiSizeUsrQpMap = (((pInit->uiWidth + 63)>>6) * ((pInit->uiHeight + 63)>>6)) * 16 * 2;
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiNaluLength   = (pInit->uiHeight+63) >> 6;
    UINT32 uiSizeRcRef    = SIZE_4X(((pInit->uiHeight + 63) >> 6)) << 4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = h26x_getHwRegSize() + 64;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H265ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));

    UINT8 i;

	uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC +
						H265ENC_HEADER_MAXSIZE*2 + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;// 256 for 16x alignment

    if(pTileCfg->bTileEn) {
        for (i = 0; i < pTileCfg->ucTileNum; i++) {
            if (i < pTileCfg->ucTileNum-1) {
                //(size_64(frm_height))*(128 pxl)*(ycbcr)*(frm buf num)
                uiTotalBufSize += (uiExtraRecSize*3/2*uiFrmBufNum);
            }

            uiSizeSideInfoLof[i] = SIZE_32X(((((pTileCfg->uiTileWidth[i]+63)>>6) << 3)));
            uiSizeSideInfo[i] = uiSizeSideInfoLof[i]*((pInit->uiHeight+63)>>6);

            //sideinfo*(rd/wr)*(frm buf num)
            uiTotalBufSize += (uiSizeSideInfo[i]*2*uiFrmBufNum);
            uiTotalBufSize += uiSizeBSDMA;
            uiTotalBufSize += uiNaluLength;
            //(rrc buf)*(i/p)*(rd/wr)
            uiTotalBufSize += (uiSizeRcRef << 2);
        }
    } else {
        uiSizeSideInfoLof[0] = SIZE_32X((((pInit->uiWidth+63)>>6) << 3));
        uiSizeSideInfo[0] = uiSizeSideInfoLof[0]*((pInit->uiHeight+63)>>6);

        uiTotalBufSize += uiSizeSideInfo[0];
        uiTotalBufSize += uiSizeBSDMA;
        uiTotalBufSize += uiNaluLength;
        uiTotalBufSize += (uiSizeRcRef << 2);
    }

	if (uiTotalBufSize > pInit->uiEncBufSize)
	{
		DBG_ERR("EncBufSize too small: 0x%08x < 0x%08x\r\n", pInit->uiEncBufSize, uiTotalBufSize);

        return H26XENC_FAIL;
	}

	if (pInit->uiEncBufAddr == 0)
	{
		DBG_ERR("EncBufAddr is NULL: 0x%08x\r\n", pInit->uiEncBufAddr);

        return H26XENC_FAIL;
	}

	DBG_IND("uiWidth           : 0x%08x\r\n", pInit->uiWidth);
    DBG_IND("uiHeight          : 0x%08x\r\n", pInit->uiHeight);
    DBG_IND("uiFrmSize         : 0x%08x\r\n", uiFrmSize);
    if (pTileCfg->bTileEn) {
        for (i = 0; i < pTileCfg->ucTileNum; i++)
        	DBG_IND("uiSizeSideInfo[%d]    : 0x%08x\r\n", i, uiSizeSideInfo[i]);
    	DBG_IND("uiExtraRecSize    : 0x%08x\r\n", uiExtraRecSize);
    } else {
    	DBG_IND("uiSizeSideInfo    : 0x%08x\r\n", uiSizeSideInfo[0]);
    }
    DBG_IND("uiSizeColMVs      : 0x%08x\r\n", uiSizeColMVs);
	DBG_IND("uiSizeRcRef       : 0x%08x\r\n", uiSizeRcRef);
	DBG_IND("uiSizeBSDMA       : 0x%08x\r\n", uiSizeBSDMA);
	DBG_IND("uiSizeUsrQpMap    : 0x%08x\r\n", uiSizeUsrQpMap);
	DBG_IND("uiSizeOsgGcac     : 0x%08x\r\n", uiSizeOsgGcac);
	DBG_IND("uiSizeAPB         : 0x%08x\r\n", uiSizeAPB);
    DBG_IND("uiSizeVdoCtx      : 0x%08x\r\n", uiSizeVdoCtx);	
	DBG_IND("uiSizeFuncCtx     : 0x%08x\r\n", uiSizeFuncCtx);	
	DBG_IND("uiSizeComnCtx     : 0x%08x\r\n", uiSizeComnCtx);	

	// virtaul buffer management //
	{
		SetMemoryAddr((UINT32 *)&pVar->pVdoCtx, &uiVirMemAddr, uiSizeVdoCtx);
		memset(pVar->pVdoCtx, 0, uiSizeVdoCtx);
		SetMemoryAddr((UINT32 *)&pVar->pFuncCtx, &uiVirMemAddr, uiSizeFuncCtx);
		memset(pVar->pFuncCtx, 0, uiSizeFuncCtx);
		SetMemoryAddr((UINT32 *)&pVar->pComnCtx, &uiVirMemAddr, uiSizeComnCtx);
		memset(pVar->pComnCtx, 0, uiSizeComnCtx);

		pFuncCtx = pVar->pFuncCtx;
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
	}

	// physical and veritaul buffer mangement //
	pAddr = &pComnCtx->stVaAddr;
	pAddr->uiFrmBufNum = uiFrmBufNum;

	SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize);
    SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize>>1);
    SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_ST_0], &uiVirMemAddr, uiSizeColMVs);

    if(pTileCfg->bTileEn) {
        UINT32 uiTotalSizeSideInfo=0;
        for (i = 0; i < pTileCfg->ucTileNum; i++) {
            if (i < pTileCfg->ucTileNum-1) {
                SetMemoryAddr(&pAddr->uiTileExtraY[i][FRM_IDX_ST_0], &uiVirMemAddr, uiExtraRecSize);
                SetMemoryAddr(&pAddr->uiTileExtraC[i][FRM_IDX_ST_0], &uiVirMemAddr, uiExtraRecSize>>1);
            }
            uiTotalSizeSideInfo += uiSizeSideInfo[i];
        }
        // set rd/wr buffers when enable tile mode
        SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_0], &uiVirMemAddr, uiTotalSizeSideInfo);
        SetMemoryAddr(&pAddr->uiSideInfo[1][FRM_IDX_ST_0], &uiVirMemAddr, uiTotalSizeSideInfo);
    }
    else {
        SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_0], &uiVirMemAddr, uiSizeSideInfo[0]);
    }

	if (pInit->ucSVCLayer == 2) {
		SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize);
        SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize>>1);
        SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_ST_1], &uiVirMemAddr, uiSizeColMVs);

        if(pTileCfg->bTileEn) {
            UINT32 uiTotalSizeSideInfo=0;
            for (i = 0; i < pTileCfg->ucTileNum; i++) {
                if (i < pTileCfg->ucTileNum-1) {
                    SetMemoryAddr(&pAddr->uiTileExtraY[i][FRM_IDX_ST_1], &uiVirMemAddr, uiExtraRecSize);
                    SetMemoryAddr(&pAddr->uiTileExtraC[i][FRM_IDX_ST_1], &uiVirMemAddr, uiExtraRecSize>>1);
                }
                uiTotalSizeSideInfo += uiSizeSideInfo[i];
            }
            // set rd/wr buffers when enable tile mode
            SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiTotalSizeSideInfo);
            SetMemoryAddr(&pAddr->uiSideInfo[1][FRM_IDX_ST_1], &uiVirMemAddr, uiTotalSizeSideInfo);
        } else {
            SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiSizeSideInfo[0]);
        }
	}

	if (pInit->uiLTRInterval != 0) {
		SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize);            
        SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize>>1);
        SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_LT_0], &uiVirMemAddr, uiSizeColMVs);

        if(pTileCfg->bTileEn) {
            UINT32 uiTotalSizeSideInfo=0;
            for (i = 0; i < pTileCfg->ucTileNum; i++) {
                if (i < pTileCfg->ucTileNum-1) {
                    SetMemoryAddr(&pAddr->uiTileExtraY[i][FRM_IDX_LT_0], &uiVirMemAddr, uiExtraRecSize);
                    SetMemoryAddr(&pAddr->uiTileExtraC[i][FRM_IDX_LT_0], &uiVirMemAddr, uiExtraRecSize/2);
                }
                uiTotalSizeSideInfo += uiSizeSideInfo[i];
            }
            // set rd/wr buffers when enable tile mode
            SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_LT_0], &uiVirMemAddr, uiTotalSizeSideInfo);
            SetMemoryAddr(&pAddr->uiSideInfo[1][FRM_IDX_LT_0], &uiVirMemAddr, uiTotalSizeSideInfo);
        } else {
            SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_LT_0], &uiVirMemAddr, uiSizeSideInfo[0]);
        }
	}

	// define non-refernece buffer address //
	pAddr->uiRecRefY[FRM_IDX_NON_REF] = pAddr->uiRecRefY[FRM_IDX_ST_0];
	pAddr->uiRecRefC[FRM_IDX_NON_REF] = pAddr->uiRecRefC[FRM_IDX_ST_0];
	pAddr->uiColMvs[FRM_IDX_NON_REF] = pAddr->uiColMvs[FRM_IDX_ST_0];

	if(pTileCfg->bTileEn) {
        for (i = 0; i < pTileCfg->ucTileNum; i++) {
            if (i < pTileCfg->ucTileNum-1) {
                pAddr->uiTileExtraY[i][FRM_IDX_NON_REF] = pAddr->uiTileExtraY[i][FRM_IDX_ST_0];
                pAddr->uiTileExtraC[i][FRM_IDX_NON_REF] = pAddr->uiTileExtraC[i][FRM_IDX_ST_0];
            }
        }
		pAddr->uiSideInfo[0][FRM_IDX_NON_REF] = pAddr->uiSideInfo[0][FRM_IDX_ST_0];
		pAddr->uiSideInfo[1][FRM_IDX_NON_REF] = pAddr->uiSideInfo[1][FRM_IDX_ST_0];
	}
    else
		pAddr->uiSideInfo[0][FRM_IDX_NON_REF] = pAddr->uiSideInfo[0][FRM_IDX_ST_0];

    SetMemoryAddr(&pAddr->uiAPBAddr, &uiVirMemAddr, uiSizeAPB);
    memset((void *)pAddr->uiAPBAddr, 0, uiSizeAPB);
	SetMemoryAddr(&pAddr->uiLLCAddr, &uiVirMemAddr, uiSizeLLC);	
	memset((void *)pAddr->uiLLCAddr, 0, uiSizeLLC);

    SetMemoryAddr(&pAddr->uiBsdma, &uiVirMemAddr, uiSizeBSDMA);
    SetMemoryAddr(&pAddr->uiNaluLen, &uiVirMemAddr, uiNaluLength);

    SetMemoryAddr(&pAddr->uiRRC[0][0], &uiVirMemAddr,
            uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
    SetMemoryAddr(&pAddr->uiRRC[0][1], &uiVirMemAddr,
            uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
    SetMemoryAddr(&pAddr->uiRRC[1][0], &uiVirMemAddr,
            uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
    SetMemoryAddr(&pAddr->uiRRC[1][1], &uiVirMemAddr,
            uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
    SetMemoryAddr(&pAddr->uiSeqHdr, &uiVirMemAddr, H265ENC_HEADER_MAXSIZE);
    SetMemoryAddr(&pAddr->uiPicHdr, &uiVirMemAddr, H265ENC_HEADER_MAXSIZE);

    pFuncCtx->stUsrQp.uiUsrQpMapSize = uiSizeUsrQpMap;
	SetMemoryAddr(&pFuncCtx->stUsrQp.uiQpMapAddr, &uiVirMemAddr, uiSizeUsrQpMap);
    
    SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr0, &uiVirMemAddr, uiSizeOsgGcac);
    memset((void *)pFuncCtx->stOsg.uiGcacStatAddr0, 0, uiSizeOsgGcac);
    h26x_flushCache(pFuncCtx->stOsg.uiGcacStatAddr0, uiSizeOsgGcac);
    SetMemoryAddr(&pFuncCtx->stOsg.uiGcacStatAddr1, &uiVirMemAddr, uiSizeOsgGcac);
    memset((void *)pFuncCtx->stOsg.uiGcacStatAddr1, 0, uiSizeOsgGcac);
    h26x_flushCache(pFuncCtx->stOsg.uiGcacStatAddr1, uiSizeOsgGcac);

	return H26XENC_SUCCESS;
}

void _h265Enc_initEncoderDump(H265ENC_INIT *pInit)
{
	DBG_IND("[KDRV_H265ENC] W = %u, H = %u, DisW = %u, TileEn = %u, buf = (0x%08x, %d), GOP = %u, UsrQp = %u, DB = (%u, %d, %d), Qpoff = (%d, %d), svc = %u, ltr = (%u, %u), SAO = (%u, %u, %u), bitrate = %u, fr = %u, I_QP = (%u, %u, %u), P_QP = (%u, %u, %u), ip_off = %d, sta = %u, FBCEn = %u, Gray = %u, FastSear = %u, HwPad = %u, rotate = %u, d2d = %u, LevelIDC = %u, SEI = %u, MulLayer = %u, VUI = (%u, %u, %u, %u, %u, %u, %u, %u, %u)\r\n",
		pInit->uiWidth,
		pInit->uiHeight,
		pInit->uiDisplayWidth,
		pInit->bTileEn,
		pInit->uiEncBufAddr,
		pInit->uiEncBufSize,
		pInit->uiGopNum,
		pInit->uiUsrQpSize,
		pInit->ucDisableDB,
		pInit->cDBAlpha,
		pInit->cDBBeta,
		pInit->iQpCbOffset,
		pInit->iQpCrOffset,
		pInit->ucSVCLayer,
		pInit->uiLTRInterval,
		pInit->uiLTRPreRef,
		pInit->uiSAO,
		pInit->uiSaoLumaFlag,
		pInit->uiSaoChromaFlag,
		pInit->uiBitRate,
		pInit->uiFrmRate,
		pInit->ucIQP,
		pInit->ucMinIQp,
		pInit->ucMaxIQp,
		pInit->ucPQP,
		pInit->ucMinPQp,
		pInit->ucMaxPQp,
		pInit->iIPQPoffset,
		pInit->uiStaticTime,
		pInit->bFBCEn,
		pInit->bGrayEn,
		pInit->bFastSearchEn,
		pInit->bHwPaddingEn,
		pInit->ucRotate,
		pInit->bD2dEn,
		pInit->ucLevelIdc,
		pInit->ucSEIIdfEn,
		pInit->uiMultiTLayer,
		pInit->bVUIEn,
		pInit->usSarWidth,
		pInit->usSarHeight,
		pInit->ucMatrixCoef,
		pInit->ucTransferCharacteristics,
		pInit->ucColourPrimaries,
		pInit->ucVideoFormat,
		pInit->ucColorRange,
		pInit->bTimeingPresentFlag);
}

//! API : initialize encoder
/*!
    Initialize config, buffer and write header, and init HW

    @return
        - @b H265ENC_SUCCESS: init success
        - @b H265ENC_FAIL: init fail
*/
INT32  h265Enc_initEncoder(H265ENC_INIT *pInit, H26XENC_VAR *pVar)
{
	H26XCOMN_CTX *pComnCtx;
    H265EncTileCfg TileCfg;

    /* dump param settings */
    _h265Enc_initEncoderDump(pInit);

    /* check horizontal extension */
    if(pInit->uiWidth % 16)
    {
        DBG_ERR("[H265Enc] encode width %d should be 16X !\r\n ", (int)(pInit->uiWidth));

        return H26XENC_FAIL;
    }
	if (pInit->bFastSearchEn > 1) {
		DBG_ERR("[H265Enc] fast searh(%d) > 1. \r\n",(int) pInit->bFastSearchEn);

		return H26XENC_FAIL;
	}
	if (pInit->ucColorRange > 1)
	{
		DBG_ERR("Error : h265 color range (%d) > 1. \r\n", (int)(pInit->ucColorRange));

		return H26XENC_FAIL;
	}
    if (set_tile_config(&TileCfg, pInit->bTileEn, pInit->uiWidth, pInit->bD2dEn, pInit->bGdcEn, pInit->uiHwVersion) == H26XENC_FAIL)
    {
        return H26XENC_FAIL;
    }

    if (h265Enc_initInternalMemory(pInit, pVar, &TileCfg) == H26XENC_FAIL)
    {
    	DBG_ERR("Error : h265Enc_initInternalMemory fail. \r\n");

		return H26XENC_FAIL;
    }

	if (h265Enc_initCfg(pInit, pVar, &TileCfg) == H26XENC_FAIL) {
        DBG_ERR("Error: h265Enc_initCfg fail.\r\n");
        return H26XENC_FAIL;
    }

	h265Enc_encSeqHeader(pVar->pVdoCtx, pVar->pComnCtx);

	pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;


	h265Enc_wrapSeqCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, (H265ENC_CTX *)pVar->pVdoCtx, 
                            h26x_getPhyAddr(pComnCtx->stVaAddr.uiBsdma));

	if (1) {
		{
            H265EncFroCfg sFroAttr = {
                1,
#if 0
                {{{ 255, 255, 255, 255},{ 200, 200, 192, 184},{ 185, 185, 178, 178}}, //dc
                {{ 255, 255, 243, 0},{ 175, 175, 170, 0},{ 175, 175, 170, 0}}}, //dc
                {{{ 249, 249, 243, 243},{ 192, 192, 176, 168},{ 178, 178, 164, 164}}, //ac
                {{ 243, 243, 219, 0},{ 160, 160, 140, 0},{ 160, 160, 140, 0}}}, //ac
                {{{ 3, 6, 12, 24},{ 4, 8, 16, 32},{ 4, 7, 14, 28}}, //st
                {{ 6, 12, 24, 0},{ 8, 15, 30, 0},{ 8, 15, 30, 0}}}, //st
                {{{ 30, 14, 6, 2},{ 30, 14, 6, 2},{ 30, 14, 6, 2}}, //mx
                {{ 14, 6, 2, 0},{ 14, 6, 2, 0},{ 14, 6, 2, 0}}} //mx
#else
                {{{ 255, 255, 255, 255},{ 200, 192, 184, 176},{ 168, 160, 152, 178}}, //dc
                {{ 255, 255, 255, 0},{ 175, 164, 156, 0},{ 154, 148, 142, 0}}}, //dc
                {{{ 255, 255, 255, 255},{ 192, 164, 156, 148},{ 140, 132, 124, 164}}, //ac
                {{ 250, 245, 240, 0},{ 160, 140, 132, 0},{ 126, 120, 114, 0}}}, //ac
                {{{ 3, 6, 12, 24},{ 4, 8, 16, 32},{ 4, 8, 16, 28}}, //st
                {{ 6, 12, 24, 0},{ 8, 16, 32, 0},{ 8, 16, 32, 0}}}, //st
                {{{ 30, 14, 6, 2},{ 30, 14, 6, 2},{ 30, 14, 6, 2}}, //mx
                {{ 14, 6, 2, 0},{ 14, 6, 2, 0},{ 14, 6, 2, 0}}} //mx
#endif
            };
			H26XEncVarCfg sVarAttr = { 45, 31, 255, 0, 0, 0, 2 };
            H265EncRdoCfg sRdoAttr = { 1,
                   -3, -3, -3, -3, -4, -4,
                   -4, -1, -1, -1, -1, -1,
                   -1, -1, -1, 0, 0, 0,
                    0,  0,
                    0,  0,  0,  0,  0,  0};

			// init fro //
            h265Enc_setFroCfg( pVar, &sFroAttr);
			//init var
            h26XEnc_setVarCfg( pVar, &sVarAttr);
            h265Enc_setRdoCfg( pVar, &sRdoAttr);
		}
	}

	// init sde //
	h26XEnc_setInitSde((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr);

	// enable psnr //
	H26XEnc_setPSNRCfg(pVar, TRUE);
	
    // init isp ratio //
    {
        H26XEncIspRatioCfg sIspRatioCfg={100, 100, 100, 100};

        h26XEnc_SetIspRatio(pVar, &sIspRatioCfg);
    }

	#if 0
	// init qp related //
	{
		H265EncQpRelatedCfg stCfg = {0};

		stCfg.ucMaxCuDQPDepth = 0;
		stCfg.ucImeCoherentQP = 1;	// for roi 16x16 keep //
		stCfg.ucRdoCoherentQP = 1;	// for roi 16x16 keep //
		stCfg.ucQpMergeMethod = 0;
		h265Enc_setQpRelatedCfg(pVar, &stCfg);
	}
	#endif
	
	return H26XENC_SUCCESS;
}

/*
void dump_tmnr(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	H26XEncTmnrAddr *pVaAddr;
    UINT32 uiFrmSize,uiMotSize,uiHisSize;
    UINT32 uiWidth,uiHeight,uiSum;
    uiWidth = pComnCtx->uiWidth;
    uiHeight = pComnCtx->uiHeight;

    UINT32 lcunum = ((uiWidth+63)>>6) * ((uiHeight+63)>>6);
    uiFrmSize = ((uiWidth+63)>>6) * ((uiHeight+63)>>6) * 64 * 64;
    uiMotSize = lcunum * 32 ;
    if((lcunum%2)==1)
        uiMotSize += 32;
    uiHisSize = (((uiWidth+63)>>6))*(((uiHeight+63)>>6)) * 256 * 8;//set lcux_step = lcuy_step = mbx_step = mby_step = 1,         Each LCU gets 64 sampling of mb_cnt = 0,        1 samping have 64bit

	pVaAddr = &pFuncCtx->stTmnr.stVaAddr;

	DBG_IND("dump TmnrRefAndRec\r\n");
    uiSum = buf_chk_sum((UINT8 *)pVaAddr->uiYAddr, uiFrmSize,0);
    DBG_IND("uiSum = 0x%08x\r\n",uiSum);
    uiSum = buf_chk_sum((UINT8 *)pVaAddr->uiCAddr, uiFrmSize/2,0);
    DBG_IND("uiSum = 0x%08x\r\n",uiSum);


	DBG_IND("dump uiMotAddr\r\n");
    //h26x_prtMem((UINT32)pVaAddr->uiMotAddr, uiMotSize);
    uiSum = buf_chk_sum((UINT8 *)pVaAddr->uiMotAddr, uiMotSize,0);
    DBG_IND("uiSum = 0x%08x\r\n",uiSum);
	DBG_IND("dump uiHisAddr\r\n");
    //h26x_prtMem((UINT32)pVaAddr->uiHisAddr, uiHisSize);
    uiSum = buf_chk_sum((UINT8 *)pVaAddr->uiHisAddr, uiHisSize,0);
    DBG_IND("uiSum = 0x%08x\r\n",uiSum);
}
*/


void _h265Enc_prepareOnePictureDump(H265ENC_INFO *pInfo)
{
	DBG_MSG("[KDRV_H265ENC] PicType = %d, Src = (0x%08x, 0x%08x, %u, %u), SrcCIv = %u, SrcOut = (%u, %u, 0x%08x, 0x%08x, %u, %u), SrcD2D = (%u, %u), BsOut = (0x%08x, %u), NalLenAddr = 0x%08x, timestamp = %u, TemId = %u, SkipFrmEn = %u, SkipFrmMode = %u, SeqHdrEn = %u, PredBitss = %u, BaseVa = 0x%08x, SdeCfg = (%u, %u, %u, %u, %u)\r\n",
		pInfo->ePicType,
		pInfo->uiSrcYAddr,
		pInfo->uiSrcCAddr,
		pInfo->uiSrcYLineOffset,
		pInfo->uiSrcCLineOffset,
		pInfo->bSrcCbCrIv,
		pInfo->bSrcOutEn,
		pInfo->ucSrcOutMode,
		pInfo->uiSrcOutYAddr,
		pInfo->uiSrcOutCAddr,
		pInfo->uiSrcOutYLineOffset,
		pInfo->uiSrcOutCLineOffset,
		pInfo->bSrcD2DEn,
		pInfo->ucSrcD2DMode,
		pInfo->uiBsOutBufAddr,
		pInfo->uiBsOutBufSize,
		pInfo->uiNalLenOutAddr,
		pInfo->uiSrcTimeStamp,
		pInfo->uiTemporalId,
		pInfo->bSkipFrmEn,
		pInfo->ucSkipFrmMode,
		pInfo->bGetSeqHdrEn,
		pInfo->uiPredBitss,
		pInfo->uiH26XBaseVa,
		pInfo->SdeCfg.bEnable,
		pInfo->SdeCfg.uiWidth,
		pInfo->SdeCfg.uiHeight,
		pInfo->SdeCfg.uiYLofst,
		pInfo->SdeCfg.uiCLofst);
}

//! API : encode picture for encoder
/*!
    Call prepare function and start HW to encode,

        wait for interrupt and get result

    @return
        - @b H265ENC_SUCCESS: encode success
        - @b H265ENC_FAIL   : encode fail
*/
INT32 h265Enc_prepareOnePicture(H265ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	BOOL bRecFBCEn = ((pInfo->bSrcOutEn == 1) && (pInfo->uiSrcOutYAddr == 0 || pInfo->uiSrcOutCAddr == 0)) ? 
                        FALSE : pSeqCfg->bFBCEn;

	INT32 Ret;
	pInfo->bSrcCbCrIv = 0;

	/* dump param settings */
	_h265Enc_prepareOnePictureDump(pInfo);

	// check info data correct not ready //
	if (pInfo->bSrcD2DEn) {
		if (pInfo->ucSrcD2DMode == 0) {
			// ISP 1 stripe //
			if (pSeqCfg->ucTileNum > 2) {
				DBG_ERR("codec tile number(%d) not support stripe_mode(%d)\r\n", pSeqCfg->ucTileNum, pInfo->ucSrcD2DMode);
				return H26XENC_FAIL;
			}
			else 
				pInfo->ucSrcD2DMode = ((pSeqCfg->bTileEn) && (pSeqCfg->ucTileNum == 2)) ? 2 : 0;
		}
		else if (pInfo->ucSrcD2DMode == 1) {
			// ISP 2 stripe //
			if (pSeqCfg->ucTileNum > 2) {
				DBG_ERR("codec tile number(%d) not support stripe_mode(%d)\r\n", pSeqCfg->ucTileNum, pInfo->ucSrcD2DMode);
				return H26XENC_FAIL;
			}
			else {
				if (pSeqCfg->ucTileNum == 2) {
					if (pSeqCfg->uiTileWidth[0] > pInfo->uiSrcD2DStrpSize[0]) {
						DBG_ERR("codec tile(0) width(%d) > stripe(0) size(%d)\r\n", pSeqCfg->uiTileWidth[0], pInfo->uiSrcD2DStrpSize[0]);
						return H26XENC_FAIL;
					}	
				}
				pInfo->ucSrcD2DMode = ((pSeqCfg->bTileEn) && (pSeqCfg->ucTileNum == 2)) ? 3 : 1;
			}
		}
		else if (pInfo->ucSrcD2DMode == 2) {
			// ISP 2 stripe //
			if (pSeqCfg->ucTileNum != 3) {
				DBG_ERR("codec tile number(%d) not support stripe_mode(%d)\r\n", pSeqCfg->ucTileNum, pInfo->ucSrcD2DMode);
				return H26XENC_FAIL;
			}
			else {
				if (pSeqCfg->uiTileWidth[0] > pInfo->uiSrcD2DStrpSize[0]) {
					DBG_ERR("codec tile(0) width(%d) > stripe(0) size(%d)\r\n", pSeqCfg->uiTileWidth[0], pInfo->uiSrcD2DStrpSize[0]);
					return H26XENC_FAIL;
				}
				if ( (pSeqCfg->uiTileWidth[0] + pSeqCfg->uiTileWidth[1]) > (pInfo->uiSrcD2DStrpSize[0] + pInfo->uiSrcD2DStrpSize[1])) {
					DBG_ERR("codec tile(0+1) width(%d, %d) > stripe(0+1) size(%d, %d)\r\n", 
						pSeqCfg->uiTileWidth[0], pSeqCfg->uiTileWidth[1], pInfo->uiSrcD2DStrpSize[0], pInfo->uiSrcD2DStrpSize[1]);
					return H26XENC_FAIL;
				}
				pInfo->ucSrcD2DMode = 4;
			}
		}
		else {
			return H26XENC_FAIL;
		}
	}

	// set data to picture config //
	pPicCfg->ePicType = pInfo->ePicType;
	pComnCtx->stVaAddr.uiBsOutAddr = pInfo->uiBsOutBufAddr;

	if (pComnCtx->ucRcMode == 0) 
		pPicCfg->ucSliceQP = IS_ISLICE(pInfo->ePicType) ? pSeqCfg->ucInitIQP : pSeqCfg->ucInitPQP;
	else
		pPicCfg->ucSliceQP = h26XEnc_getRcQuant(pVar, pPicCfg->ePicType, (pSeqCfg->uiLTInterval == 1 && (pPicCfg->iPoc % pSeqCfg->uiLTInterval) == 0), pInfo->uiBsOutBufSize);

    Ret = h265Enc_preparePicCfg( pVdoCtx, pInfo, pVar);
    if(Ret != H26XENC_SUCCESS)
        return Ret;

    h265Enc_updateRowRcCfg(pVar);

	if (pPicCfg->uiPrePicCnt != pComnCtx->uiPicCnt)
	{
        h265Enc_modifyRefFrm(pVdoCtx, pVar, bRecFBCEn);
    }

	h265Enc_encSliceHeader(pVdoCtx, pFuncCtx, pComnCtx, pInfo->bGetSeqHdrEn);

	Ret = h265Enc_wrapPicCfg(pRegSet, pInfo, pVar);
    if(Ret != H26XENC_SUCCESS) {
        DBG_ERR("wrong rec/ref idx");
        return Ret;
    }

	h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr, pSeqCfg->usLcuHeight, pComnCtx->uiPicCnt, 
                                IS_ISLICE(pPicCfg->ePicType));
    h26xEnc_wrapRowRcPicUpdate(pRegSet, &pFuncCtx->stRowRc, pComnCtx->uiPicCnt, IS_PSLICE(pPicCfg->ePicType), 
                                pPicCfg->ucSliceQP, pVdoCtx->uiSvcLable);
#if USE_JND_DEFAULT_VALUE
    if (gUseDefaultJNDCfg)
        h26XEnc_setJndCfg(pVar, &gEncJNDCfg);
#endif
    
	if (H26X_ENC_MODE == 0) {
		h26x_setLLCmd(pVar->uiEncId, pComnCtx->stVaAddr.uiAPBAddr, pComnCtx->stVaAddr.uiLLCAddr, 0, h26x_getHwRegSize() + 64);
	}
    //dump_tmnr(pVar);

    return H26XENC_SUCCESS;
}

void h265Enc_updateLtPOC(H265EncSeqCfg *pH265EncSeqCfg, H265EncPicCfg *pH265EncPicCfg)
{
	//update long-term poc
	if (pH265EncSeqCfg->uiLTRpicFlg) {
		if (pH265EncSeqCfg->bEnableLt == 1 && (pH265EncPicCfg->iPoc % pH265EncSeqCfg->uiLTInterval) == 0
                    && pH265EncSeqCfg->uiLTRPreRef == 1) {
			pH265EncSeqCfg->iPocLT[0] = pH265EncPicCfg->iPoc;  //update refernce frame
		} else if (pH265EncSeqCfg->bEnableLt == 1 && (pH265EncPicCfg->iPoc % pH265EncSeqCfg->uiLTInterval) == 0
		            && pH265EncSeqCfg->uiLTRPreRef == 0 && IS_IDRSLICE(pH265EncPicCfg->ePicType)) {
			pH265EncSeqCfg->iPocLT[0] = pH265EncPicCfg->iPoc;  //update refernce frame
		}
	}
	DBG_IND("lt pic[0] %d\r\n", (int)(pH265EncSeqCfg->iPocLT[0]));
}

INT32 h265Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet    *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;
	H265EncSeqCfg  *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg  *pPicCfg = &pVdoCtx->stPicCfg;
    UINT32 cu16x16_size;
    UINT32 aq_cu_num;

    //dump_tmnr(pVar);
    #if 0

	if (enc_mode)
	{
		// direct mode //
		h26x_getEncReport(pRegSet->CHK_REPORT);	// shall remove while link-list //
		pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalLCUs>>1))/pVdoCtx->stSeqCfg.uiTotalLCUs;
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
		pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalLCUs>>1))/pVdoCtx->stSeqCfg.uiTotalLCUs;
	}
    #else
	if (enc_mode)
	{
		// direct mode //
		h26x_getEncReport(pRegSet->CHK_REPORT);	// shall remove while link-list //
	}
	//fmem_dcache_sync((void *)pRegSet, sizeof(H26XRegSet), DMA_BIDIRECTIONAL);
	#if 1
    aq_cu_num = pVdoCtx->stSeqCfg.uiTotalLCUs << (2*pFuncCtx->stAq.stCfg.ucDepth);
    pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (aq_cu_num>>1))/aq_cu_num;
    #else
	pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (pVdoCtx->stSeqCfg.uiTotalLCUs>>1))/pVdoCtx->stSeqCfg.uiTotalLCUs;
#endif
#endif

    pResult->uiBSLen = pRegSet->CHK_REPORT[H26X_BS_LEN];
    pResult->uiBSChkSum = pRegSet->CHK_REPORT[H26X_BS_CHKSUM];
    pResult->uiRDOCost[0] = pRegSet->CHK_REPORT[H26X_RRC_RDOPT_COST_LSB];
    pResult->uiRDOCost[1] = pRegSet->CHK_REPORT[H26X_RRC_RDOPT_COST_MSB];
    pResult->uiAvgQP = (pRegSet->CHK_REPORT[H26X_RRC_QP_SUM] + (pVdoCtx->stSeqCfg.uiTotalLCUs>>1))/pVdoCtx->stSeqCfg.uiTotalLCUs; // row rc
    cu16x16_size = SIZE_16X(pVdoCtx->stSeqCfg.usWidth)/16*SIZE_16X(pVdoCtx->stSeqCfg.usHeight)/16;
    //pResult->uiAvgQP = (pRegSet->CHK_REPORT[H26X_QP_CHKSUM] + (cu16x16_size>>1))/cu16x16_size;

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
        pResult->uiMotionRatio = (((pRegSet->CHK_REPORT[H26X_SCD_REPORT]>>16)&0xffff)*100 + (cu16x16_size>>1))/cu16x16_size;
    else
        pResult->uiMotionRatio = 0;

    pResult->iVPSHdrLen = pVdoCtx->uiVPSHdrLen;
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
	pResult->bRefLT  = (pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0));

	h26x_flushCache(pComnCtx->stVaAddr.uiBsOutAddr, pResult->uiBSLen);

	h265Enc_updateLtPOC(pSeqCfg, pPicCfg);
    h265Enc_updatePicCnt(pVar);
    h26XEnc_getRowRcState(pPicCfg->ePicType, pRegSet, &pFuncCtx->stRowRc, pResult->uiSvcLable);
	h26XEnc_UpdateRc(pVar, pVdoCtx->stPicCfg.ePicType, pResult);

	//pComnCtx->uiPicCnt++;
    pResult->ucNxtPicType = (pVdoCtx->stPicCfg.iPoc % pVdoCtx->stSeqCfg.uiGopNum) ? P_SLICE : IDR_SLICE;
	pResult->ucQP = pVdoCtx->stPicCfg.ucSliceQP;

	if(pPicCfg->uiTemporalId[0] == pSeqCfg->TidLt && pSeqCfg->TidLt)  
		pResult->ucPicType = 4; //is KeyP
	else
		pResult->ucPicType = pPicCfg->ePicType;			
	
	pResult->uiHwEncTime = pRegSet->CHK_REPORT[H26X_CYCLE_CNT]/h26x_getClk();
		
    pVdoCtx->stPicCfg.eNxtPicType = pResult->ucNxtPicType;
	pComnCtx->uiLastHwInt = uiHwInt;

    return H26XENC_SUCCESS;
}

UINT32 h265Enc_getSVCLabel(H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncSeqCfg  *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg  *pPicCfg = &pVdoCtx->stPicCfg;
    UINT32 ref_poc, svc_label = 0;
    
    ref_poc = (pSeqCfg->u32LtInterval) ? ( (pPicCfg->iPoc)  %  (int)pSeqCfg->u32LtInterval ) : pPicCfg->iPoc;
    if (pSeqCfg->ucSVCLayer == 2) {
        UINT32 svc_order;
        svc_order = ref_poc%4;
        if (0 == svc_order)
            svc_label = 0;
        else if (2 == svc_order)
            svc_label = 1;
        else
            svc_label = 2;
    }
    else if (pSeqCfg->ucSVCLayer == 1) {
        svc_label = ref_poc%2;
    }
    return svc_label;
}

UINT32 h265Enc_getEncodeRatio(UINT32 chip_idx, H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncSeqCfg  *pSeqCfg = &pVdoCtx->stSeqCfg;
    UINT32 result = h26x_getDbg1(0);
    UINT32 lcu_x, lcu_y;
    UINT32 enc_ratio;

    lcu_y = (result&0x00ff0000)>>16;
    lcu_x = (result&0xff000000)>>24;
    enc_ratio = (lcu_y * pSeqCfg->usLcuWidth + lcu_x)*100 / pSeqCfg->uiTotalLCUs;
    if (0 == enc_ratio)
        enc_ratio = 1;
    return enc_ratio;
}

INT32 h265Enc_setRdoCfg(H26XENC_VAR *pVar, H265EncRdoCfg *pRdo)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncRdoCfg *pRdoCfg = &pVdoCtx->stRdoCfg;

	// check info data correct not ready //

	memcpy(pRdoCfg, pRdo, sizeof(H265EncRdoCfg));

	h265Enc_wrapRdoCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);

	return H26XENC_SUCCESS;
}

INT32 h265Enc_setFroCfg(H26XENC_VAR *pVar, H265EncFroCfg *pFro)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncFroCfg *pFroCfg = &pVdoCtx->stFroCfg;

	// check info data correct not ready //

	memcpy(pFroCfg, pFro, sizeof(H265EncFroCfg));

	h265Enc_wrapFroCfg((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);

	return H26XENC_SUCCESS;
}

void h265Enc_setPicQP(UINT8 ucQP, H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	pPicCfg->ucSliceQP = ucQP;
}

INT32 h265Enc_setQpRelatedCfg(H26XENC_VAR *pVar, H265EncQpRelatedCfg *pQR)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265ENC_CTX *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;
	H265EncQpRelatedCfg *pQpCfg = &pVdoCtx->stQPCfg;

	memcpy(pQpCfg, pQR, sizeof(H265EncQpRelatedCfg));
	h265Enc_wrapQpRelatedCfg(pRegSet, pQpCfg);

	return H26XENC_SUCCESS;
}

UINT32 h265Enc_queryMemSize(H26XEncMeminfo *pInfo)
{
	// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiFrmBufNum = 1 + (pInfo->ucSVCLayer == 2) + (pInfo->uiLTRInterval != 0);
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = SIZE_64X(pInfo->uiWidth) * SIZE_64X(pInfo->uiHeight);
    UINT32 uiSizeSideInfoLof[H26X_MAX_TILE_NUM];
    UINT32 uiSizeSideInfo[H26X_MAX_TILE_NUM];
	UINT32 uiSizeColMVs   = SIZE_16X((((((pInfo->uiWidth + 63) >> 6) * ((pInfo->uiHeight + 63) >> 6)) << 6) << 2));
    UINT32 uiExtraRecSize = SIZE_64X(pInfo->uiHeight) << 7;

    UINT32 uiSizeBSDMA    = ((((pInfo->uiHeight + 63)>>6)<<1) + 1 + 4) << 2;
	UINT32 uiSizeUsrQpMap = ((pInfo->uiWidth + 63) * (pInfo->uiHeight + 63) << 1) >> 8;
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiNaluLength   = (pInfo->uiHeight+63) >> 6;
    UINT32 uiSizeRcRef    = SIZE_4X(((pInfo->uiHeight + 63) >> 6)) << 4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = h26x_getHwRegSize() + 64;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H265ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));
    H265EncTileCfg TileCfg;

	uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC +
						H265ENC_HEADER_MAXSIZE*2 + uiSizeUsrQpMap + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;//256 for 16x alignment

    set_tile_config(&TileCfg, pInfo->bTileEn, pInfo->uiWidth, pInfo->bD2dEn, pInfo->bGdcEn, pInfo->uiHwVersion);

    if (TileCfg.bTileEn) {
        UINT8 i;
        for (i = 0; i < TileCfg.ucTileNum; i++) {
            if (i < TileCfg.ucTileNum-1) {
                //(size_64(frm_height))*(128 pxl)*(ycbcr)*(frm buf num)
                uiTotalBufSize += (uiExtraRecSize*3/2*uiFrmBufNum);
            }

            uiSizeSideInfoLof[i] = SIZE_32X(((((TileCfg.uiTileWidth[i]+63)>>6) << 3)));
            uiSizeSideInfo[i] = uiSizeSideInfoLof[i]*((pInfo->uiHeight+63)>>6);

            //sideinfo*(rd/wr)*(frm buf num)
            uiTotalBufSize += (uiSizeSideInfo[i]*2*uiFrmBufNum);
            uiTotalBufSize += uiSizeBSDMA;
            uiTotalBufSize += uiNaluLength;
            //(rrc buf)*(i/p)*(rd/wr)
            uiTotalBufSize += (uiSizeRcRef << 2);
        }
    } else {
        uiSizeSideInfoLof[0] = SIZE_32X((((pInfo->uiWidth+63)>>6) << 3));
        uiSizeSideInfo[0] = uiSizeSideInfoLof[0]*((pInfo->uiHeight+63)>>6);

        uiTotalBufSize += uiSizeSideInfo[0];
        uiTotalBufSize += uiSizeBSDMA;
        uiTotalBufSize += uiNaluLength;
        uiTotalBufSize += (uiSizeRcRef << 2);
    }

	return SIZE_4X(uiTotalBufSize);
}

void h265Enc_getSeqHdr(H26XENC_VAR *pVar, UINT32 *pAddr, UINT32 *pSize)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	*pSize = pVdoCtx->uiSeqHdrLen;
	*pAddr = pComnCtx->stVaAddr.uiSeqHdr;
}

UINT32 h265Enc_getNxtPicType(H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;

	return (UINT32)pVdoCtx->stPicCfg.eNxtPicType;
}

