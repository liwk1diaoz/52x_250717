/*
    H265ENC module driver for NT96660

    Interface for Init and Info

    @file       h265enc_api.c
    @ingroup    mICodecH265

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "nvt_vdocdc_dbg.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_int.h"
#include "h26xenc_wrap.h"

#include "h265enc_api.h"
#include "h265enc_int.h"
#include "h265enc_wrap.h"
#include "h265enc_header.h"
#include "h26x_bitstream.h"

#include "kdrv_videoenc/kdrv_videoenc_lmt.h"

#if JND_DEFAULT_ENABLE
    extern H26XEncJndCfg gEncJNDCfg;
#endif
#if H26X_SET_PROC_PARAM
extern int gH265RowRCStopFactor;
#endif
extern UINT32 gForceLongStartCode;

static INT32 set_tile_config(H265EncTileCfg *pTileCfg, UINT32 frm_width, BOOL bD2d, BOOL bGdc, HEVC_QLVL_TYPE q_level)
{
	BOOL bTile=0;
	UINT32 fsr_width=0;

	// determine tile enable
	if (bD2d) {
		if (bGdc) {
			bTile = (frm_width <  H265_GDC_MIN_W_1_TILE)? 0:1;
			fsr_width = 1536;
		} else {
			bTile = (frm_width <= H265_D2D_MIN_W_1_TILE)? 0:1;
			fsr_width = 1536;
		}
	} else {
		if (q_level == HEVC_QUALITY_MAIN) {
			bTile = (frm_width <= H265E_MAX_WIDTH_WITHOUT_TILE)? 0:1;
			fsr_width = 1152;
		} else if (q_level == HEVC_QUALITY_BASE) {
			bTile = (frm_width <= H265E_MAX_WIDTH_WITHOUT_TILE)? 0:1;
			fsr_width = 1536;
		} else {
			DBG_ERR("quality_level not support yet:%d\r\n", q_level);
		}
	}

	DBG_IND("[KDRV_H265ENC] bTile = %u, bD2d = %u, bGdc = %u, frm_width = %u, q_lvl: %u\r\n", (int)bTile, (int)bD2d, (int)bGdc, (int)frm_width, (int)q_level);
    memset(pTileCfg, 0, sizeof(H265EncTileCfg));
    pTileCfg->bTileEn = bTile;

	// determine tile config
	if (bTile) {
		if (frm_width <= (fsr_width*2-256)) {
			pTileCfg->ucTileNum = 2;
			if (bD2d == 1) {
				if (bGdc == 1) {
					pTileCfg->uiTileWidth[0] = SIZE_128X(frm_width/2);
				} else {
                    pTileCfg->uiTileWidth[0] = H26X_FIRST_TILE_MAX_W_SR28;
				}
			} else {
				pTileCfg->uiTileWidth[0] = ((frm_width >> 1) < H26X_FIRST_TILE_MIN_W)? H26X_FIRST_TILE_MIN_W: SIZE_64X(frm_width >> 1);
			}
			pTileCfg->uiTileWidth[1] = frm_width - pTileCfg->uiTileWidth[0];
		} else if (frm_width <= (fsr_width*3-512)) {
			pTileCfg->ucTileNum = 3;
			if (frm_width <= (fsr_width-256)*3) {
				pTileCfg->uiTileWidth[0] = pTileCfg->uiTileWidth[1] = ((bD2d == 1)? SIZE_128X(frm_width / 3): SIZE_64X(frm_width / 3));
				pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0]*2;
			} else {
				pTileCfg->uiTileWidth[1] = (fsr_width-256);
				pTileCfg->uiTileWidth[0] = ((bD2d==1)? SIZE_128X((frm_width-pTileCfg->uiTileWidth[1])/2): SIZE_64X((frm_width-pTileCfg->uiTileWidth[1])/2));
				pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1];
			}
		} else {
			if (bD2d == 1) {
				DBG_ERR("Error: width(%d) is over limit(%d), gdc:%d, d2d:%d, tile:%d\r\n", (int)frm_width, H265E_D2D_MAX_W_528, (int)bGdc, (int)bD2d, (int)bTile);
			} else {
				if (frm_width <= (fsr_width*4-768)) {
					pTileCfg->ucTileNum = 4;
					if (frm_width <= (fsr_width-256)*4) {
						pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width/4);
						pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]*3);
					} else {
						pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = (fsr_width-256);
						pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*2)/2);
						pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
					}
				} else {
					pTileCfg->ucTileNum = 5;
					if (frm_width <= (fsr_width-256)*5) {
						pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width/5);
						pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]*4);
					} else {
						pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = (fsr_width-256);
						pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*3)/2);
						pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
					}
				}
			}
		}
	} else {
		pTileCfg->ucTileNum = 0;
	}


	DBG_IND("[KDRV_H265ENC] tile num = %u, width = (%u, %u, %u, %u, %u)\r\n", (int)pTileCfg->ucTileNum, (int)pTileCfg->uiTileWidth[0], (int)pTileCfg->uiTileWidth[1], (int)pTileCfg->uiTileWidth[2], (int)pTileCfg->uiTileWidth[3], (int)pTileCfg->uiTileWidth[4]);
	return H26XENC_SUCCESS;
}

static INT32 set_tile_config_528(H265EncTileCfg *pTileCfg, UINT32 frm_width, BOOL bD2d, BOOL bGdc, HEVC_QLVL_TYPE q_level)
{
	BOOL bTile=0;
	UINT32 fsr_width=0;

	// determine tile enable
	if (bD2d) {
		// same as 520 condition
		if (bGdc) {
			bTile = (frm_width <  H265_GDC_MIN_W_1_TILE_528)? 0:1;
			fsr_width = 1536;
		} else {
			bTile = (frm_width <= H265_D2D_MIN_W_1_TILE_528)? 0:1;
			fsr_width = 2176;
		}
	} else {
		if (q_level == HEVC_QUALITY_MAIN) {
			bTile = (frm_width <= H26X_MAX_W_WITHOUT_TILE_V36_528)? 0:1;
			fsr_width = 1536;
		} else if (q_level == HEVC_QUALITY_BASE) {
			bTile = (frm_width <= H26X_MAX_W_WITHOUT_TILE_V20_528)? 0:1;;
			fsr_width = 2560;
		} else {
			DBG_ERR("quality_level not support yet:%d\r\n", (int)q_level);
		}
	}

	DBG_IND("[KDRV_H265ENC] bTile = %u, bD2d = %u, bGdc = %u, frm_width = %u, q_lvl: %u\r\n", (int)bTile, (int)bD2d, (int)bGdc, (int)frm_width, (int)q_level);
	memset(pTileCfg, 0, sizeof(H265EncTileCfg));
	pTileCfg->bTileEn = bTile;

	// determine tile config
	if (bTile) {
		if (frm_width <= (fsr_width*2-256)) {
			pTileCfg->ucTileNum = 2;
 			if (bD2d == 1 && bGdc == 1) {
				pTileCfg->uiTileWidth[0] = SIZE_128X(frm_width/2);
			} else {
				pTileCfg->uiTileWidth[0] = ((frm_width >> 1) < H26X_FIRST_TILE_MIN_W)? H26X_FIRST_TILE_MIN_W: SIZE_64X(frm_width >> 1);
			}
			pTileCfg->uiTileWidth[1] = frm_width - pTileCfg->uiTileWidth[0];
		} else if (frm_width <= (fsr_width*3-512)) {
			pTileCfg->ucTileNum = 3;
			if (frm_width <= (fsr_width-256)*3) {
				pTileCfg->uiTileWidth[0] = pTileCfg->uiTileWidth[1] = ((bD2d == 1)? SIZE_128X(frm_width / 3): SIZE_64X(frm_width / 3));
				pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0]*2;
			} else {
				pTileCfg->uiTileWidth[1] = (fsr_width-256);
				pTileCfg->uiTileWidth[0] = ((bD2d == 1)? SIZE_128X((frm_width-pTileCfg->uiTileWidth[1])/2): SIZE_64X((frm_width-pTileCfg->uiTileWidth[1])/2));
				pTileCfg->uiTileWidth[2] = frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1];
			}
		} else {
			if (bD2d == 1) {
				DBG_ERR("Error: not support width(%d), gdc:%d, d2d:%d, tile:%d\r\n", (int)frm_width, (int)bGdc, (int)bD2d, (int)bTile);
			} else {
				if (frm_width <= (fsr_width*4-768)) {
					pTileCfg->ucTileNum = 4;
					if (frm_width <= (fsr_width-256)*4) {
						pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width/4);
						pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]*3);
					} else {
						pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = (fsr_width-256);
						pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*2)/2);
						pTileCfg->uiTileWidth[3] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2]);
					}
				} else {
					pTileCfg->ucTileNum = 5;
					if (frm_width <= (fsr_width-256)*5) {
						pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = pTileCfg->uiTileWidth[0] = SIZE_64X(frm_width/5);
						pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0]*4);
					} else {
						pTileCfg->uiTileWidth[3] = pTileCfg->uiTileWidth[2] = pTileCfg->uiTileWidth[1] = (fsr_width-256);
						pTileCfg->uiTileWidth[0] = SIZE_64X((frm_width - pTileCfg->uiTileWidth[1]*3)/2);
						pTileCfg->uiTileWidth[4] = SIZE_16X(frm_width - pTileCfg->uiTileWidth[0] - pTileCfg->uiTileWidth[1] - pTileCfg->uiTileWidth[2] - pTileCfg->uiTileWidth[3]);
					}
				}
			}
		}
	}

	DBG_IND("[KDRV_H265ENC] tile num = %u, width = (%u, %u, %u, %u, %u)\r\n", (int)pTileCfg->ucTileNum, (int)pTileCfg->uiTileWidth[0], (int)pTileCfg->uiTileWidth[1], (int)pTileCfg->uiTileWidth[2], (int)pTileCfg->uiTileWidth[3], (int)pTileCfg->uiTileWidth[4]);
	return H26XENC_SUCCESS;
}

//! Initialize internal memory
/*!
    Divide internal memory into different buffers

    @return
        - @b H265ENC_SUCCESS: init success
        - @b H265ENC_FAIL: size is too small or address is null
*/
#if H26X_MEM_USAGE
extern H26XMemUsage g_mem_usage[2][16];
#endif
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
	UINT32 uiTotalSizeSideInfo=0;
	UINT32 uiSizeColMVs   = (pInit->bColMvEn==0)? 0: SIZE_16X((((((pInit->uiWidth + 63) >> 6) * ((pInit->uiHeight + 63) >> 6)) << 6) << 2));
	UINT32 uiExtraRecSize = SIZE_64X(pInit->uiHeight) << 7;

	UINT32 uiSizeBSDMA    = ((((pInit->uiHeight + 63)>>6)<<1) + 1 + 4) << 2;
	UINT32 uiSizeUsrQpMap = (((pInit->uiWidth + 63)>>6) * ((pInit->uiHeight + 63)>>6)) * 16 * 2;
	UINT32 uiSizeMDMap    = ((SIZE_512X(pInit->uiWidth) >> 4) * (SIZE_16X(pInit->uiHeight) >> 4)) >> 3;
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiNaluLength   = 0;//   = ((pInit->uiHeight+63) >> 6)*4 + 16;
	UINT32 uiSeqHdrSize   = H265ENC_HEADER_MAXSIZE;
	UINT32 uiPicHdrSize   = (((pInit->uiHeight+63)/64)*20) + uiSeqHdrSize + 128;	// each slice header + sequence header + sei(128) //
	UINT32 uiSizeRcRef    = SIZE_4X(((pInit->uiHeight + 63) >> 6)) << 4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = (H26X_ENC_MODE == 0)? h26x_getHwRegSize() + 64: 0;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H265ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));

	UINT8 i, k=0;
	UINT32 uiRecBufAddr=0;

#if H26X_USE_DIFF_MAQ
	if (pInit->bRecBufComm) {
		uiTotalBufSize = uiSizeColMVs*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeUsrQpMap + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
	                        uiSeqHdrSize + uiPicHdrSize + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;// 256 for 16x alignment
	} else {
		uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeUsrQpMap + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
	                        uiSeqHdrSize + uiPicHdrSize + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;// 256 for 16x alignment
	}
#else
	if (pInit->bRecBufComm) {
		uiTotalBufSize = uiSizeColMVs*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeUsrQpMap + uiSizeMDMap*3 +
							uiSeqHdrSize + uiPicHdrSize + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;// 256 for 16x alignment
	} else {
		uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeUsrQpMap + uiSizeMDMap*3 +
							uiSeqHdrSize + uiPicHdrSize + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;// 256 for 16x alignment
	}
#endif

	if(pTileCfg->bTileEn) {
		UINT32 slice_num = 0;
		for (i = 0; i < pTileCfg->ucTileNum; i++) {
			slice_num += ((pInit->uiHeight+63) >> 6);
		}
		uiNaluLength = SIZE_256X(slice_num * 4) + 16;
		if(slice_num % 64 == 0){
			uiNaluLength += 256;
		}
		uiTotalBufSize += uiNaluLength;

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
			//(rrc buf)*(i/p)*(rd/wr)
#if (0 == RRC_BY_FRAME_LEVEL)
			uiTotalBufSize += (uiSizeRcRef << 2);
#else
			uiTotalBufSize += uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2);
#endif
		}
	} else {
		UINT32 slice_num = 0;
		uiSizeSideInfoLof[0] = SIZE_32X((((pInit->uiWidth+63)>>6) << 3));
		uiSizeSideInfo[0] = uiSizeSideInfoLof[0]*((pInit->uiHeight+63)>>6);
		slice_num = (pInit->uiHeight+63) >> 6;
		uiNaluLength = SIZE_256X(slice_num * 4) + 16;
		if(slice_num % 64 == 0){
			//ic limitaion: 64X need more 256 byte
			uiNaluLength += 256;
		}

		uiTotalBufSize += uiSizeSideInfo[0];
		uiTotalBufSize += uiSizeBSDMA;
		uiTotalBufSize += uiNaluLength;
#if (0 == RRC_BY_FRAME_LEVEL)
		uiTotalBufSize += (uiSizeRcRef << 2);
#else
		uiTotalBufSize += uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2);
#endif
	}

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
	if (pTileCfg->bTileEn) {
		for (i = 0; i < pTileCfg->ucTileNum; i++) {
			DBG_IND("uiSizeSideInfo[%d]    : 0x%08x\r\n", (int)i, (int)uiSizeSideInfo[i]);
		}
		DBG_IND("uiExtraRecSize    : 0x%08x\r\n", (int)uiExtraRecSize);
	} else {
		DBG_IND("uiSizeSideInfo    : 0x%08x\r\n", (int)uiSizeSideInfo[0]);
	}
	DBG_IND("uiSizeColMVs      : 0x%08x\r\n", (int)uiSizeColMVs);
	DBG_IND("uiSizeRcRef       : 0x%08x\r\n", (int)uiSizeRcRef);
	DBG_IND("uiSizeBSDMA       : 0x%08x\r\n", (int)uiSizeBSDMA);
	DBG_IND("uiSizeUsrQpMap    : 0x%08x\r\n", (int)uiSizeUsrQpMap);
	DBG_IND("uiSizeMdMap       : 0x%08x\r\n", (int)uiSizeMDMap);
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

		// header buffer size //
		pComnCtx->uiPicHdrBufSize = uiPicHdrSize;
	}

	// physical and veritaul buffer mangement //
	pAddr = &pComnCtx->stVaAddr;
	pAddr->uiFrmBufNum = uiFrmBufNum;

	if (pInit->bRecBufComm) {
		uiRecBufAddr = pInit->uiRecBufAddr[k++];
		SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiRecBufAddr, uiFrmSize);
		SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiRecBufAddr, uiFrmSize>>1);
	} else {
		SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize);
		SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_0], &uiVirMemAddr, uiFrmSize>>1);
	}
	SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_ST_0], &uiVirMemAddr, uiSizeColMVs);
	memset((void *)pAddr->uiColMvs[FRM_IDX_ST_0], 0, uiSizeColMVs);
	h26x_cache_clean(pAddr->uiColMvs[FRM_IDX_ST_0], uiSizeColMVs);

	if(pTileCfg->bTileEn) {
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
		if (pInit->bRecBufComm) {
			uiRecBufAddr = pInit->uiRecBufAddr[k++];
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiRecBufAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiRecBufAddr, uiFrmSize>>1);
		} else {
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_ST_1], &uiVirMemAddr, uiFrmSize>>1);
		}
		SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_ST_1], &uiVirMemAddr, uiSizeColMVs);
		memset((void *)pAddr->uiColMvs[FRM_IDX_ST_1], 0, uiSizeColMVs);
		h26x_cache_clean(pAddr->uiColMvs[FRM_IDX_ST_1], uiSizeColMVs);

		if(pTileCfg->bTileEn) {
			for (i = 0; i < pTileCfg->ucTileNum; i++) {
				if (i < pTileCfg->ucTileNum-1) {
					SetMemoryAddr(&pAddr->uiTileExtraY[i][FRM_IDX_ST_1], &uiVirMemAddr, uiExtraRecSize);
					SetMemoryAddr(&pAddr->uiTileExtraC[i][FRM_IDX_ST_1], &uiVirMemAddr, uiExtraRecSize>>1);
				}
			}
			// set rd/wr buffers when enable tile mode
			SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiTotalSizeSideInfo);
			SetMemoryAddr(&pAddr->uiSideInfo[1][FRM_IDX_ST_1], &uiVirMemAddr, uiTotalSizeSideInfo);
		} else {
			SetMemoryAddr(&pAddr->uiSideInfo[0][FRM_IDX_ST_1], &uiVirMemAddr, uiSizeSideInfo[0]);
		}
	}

	if (pInit->uiLTRInterval != 0) {
		if (pInit->bRecBufComm) {
			uiRecBufAddr = pInit->uiRecBufAddr[k];
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiRecBufAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiRecBufAddr, uiFrmSize>>1);
		} else {
			SetMemoryAddr(&pAddr->uiRecRefY[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize);
			SetMemoryAddr(&pAddr->uiRecRefC[FRM_IDX_LT_0], &uiVirMemAddr, uiFrmSize>>1);
		}
		SetMemoryAddr(&pAddr->uiColMvs[FRM_IDX_LT_0], &uiVirMemAddr, uiSizeColMVs);
		memset((void *)pAddr->uiColMvs[FRM_IDX_LT_0], 0, uiSizeColMVs);
		h26x_cache_clean(pAddr->uiColMvs[FRM_IDX_LT_0], uiSizeColMVs);

		if(pTileCfg->bTileEn) {
			for (i = 0; i < pTileCfg->ucTileNum; i++) {
				if (i < pTileCfg->ucTileNum-1) {
					SetMemoryAddr(&pAddr->uiTileExtraY[i][FRM_IDX_LT_0], &uiVirMemAddr, uiExtraRecSize);
					SetMemoryAddr(&pAddr->uiTileExtraC[i][FRM_IDX_LT_0], &uiVirMemAddr, uiExtraRecSize/2);
				}
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
	if (uiSizeLLC) {
		SetMemoryAddr(&pAddr->uiLLCAddr, &uiVirMemAddr, uiSizeLLC);
		memset((void *)pAddr->uiLLCAddr, 0, uiSizeLLC);
	}

	if (pTileCfg->bTileEn) {
		SetMemoryAddr(&pAddr->uiBsdma, &uiVirMemAddr, uiSizeBSDMA*pTileCfg->ucTileNum);
	} else {
		SetMemoryAddr(&pAddr->uiBsdma, &uiVirMemAddr, uiSizeBSDMA);
	}
	SetMemoryAddr(&pAddr->uiNaluLen, &uiVirMemAddr, uiNaluLength);

	#if RRC_BY_FRAME_LEVEL
	for (i = 0; i < MAX_RRC_FRAME_LEVEL; i++) {
		SetMemoryAddr(&pAddr->uiRRC[i][0], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
		SetMemoryAddr(&pAddr->uiRRC[i][1], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
	}
	#else
	SetMemoryAddr(&pAddr->uiRRC[0][0], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
	SetMemoryAddr(&pAddr->uiRRC[0][1], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
	SetMemoryAddr(&pAddr->uiRRC[1][0], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
	SetMemoryAddr(&pAddr->uiRRC[1][1], &uiVirMemAddr, uiSizeRcRef + ((pTileCfg->bTileEn)? (pTileCfg->ucTileNum-1)*uiSizeRcRef: 0));
	#endif
	SetMemoryAddr(&pAddr->uiSeqHdr, &uiVirMemAddr, uiSeqHdrSize);
	SetMemoryAddr(&pAddr->uiPicHdr, &uiVirMemAddr, uiPicHdrSize);

	pFuncCtx->stUsrQp.uiUsrQpMapSize = uiSizeUsrQpMap;
	SetMemoryAddr(&pFuncCtx->stUsrQp.uiQpMapAddr, &uiVirMemAddr, uiSizeUsrQpMap);

	#if H26X_USE_DIFF_MAQ
	pFuncCtx->stMAq.stAddrCfg.ucMotBufNum = 2; //MAQ and MAQ diff
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
#if H26X_MEM_USAGE
	{
		g_mem_usage[0][pVar->uiEncId].cxt_size = uiTotalBufSize;

		i = 0;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_VDO_CTX].st_adr = (UINT32)pVar->pVdoCtx;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_VDO_CTX].size = uiSizeVdoCtx;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FUNC_CTX].st_adr = (UINT32)pVar->pFuncCtx;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FUNC_CTX].size = uiSizeFuncCtx;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COMN_CTX].st_adr = (UINT32)pVar->pComnCtx;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COMN_CTX].size = uiSizeComnCtx;

		{
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST0].st_adr = pAddr->uiRecRefY[FRM_IDX_ST_0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST0].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV].st_adr = pAddr->uiColMvs[FRM_IDX_ST_0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV].size = uiSizeColMVs;
			if (pTileCfg->bTileEn) {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_ST0].st_adr = pAddr->uiTileExtraY[0][FRM_IDX_ST_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_ST0].size = uiExtraRecSize*3/2 * (pTileCfg->ucTileNum-1);

				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].size = uiTotalSizeSideInfo*2;

			} else {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST0].size = uiSizeSideInfo[0];
			}
		}
		if (pAddr->uiRecRefY[FRM_IDX_ST_1]) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST1].st_adr = pAddr->uiRecRefY[FRM_IDX_ST_1];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_ST1].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV_ST1].st_adr = pAddr->uiColMvs[FRM_IDX_ST_1];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV_ST1].size = uiSizeColMVs;

			if (pTileCfg->bTileEn) {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_ST1].st_adr = pAddr->uiTileExtraY[0][FRM_IDX_ST_1];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_ST1].size = uiExtraRecSize*3/2 * (pTileCfg->ucTileNum-1);

				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_1];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].size = uiTotalSizeSideInfo*2;
			} else {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].st_adr = pAddr->uiSideInfo[0][FRM_IDX_ST_1];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_ST1].size = uiSizeSideInfo[0];
			}
		}

		if (pAddr->uiRecRefY[FRM_IDX_LT_0]) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_LT].st_adr = pAddr->uiRecRefY[FRM_IDX_LT_0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_FRM_LT].size = (pInit->bRecBufComm)? 0: uiFrmSize*3/2;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV_LT].st_adr = pAddr->uiColMvs[FRM_IDX_LT_0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_COLMV_LT].size = uiSizeColMVs;

			if (pTileCfg->bTileEn) {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_LT].st_adr = pAddr->uiTileExtraY[0][FRM_IDX_LT_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_EXTRA_LT].size = uiExtraRecSize*3/2 * (pTileCfg->ucTileNum-1);

				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].st_adr = pAddr->uiSideInfo[0][FRM_IDX_LT_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].size = uiTotalSizeSideInfo*2;

			} else {
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].st_adr = pAddr->uiSideInfo[0][FRM_IDX_LT_0];
				g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SI_LT].size = uiSizeSideInfo[0];
			}
		}
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_APB].st_adr = pAddr->uiAPBAddr;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_APB].size = uiSizeAPB;
		if (uiSizeLLC) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_LL].st_adr = pAddr->uiLLCAddr;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_LL].size = uiSizeLLC;
		}

		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_BSDMA].st_adr = pAddr->uiBsdma;
		if (pTileCfg->bTileEn) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_BSDMA].size = uiSizeBSDMA*pTileCfg->ucTileNum;
		} else {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_BSDMA].size = uiSizeBSDMA;
		}
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_NALU_LEN].st_adr = pAddr->uiNaluLen;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_NALU_LEN].size = uiNaluLength;
#if RRC_BY_FRAME_LEVEL
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].st_adr = pAddr->uiRRC[0][0];
		if (pTileCfg->bTileEn) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*2*MAX_RRC_FRAME_LEVEL*pTileCfg->ucTileNum;
		} else {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*2*MAX_RRC_FRAME_LEVEL;
		}
#else
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].st_adr = pAddr->uiRRC[0];
		if (pTileCfg->bTileEn) {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*4**pTileCfg->ucTileNum;
		} else {
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_RC_REF].size = uiSizeRcRef*4;
		}
#endif

		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SEQ_HDR].st_adr = pAddr->uiSeqHdr;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_SEQ_HDR].size = uiSeqHdrSize;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_PIC_HDR].st_adr = pAddr->uiPicHdr;
		g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_PIC_HDR].size = uiPicHdrSize;

		{
			H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_USR_QP].st_adr = pFuncCtx->stUsrQp.uiQpMapAddr;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_USR_QP].size = uiSizeUsrQpMap;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_MD].st_adr = pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_MD].size = uiSizeMDMap*3;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_HIST].st_adr = pFuncCtx->stMAq.stAddrCfg.uiHistMotAddr[0];
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_HIST].size = uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+1);
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_GCAC].st_adr = pFuncCtx->stOsg.uiGcacStatAddr0;
			g_mem_usage[0][pVar->uiEncId].buf_cmd[H26X_MEM_GCAC].size = uiSizeOsgGcac*2;
		}
	}
#endif

	return H26XENC_SUCCESS;
}

void _h265Enc_initEncoderDump(H265ENC_INIT *pInit)
{
	DBG_IND("[KDRV_H265ENC] W = %u, H = %u, DisW = %u, TileEn = %u, buf = (0x%08x, %d), GOP = %u, UsrQp = %u, DB = (%u, %d, %d), Qpoff = (%d, %d), svc = %u, ltr = (%u, %u), SAO = (%u, %u, %u), bitrate = %u, fr = %u, I_QP = (%u, %u, %u), P_QP = (%u, %u, %u), ip_off = %d, sta = %u, FBCEn = %u, Gray = %u, FastSear = %u, HwPad = %u, rotate = %u, d2d = %u, LevelIDC = %u, SEI = %u, MulLayer = %u, VUI = (%u, %u, %u, %u, %u, %u, %u, %u, %u)\r\n",
		(int)pInit->uiWidth,
		(int)pInit->uiHeight,
		(int)pInit->uiDisplayWidth,
		(int)pInit->bTileEn,
		(int)pInit->uiEncBufAddr,
		(int)pInit->uiEncBufSize,
		(int)pInit->uiGopNum,
		(int)pInit->uiUsrQpSize,
		(int)pInit->ucDisableDB,
		(int)pInit->cDBAlpha,
		(int)pInit->cDBBeta,
		(int)pInit->iQpCbOffset,
		(int)pInit->iQpCrOffset,
		(int)pInit->ucSVCLayer,
		(int)pInit->uiLTRInterval,
		(int)pInit->uiLTRPreRef,
		(int)pInit->uiSAO,
		(int)pInit->uiSaoLumaFlag,
		(int)pInit->uiSaoChromaFlag,
		(int)pInit->uiBitRate,
		(int)pInit->uiFrmRate,
		(int)pInit->ucIQP,
		(int)pInit->ucMinIQp,
		(int)pInit->ucMaxIQp,
		(int)pInit->ucPQP,
		(int)pInit->ucMinPQp,
		(int)pInit->ucMaxPQp,
		(int)pInit->iIPQPoffset,
		(int)pInit->uiStaticTime,
		(int)pInit->bFBCEn,
		(int)pInit->bGrayEn,
		(int)pInit->bFastSearchEn,
		(int)pInit->bHwPaddingEn,
		(int)pInit->ucRotate,
		(int)pInit->bD2dEn,
		(int)pInit->ucLevelIdc,
		(int)pInit->ucSEIIdfEn,
		(int)pInit->uiMultiTLayer,
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

	if (h26x_getChipId() == 98520) {
		if (set_tile_config(&TileCfg, pInit->uiWidth, pInit->bD2dEn, pInit->bGdcEn, pInit->eQualityLvl) == H26XENC_FAIL)
			return H26XENC_FAIL;
	} else {
		if (set_tile_config_528(&TileCfg, pInit->uiWidth, pInit->bD2dEn, pInit->bGdcEn, pInit->eQualityLvl) == H26XENC_FAIL)
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
			H265EncRdo sRdoAttr = {1,
				{0, 0,
				0, 0, 0,
				14, 28, 14, 28, 7},
				-3, -3, -3, -3, -4, -4,
				-4, -1, -1, -1, -1, -1,
				-1, -1, -1, 0, 0, 0,
				0,  0,  0};

			// init fro //
			h265Enc_setFroCfg( pVar, &sFroAttr);
			//init var
			h26XEnc_setVarCfg( pVar, &sVarAttr);
			h265Enc_InitRdoCfg( pVar, &sRdoAttr);
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
			H26XEncRmdCfg sRmdCfg = {10, 5, 20, 20, 25, 0};

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
		(int)pInfo->ucSkipFrmMode,
		(int)pInfo->bGetSeqHdrEn,
		(int)pInfo->uiPredBitss,
		(int)pInfo->uiH26XBaseVa,
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
	//pInfo->bSrcCbCrIv = 0;

	/* dump param settings */
	_h265Enc_prepareOnePictureDump(pInfo);

	// check info data correct not ready //
	if (pInfo->bSrcD2DEn) {
		if (pInfo->ucSrcD2DMode == 0) {
			// ISP 1 stripe //
			if (pSeqCfg->ucTileNum > 2) {
				DBG_ERR("codec tile number(%d) not support stripe_mode(%d)\r\n", (int)pSeqCfg->ucTileNum, (int)pInfo->ucSrcD2DMode);
				return H26XENC_FAIL;
			}
			else
				pInfo->ucSrcD2DMode = ((pSeqCfg->bTileEn) && (pSeqCfg->ucTileNum == 2)) ? 2 : 0;
		}
		else if (pInfo->ucSrcD2DMode == 1) {
			// ISP 2 stripe //
			if (pSeqCfg->ucTileNum > 2) {
				DBG_ERR("codec tile number(%d) not support stripe_mode(%d)\r\n", (int)pSeqCfg->ucTileNum, (int)pInfo->ucSrcD2DMode);
				return H26XENC_FAIL;
			}
			else {
				if (pSeqCfg->ucTileNum == 2) {
					if (pSeqCfg->uiTileWidth[0] > pInfo->uiSrcD2DStrpSize[0]) {
						DBG_ERR("codec tile(0) width(%d) > stripe(0) size(%d)\r\n", (int)pSeqCfg->uiTileWidth[0], (int)pInfo->uiSrcD2DStrpSize[0]);
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
					DBG_ERR("codec tile(0) width(%d) > stripe(0) size(%d)\r\n", (int)pSeqCfg->uiTileWidth[0], (int)pInfo->uiSrcD2DStrpSize[0]);
					return H26XENC_FAIL;
				}
				if ( (pSeqCfg->uiTileWidth[0] + pSeqCfg->uiTileWidth[1]) > (pInfo->uiSrcD2DStrpSize[0] + pInfo->uiSrcD2DStrpSize[1])) {
					DBG_ERR("codec tile(0+1) width(%d, %d) > stripe(0+1) size(%d, %d)\r\n",
						(int)pSeqCfg->uiTileWidth[0], (int)pSeqCfg->uiTileWidth[1], (int)pInfo->uiSrcD2DStrpSize[0], (int)pInfo->uiSrcD2DStrpSize[1]);
					return H26XENC_FAIL;
				}
				pInfo->ucSrcD2DMode = 4;
			}
		}
		else {
			return H26XENC_FAIL;
		}
	}
	pInfo->uiBsOutBufSize = ((pInfo->uiBsOutBufSize>>7)<<7);
	// set data to picture config //
	pPicCfg->ePicType = pInfo->ePicType;
	pComnCtx->stVaAddr.uiBsOutAddr = pInfo->uiBsOutBufAddr;

	if (pComnCtx->ucRcMode == 0)
		pPicCfg->ucSliceQP = IS_ISLICE(pInfo->ePicType) ? pSeqCfg->ucInitIQP : pSeqCfg->ucInitPQP;
	else {
		if (0 == pInfo->bSkipFrmEn || IS_ISLICE(pInfo->ePicType)) {
			pPicCfg->ucSliceQP = h26XEnc_getRcQuant(pVar, pPicCfg->ePicType, (pSeqCfg->u32LtInterval > 1 && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0)), pInfo->uiBsOutBufSize);
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
	if (pFuncCtx->stSliceSplit.stCfg.bEnable == 1) {
		h26XEnc_setSliceSplitCfg(pVar, &pFuncCtx->stSliceSplit.stCfg);
	}

	Ret = h265Enc_preparePicCfg( pVdoCtx, pInfo, pVar);
	if(Ret != H26XENC_SUCCESS) {
		DBG_ERR("h265 prepare pic cfg error\r\n");
		return Ret;
	}

	h265Enc_updateRowRcCfg(pVar);

	if (pPicCfg->uiPrePicCnt != pComnCtx->uiPicCnt)
	{
		h265Enc_modifyRefFrm(pVdoCtx, pVar, bRecFBCEn);
	}

	Ret = h265Enc_encSliceHeader(pVdoCtx, pFuncCtx, pComnCtx, pInfo->bGetSeqHdrEn);
	if(Ret != H26XENC_SUCCESS) {
		DBG_ERR("h265 enc slice header error\r\n");
		return Ret;
	}

	Ret = h265Enc_wrapPicCfg(pRegSet, pInfo, pVar);
	if(Ret != H26XENC_SUCCESS) {
		DBG_ERR("h265 wrap pic cfg error\r\n");
		return Ret;
	}

	h26xEnc_wrapGdrCfg(pRegSet, &pFuncCtx->stGdr, pSeqCfg->usLcuHeight, pComnCtx->uiPicCnt, IS_ISLICE(pPicCfg->ePicType));
	h26xEnc_wrapRowRcPicUpdate(pRegSet, &pFuncCtx->stRowRc, pComnCtx->uiPicCnt, IS_PSLICE(pPicCfg->ePicType), pPicCfg->ucSliceQP, pVdoCtx->uiSvcLable, pComnCtx);
#if H26X_KEYP_DISABLE_TNR
	if (h26x_getChipId() == 98528) {
		if (IS_ISLICE(pInfo->ePicType) || (pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0)))
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
	//dump_tmnr(pVar);
	h26x_setChkSumEn(pVdoCtx->bSEIPayloadBsChksum==1);

	return H26XENC_SUCCESS;
}

INT32 h265Enc_updateLtPOC(H265EncSeqCfg *pH265EncSeqCfg, H265EncPicCfg *pH265EncPicCfg)
{
	//update long-term poc
	if (pH265EncSeqCfg->uiLTRpicFlg) {
		//fix cim
		if (pH265EncSeqCfg->uiLTInterval == 0) {
			DBG_ERR("wrong LT interval:%d\r\n", (int)pH265EncSeqCfg->uiLTInterval);
			return H26XENC_FAIL;
		} else {
			if (pH265EncSeqCfg->bEnableLt == 1 && (pH265EncPicCfg->iPoc % pH265EncSeqCfg->uiLTInterval) == 0
	                    && pH265EncSeqCfg->uiLTRPreRef == 1) {
				pH265EncSeqCfg->iPocLT[0] = pH265EncPicCfg->iPoc;  //update refernce frame
			} else if (pH265EncSeqCfg->bEnableLt == 1 && (pH265EncPicCfg->iPoc % pH265EncSeqCfg->uiLTInterval) == 0
			            && pH265EncSeqCfg->uiLTRPreRef == 0 && IS_IDRSLICE(pH265EncPicCfg->ePicType)) {
				pH265EncSeqCfg->iPocLT[0] = pH265EncPicCfg->iPoc;  //update refernce frame
			}
		}
		return H26XENC_SUCCESS;
	}
	DBG_IND("lt pic[0] %d\r\n", (int)(pH265EncSeqCfg->iPocLT[0]));
	return H26XENC_SUCCESS;
}

INT32 h265Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt, UINT32 retri, H265ENC_INFO *pInfo, UINT32 bsout_buf_addr_2)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet    *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;
	H265EncSeqCfg  *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg  *pPicCfg = &pVdoCtx->stPicCfg;
	UINT32 cu16x16_size;
	UINT32 aq_cu_num, aq_cu_size = (pFuncCtx->stAq.stCfg.bAqMode == 1)? 16: (pFuncCtx->stAq.stCfg.ucDepth==0)? 64: (pFuncCtx->stAq.stCfg.ucDepth==1)? 32: 16;

	//fix cim
	if (pVdoCtx->stSeqCfg.uiTotalLCUs == 0 || pVdoCtx->stSeqCfg.uiGopNum == 0) {
		DBG_ERR("wrong total Lcu/Gop number:%d,%d\r\n", (int)pVdoCtx->stSeqCfg.uiTotalLCUs, (int)pVdoCtx->stSeqCfg.uiGopNum);
		return H26XENC_FAIL;
	} else {
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
		else
			h26x_cache_invalidate((UINT32)pRegSet->CHK_REPORT, sizeof(UINT32)*H26X_REPORT_NUMBER);
		//fmem_dcache_sync((void *)pRegSet, sizeof(H26XRegSet), DMA_BIDIRECTIONAL);
		#if 1
		//aq_cu_num = pVdoCtx->stSeqCfg.uiTotalLCUs << (2*pFuncCtx->stAq.stCfg.ucDepth);
		aq_cu_num = ((pVdoCtx->stSeqCfg.usWidth + aq_cu_size-1) / aq_cu_size) * ((pVdoCtx->stSeqCfg.usHeight + aq_cu_size-1) / aq_cu_size);
		if (pPicCfg->uiFrameSkip == 0) {
			pFuncCtx->stAq.stCfg.ucAslog2 = (pRegSet->CHK_REPORT[H26X_SRAQ_ISUM_ACT_LOG] + (aq_cu_num>>1))/aq_cu_num;
		}
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

		if (retri) {
			h26x_cache_invalidate(pInfo->uiBsOutBufAddr, pInfo->uiBsOutBufSize);
			h26x_cache_invalidate(bsout_buf_addr_2, pResult->uiBSLen-pInfo->uiBsOutBufSize);
		} else {
			h26x_cache_invalidate(pComnCtx->stVaAddr.uiBsOutAddr, pResult->uiBSLen);
		}

		h265Enc_updateLtPOC(pSeqCfg, pPicCfg);
		h265Enc_updatePicCnt(pVar);
		if (pFuncCtx->stRowRc.uiPredBits)
			h26XEnc_getRowRcState(pPicCfg->ePicType, pRegSet, &pFuncCtx->stRowRc, pResult->uiSvcLable);
		if (pComnCtx->ucRcMode != 0) {
			if (pPicCfg->uiFrameSkip == 0) {
				h26XEnc_UpdateRc(pVar, pVdoCtx->stPicCfg.ePicType, pResult);
			} else {
				DBG_IND("skip frame\r\n");
			}
		}
		pPicCfg->prev_frm_is_skip[pVdoCtx->eRecIdx] = pPicCfg->uiFrameSkip;

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

UINT32 h265Enc_getEncodeRatio(H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncSeqCfg  *pSeqCfg = &pVdoCtx->stSeqCfg;
	UINT32 result = h26x_getDbg1(0);
	UINT32 lcu_x, lcu_y;
	UINT32 enc_ratio;

	if (pSeqCfg->uiTotalLCUs == 0) {
		DBG_ERR("wrong Lcu numbers:%d\r\n", (int)pSeqCfg->uiTotalLCUs);

		return 0;
	} else {
		lcu_y = (result&0x00ff0000)>>16;
		lcu_x = (result&0xff000000)>>24;
		enc_ratio = (lcu_y * pSeqCfg->usLcuWidth + lcu_x)*100 / pSeqCfg->uiTotalLCUs;
		if (0 == enc_ratio)
			enc_ratio = 1;

		return enc_ratio;
	}
}

INT32 h265Enc_InitRdoCfg(H26XENC_VAR *pVar, H265EncRdo *pRdo)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncRdo *pRdoCfg = &pVdoCtx->stRdo;

	// check info data correct not ready //

	memcpy(pRdoCfg, pRdo, sizeof(H265EncRdo));

	h265Enc_wrapRdo((H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr, pVdoCtx);

	return H26XENC_SUCCESS;
}

INT32 h265Enc_setRdoCfg(H26XENC_VAR *pVar, H265EncRdoCfg *pRdo)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncRdoCfg *pRdoCfg = &pVdoCtx->stRdo.stRdoCfg;

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

void h265Enc_setGopNum(H26XENC_VAR *pVar, UINT32 gop)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;

	pVdoCtx->stSeqCfg.uiGopNum = gop;
}

UINT32 h265Enc_queryMemSize(H26XEncMeminfo *pInfo)
{
	// ( rec_ref(ST_0), svc layer(ST_1(layer 2)), LT) //
	UINT32 uiFrmBufNum = 1 + (pInfo->ucSVCLayer == 2) + (pInfo->uiLTRInterval != 0);
	UINT32 uiTotalBufSize = 0;

	UINT32 uiFrmSize      = SIZE_64X(pInfo->uiWidth) * SIZE_64X(pInfo->uiHeight);
	UINT32 uiSizeSideInfoLof[H26X_MAX_TILE_NUM];
	UINT32 uiSizeSideInfo[H26X_MAX_TILE_NUM];
	UINT32 uiSizeColMVs   = (pInfo->bColMvEn==0)? 0: SIZE_16X((((((pInfo->uiWidth + 63) >> 6) * ((pInfo->uiHeight + 63) >> 6)) << 6) << 2));
	UINT32 uiExtraRecSize = SIZE_64X(pInfo->uiHeight) << 7;

	UINT32 uiSizeBSDMA    = ((((pInfo->uiHeight + 63)>>6)<<1) + 1 + 4) << 2;
	UINT32 uiSizeUsrQpMap = ((pInfo->uiWidth + 63)>>6) * ((pInfo->uiHeight + 63)>>6) * 16 * 2;
	UINT32 uiSizeMDMap    = ((SIZE_512X(pInfo->uiWidth) >> 4) * (SIZE_16X(pInfo->uiHeight) >> 4)) >> 3;
	UINT32 uiSizeOsgGcac  = 256;
	UINT32 uiNaluLength   = 0;
	UINT32 uiSeqHdrSize   = H265ENC_HEADER_MAXSIZE;
	UINT32 uiPicHdrSize   = (((pInfo->uiHeight+63)/64)*20) + uiSeqHdrSize + 128;	// each slice header + sequence header + sei(128) //
	UINT32 uiSizeRcRef    = SIZE_4X(((pInfo->uiHeight + 63) >> 6)) << 4;
	UINT32 uiSizeAPB	  = h26x_getHwRegSize();
	UINT32 uiSizeLLC	  = (H26X_ENC_MODE == 0)? h26x_getHwRegSize() + 64: 0;
	UINT32 uiSizeVdoCtx   = SIZE_32X(sizeof(H265ENC_CTX));
	UINT32 uiSizeFuncCtx  = SIZE_32X(sizeof(H26XFUNC_CTX));
	UINT32 uiSizeComnCtx  = SIZE_32X(sizeof(H26XCOMN_CTX));
	H265EncTileCfg TileCfg;

#if H26X_USE_DIFF_MAQ
	if (pInfo->bCommReconFrmBuf) {
		uiTotalBufSize = uiSizeColMVs*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
							uiSeqHdrSize + uiPicHdrSize + uiSizeUsrQpMap + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;//256 for 16x alignment
	} else {
		uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeMDMap*(H26X_MOTION_BITMAP_NUM+4) +
							uiSeqHdrSize + uiPicHdrSize + uiSizeUsrQpMap + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;//256 for 16x alignment
	}
#else
	if (pInfo->bCommReconFrmBuf) {
		uiTotalBufSize = uiSizeColMVs*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeMDMap*3 +
							uiSeqHdrSize + uiPicHdrSize + uiSizeUsrQpMap + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;//256 for 16x alignment
	} else {
		uiTotalBufSize = (uiFrmSize*3/2 + uiSizeColMVs)*uiFrmBufNum + uiSizeOsgGcac*2 + uiSizeAPB + uiSizeLLC + uiSizeMDMap*3 +
							uiSeqHdrSize + uiPicHdrSize + uiSizeUsrQpMap + uiSizeVdoCtx + uiSizeFuncCtx + uiSizeComnCtx + 256;//256 for 16x alignment
	}
#endif

	if (h26x_getChipId() == 98520) {
		set_tile_config(&TileCfg, pInfo->uiWidth, pInfo->bD2dEn, pInfo->bGdcEn, pInfo->ucQualityLevel);
	} else {
		set_tile_config_528(&TileCfg, pInfo->uiWidth, pInfo->bD2dEn, pInfo->bGdcEn, pInfo->ucQualityLevel);
	}

	if (TileCfg.bTileEn) {
		UINT8 i;
		UINT32 slice_num = 0;
		for (i = 0; i < TileCfg.ucTileNum; i++) {
			slice_num += ((pInfo->uiHeight+63) >> 6);
		}
		uiNaluLength = SIZE_256X(slice_num * 4) + 16;
		if(slice_num % 64 == 0){
			//ic limitaion: 64X need more 256 byte
			uiNaluLength += 256;
		}
		uiTotalBufSize += uiNaluLength;

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
			//(rrc buf)*(i/p)*(rd/wr)
			#if (0 == RRC_BY_FRAME_LEVEL)
			uiTotalBufSize += (uiSizeRcRef << 2);
			#else
			uiTotalBufSize += (uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2));
			#endif
		}
	} else {
		UINT32 slice_num = 0;
		uiSizeSideInfoLof[0] = SIZE_32X((((pInfo->uiWidth+63)>>6) << 3));
		uiSizeSideInfo[0] = uiSizeSideInfoLof[0]*((pInfo->uiHeight+63)>>6);
		slice_num = (pInfo->uiHeight+63) >> 6;
		uiNaluLength = SIZE_256X(slice_num * 4) + 16;
		if(slice_num % 64 == 0){
			//ic limitaion: 64X need more 256 byte
			uiNaluLength += 256;
		}

		uiTotalBufSize += (uiSizeSideInfo[0]*uiFrmBufNum);
		uiTotalBufSize += uiSizeBSDMA;
		uiTotalBufSize += uiNaluLength;
		#if (0 == RRC_BY_FRAME_LEVEL)
		uiTotalBufSize += (uiSizeRcRef << 2);
		#else
		uiTotalBufSize += (uiSizeRcRef*(MAX_RRC_FRAME_LEVEL*2));
		#endif
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

void h265Enc_getRdoCfg(H26XENC_VAR *pVar, H265EncRdoCfg *pRdo)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncRdoCfg *pRdoCfg = &pVdoCtx->stRdo.stRdoCfg;

	// check info data correct not ready //

	memcpy(pRdo, pRdoCfg, sizeof(H265EncRdoCfg));
}

UINT32 h265Enc_queryRecFrmSize(const H26XEncMeminfo *pInfo)
{
	UINT32 uiFrmSize = SIZE_64X(pInfo->uiWidth)*SIZE_64X(pInfo->uiHeight);
	return (uiFrmSize*3/2);
}

#if H26X_SET_PROC_PARAM
int h265Enc_getRowRCStopFactor(void)
{
    return gH265RowRCStopFactor;
}
int h265Enc_setRowRCStopFactor(int value)
{
    if (value > 0)
        gH265RowRCStopFactor = value;
    return 0;
}
#endif

int h265Enc_setLongStartCode(UINT32 lsc)
{
	if (0 == lsc || 1 == lsc)
		gForceLongStartCode = lsc;
	return 0;
}

int h265Enc_setSEIBsChksumen(H26XENC_VAR *pVar, UINT32 enable)
{
	H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;

	pVdoCtx->bSEIPayloadBsChksum = enable;

	return 0;
}
