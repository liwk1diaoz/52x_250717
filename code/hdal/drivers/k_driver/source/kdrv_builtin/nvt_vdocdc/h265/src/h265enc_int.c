/*
    H265ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H265RegSet

    @file       h265enc_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "nvt_vdocdc_dbg.h"

#include "h26x_common.h"

#include "h265enc_int.h"
#include "h265enc_wrap.h"
#include "h26xenc_int.h"
#include "h26xenc_wrap.h"

#define Clip3_JM(min,max,val)   (((val)<(min))?(min):(((val)>(max))?(max):(val)))
#define clip(low, high, val)    ((val)>(high)?(high):((val)<(low)?(low):(val)))

#ifdef __FREERTOS
    int gH265RowRCStopFactor = 5;
#else
    int gH265RowRCStopFactor = 5;
#endif
extern int gRRCNDQPStep;
extern int gRRCNDQPRange;

//UINT8 slice_type_name[4][256] = {"P","B","I","IDR"};
/*
static INT32 __inline log2bin(UINT32 uiValue)
{
	INT32 iN = 0;
	while (uiValue) {
		uiValue >>= 1;
		iN++;
	}
	return iN;
}
*/
static void init_tid(H265ENC_INIT *pInit, H265EncSeqCfg *pSeqCfg)
{
	pSeqCfg->Tid1x = 4;
	pSeqCfg->Tid2x = 3;
	pSeqCfg->Tid4x = 2;
	pSeqCfg->TidLt = 1;

	if (pSeqCfg->bEnableLt == 0) {
		pSeqCfg->TidLt = 0;
		pSeqCfg->Tid4x--;
		pSeqCfg->Tid2x--;
		pSeqCfg->Tid1x--;
	}
	if (pSeqCfg->uiEn4xP == 0) {
		pSeqCfg->Tid4x = 0;
		pSeqCfg->Tid2x--;
		pSeqCfg->Tid1x--;
	}
	if (pSeqCfg->uiEn2xP == 0) {
		pSeqCfg->Tid2x = 0;
		pSeqCfg->Tid1x--;
	}
	if (pSeqCfg->ucSVCLayer == 1) {
		pSeqCfg->Tid1x = 0;
	}

	//inital for rtsp tid
	if (pInit->ucSVCLayer == 0) {
		pSeqCfg->Tid2x = -1;
		pSeqCfg->Tid4x = -1;
	} else if (pInit->ucSVCLayer == 1) {
		pSeqCfg->Tid4x = -1;
	}

	DBG_IND("TemporalId IDR(0) 1X(%d) 2X(%d) 4X(%d) long-term(%d)\r\n",
			(int)pSeqCfg->Tid1x, (int)pSeqCfg->Tid2x, (int)pSeqCfg->Tid4x, (int)pSeqCfg->TidLt);

}
#if 0
static void init_row_rc(H265ENC_INIT *pEncInit, H265EncRowRcCfg *pRowRcCfg)
{
	memset(pRowRcCfg, 0, sizeof(H265EncRowRcCfg));

	pRowRcCfg->uiEnable = 0;

	pRowRcCfg->uiPredWT  = 0;
	pRowRcCfg->uiQPRange = pEncInit->uiRowRcQpDelta;
	pRowRcCfg->uiQPStep  = pEncInit->uiRowRcQpStep;

	pRowRcCfg->uiMinCostTH = 7;

	pRowRcCfg->uiRefCoeff[0] = 8;
	pRowRcCfg->uiRefCoeff[1] = 8;

	pRowRcCfg->iPrevPicBitsBias[0] = 0;
	pRowRcCfg->iPrevPicBitsBias[1] = 0;
}
#endif

//! Initialize encoder configurate
/*!
    Convert pInit to SeqCfg and PicCfg

    @return
        - @b H265ENC_SUCCESS: init success
        - @b H265ENC_FAIL   : init fail
*/
INT32 h265Enc_initCfg(H265ENC_INIT *pInit, H26XENC_VAR *pVar, H265EncTileCfg *pTileCfg)
{
	H265ENC_CTX   *pVdoCtx = pVar->pVdoCtx;
    H26XFUNC_CTX  *pFuncCtx = pVar->pFuncCtx;
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
    H26XEncRowRc  *pRowRc = &pFuncCtx->stRowRc;
    H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	int i, j;

	// sequence config
	pSeqCfg->usWidth		= pInit->uiWidth;
	pSeqCfg->usHeight		= pInit->uiHeight;
	pSeqCfg->usLcuWidth  = SIZE_64X(pSeqCfg->usWidth) / 64;
	pSeqCfg->usLcuHeight = SIZE_64X(pSeqCfg->usHeight) / 64;
	pSeqCfg->uiTotalLCUs = pSeqCfg->usLcuWidth * pSeqCfg->usLcuHeight;

	pSeqCfg->uiDisplayWidth	= pInit->uiDisplayWidth;
	pSeqCfg->uiGopNum		= pInit->uiGopNum;
	pSeqCfg->usLog2MaxFrm   = Clip3_JM(4, 16, (int)(log2bin(66535)));
	pSeqCfg->usLog2MaxPoc   = Clip3_JM(4, 16, (int)(log2bin(4096)));

	pSeqCfg->uiUsrQpSize    = pInit->uiUsrQpSize;
	pPicCfg->iQpCbOffset    = pInit->iQpCbOffset;
	pPicCfg->iQpCrOffset    = pInit->iQpCrOffset;

	//pSeqCfg->uiRowRcEnalbe     = pInit->uiRowRcEnalbe;
	pSeqCfg->ilf_attr.ilf_db_disable        = pInit->ucDisableDB & 0x1;
	pSeqCfg->ilf_attr.ilf_db_en             = ~(pSeqCfg->ilf_attr.ilf_db_disable);
#if CMODEL_VERIFY
    pSeqCfg->ilf_attr.db_control_present    = 0;
#else
	pSeqCfg->ilf_attr.db_control_present    = 1;
#endif
	pSeqCfg->ilf_attr.llf_bata_offset_div2  = pInit->cDBBeta;
	pSeqCfg->ilf_attr.llf_alpha_c0_offset_div2 = pInit->cDBAlpha;
	pSeqCfg->ilf_attr.ilf_db_override_en    = 1;
	pSeqCfg->ilf_attr.ilf_across_slice_en   = ((pInit->ucDisableDB >> 1) & 0x1);
    pSeqCfg->ilf_attr.ilf_across_tile_en    = ((pInit->ucDisableDB >> 2) & 0x1);

	pSeqCfg->ucSVCLayer            = pInit->ucSVCLayer;
	pSeqCfg->uiLTInterval	       = pInit->uiLTRInterval;
	//pSeqCfg->bLTRPreRef		   = pInit->bLTRPreRef;

	if (pInit->ucSVCLayer == 0 &&  pInit->uiLTRInterval == 0) {
		pSeqCfg->ucSVCLayer        = 1;
		pSeqCfg->uiEn4xP           = 0;
		pSeqCfg->uiEn2xP           = 0;
		pSeqCfg->bEnableLt         = 0;
	} else {
		pSeqCfg->ucSVCLayer        = pInit->ucSVCLayer + 2 + (pInit->uiLTRInterval > 0); //svc(2x,4x) + 1x + idr + lt
		pSeqCfg->uiEn4xP           = (pInit->ucSVCLayer > 1);
		pSeqCfg->uiEn2xP           = (pInit->ucSVCLayer > 0);
		pSeqCfg->bEnableLt         = (pInit->uiLTRInterval > 0);
	}
	pPicCfg->uiNthInGop            = 0;
	pSeqCfg->ucNumRefIdxL0         = 1;

	pSeqCfg->uiBitRate		       = pInit->uiBitRate;	// Bit rate (bits per second)
	pSeqCfg->uiFrmRate 		       = pInit->uiFrmRate;	// Frame rate (frames per second * 1000)
	pSeqCfg->ucInitIQP	           = pInit->ucIQP;
	pSeqCfg->ucInitPQP		       = pInit->ucPQP;
    pSeqCfg->ucInitIQP             = clip(pComnCtx->stRc.m_minIQp, pComnCtx->stRc.m_maxIQp, pSeqCfg->ucInitIQP);
    pSeqCfg->ucInitPQP             = clip(pComnCtx->stRc.m_minPQp, pComnCtx->stRc.m_maxPQp, pSeqCfg->ucInitPQP);

	pSeqCfg->bFBCEn			       = pInit->bFBCEn;
	pSeqCfg->bGrayEn		       = pInit->bGrayEn;
	pSeqCfg->bFastSearchEn	       = pInit->bFastSearchEn;
	pSeqCfg->bHwPaddingEn	       = pInit->bHwPaddingEn;
	pSeqCfg->ucRotate		       = pInit->ucRotate;
	pSeqCfg->bD2dEn			       = pInit->bD2dEn;
	pSeqCfg->bColMvEn			   = pInit->bColMvEn;

	pSeqCfg->bVUIEn		           = pInit->bVUIEn;
	pSeqCfg->usSarWidth		       = pInit->usSarWidth;
	pSeqCfg->usSarHeight	       = pInit->usSarHeight;
    pSeqCfg->ucMatrixCoef          = pInit->ucMatrixCoef;
    pSeqCfg->ucTransferCharacteristics = pInit->ucTransferCharacteristics;
    pSeqCfg->ucColourPrimaries     = pInit->ucColourPrimaries;
    pSeqCfg->ucVideoFormat         = pInit->ucVideoFormat;
	pSeqCfg->ucColorRange	       = pInit->ucColorRange;
    pSeqCfg->bTimeingPresentFlag  = pInit->bTimeingPresentFlag;

	// picture config //
	pPicCfg->ePicType		= IDR_SLICE;
	pComnCtx->uiPicCnt = 0;

	for (i = 0; i < FRM_IDX_MAX; i++) {
 		pPicCfg->iRefAndRecPOC[i] = -1;//initial
 		pPicCfg->uiRefAndRecIsLT[i] = 0;
	}

	init_tid(pInit, pSeqCfg);

	if (pInit->ucSVCLayer == 1) {
		pSeqCfg->uiNumStRps      = 2; //number of short-term reference
		pSeqCfg->iRefFrmNum      = 2;
	} else if (pInit->ucSVCLayer == 2) {
		pSeqCfg->uiNumStRps      = 4; //number of short-term reference
		pSeqCfg->iRefFrmNum      = 3;
	} else {
		pSeqCfg->uiNumStRps      = 1;
		pSeqCfg->iRefFrmNum      = 2;
	}
	if (pSeqCfg->bEnableLt) {
		pSeqCfg->uiNumStRps++;
		pSeqCfg->iRefFrmNum++;
	}

	if (pSeqCfg->bEnableLt) {
		pSeqCfg->uiLTRpicFlg     = 1; //enable long-term reference
		pSeqCfg->uiNumLTPic      = 1;
	} else {
		pSeqCfg->uiLTRpicFlg     = 0;
		pSeqCfg->uiNumLTPic      = 0;
	}

	pSeqCfg->uiNumLTRpsSps   = 0;
	//pSeqCfg->uiTempMvpEn     = 1;
	//pSeqCfg->uiSliceTempMvpEn = 1;
	pSeqCfg->uiTempMvpEn     = pSeqCfg->bColMvEn;
	pSeqCfg->uiSliceTempMvpEn = pSeqCfg->bColMvEn;
	/*chroma_format_idc     separate_colour_plane_flag  Chroma format   SubWidthC   SubHeightC
	               1                        0               4:2:0           2          2           */
	pSeqCfg->uiChromaIdc     = 1; //4:2:0
	pSeqCfg->uiSepColorPlaneFlg = 0;
	pSeqCfg->uiChromaArrayType = pSeqCfg->uiChromaIdc; //If separate_colour_plane_flag is equal to 0, ChromaArrayType is set equal to chroma_format_idc.
	if(pInit->uiMultiTLayer == 0)
		pSeqCfg->uiMaxSubLayerMinus1 = 0;
	else
		pSeqCfg->uiMaxSubLayerMinus1 = pSeqCfg->ucSVCLayer - 1;
	pSeqCfg->uiTempIdNestFlg = pSeqCfg->uiMaxSubLayerMinus1 > 0 ? 0 : 1;

	for (i = 0; i < (int)pSeqCfg->uiNumStRps; i++) {
		STRPS *rps = &pSeqCfg->short_term_ref_pic_set[i];
		rps->NumNegativePics = (i == (int)(pSeqCfg->uiNumStRps - 1) && pSeqCfg->bEnableLt) ? 0 : ((i == 2  && pSeqCfg->uiEn4xP) ? 2 : 1);
		for (j = 0; j < (int)rps->NumNegativePics; j++) {
			if (i == 0) {                       //stRpsIdx=0
				rps->DeltaPocS0[j] = -1;
				rps->UsedByCurrPicS0[j] = 1;
			} else if (i == 1) {                //stRpsIdx=1
				rps->DeltaPocS0[j] = -2;
				rps->UsedByCurrPicS0[j] = 1;
			} else if (i == 2) {                //stRpsIdx=2
				if (j == 0) {
					rps->DeltaPocS0[j] = -1;
					rps->UsedByCurrPicS0[j] = 1;
				} else if (j == 1) {
					rps->DeltaPocS0[j] = -3;
					rps->UsedByCurrPicS0[j] = 1;
				}
			} else if (i == 3) {                //stRpsIdx=3
				rps->DeltaPocS0[j] = -4;
				rps->UsedByCurrPicS0[j] = 1;
			}
		}
	}
	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
		pSeqCfg->iPocLT[i]          = -1;
		pSeqCfg->uiUsedbyCurrLTFlag[i] = 0;
	}

	pSeqCfg->u32LtInterval     = pInit->uiLTRInterval;//lt ref interval
	pSeqCfg->uiLTInterval      = (pInit->uiLTRPreRef == 1) ? pSeqCfg->u32LtInterval : pInit->uiGopNum ; // interval for refresh lt ref pic // lt ref nterval or idr interval
	pSeqCfg->uiLTRPreRef       = pInit->uiLTRPreRef;
	pSeqCfg->uilayer_order_cnt = 0;
	pSeqCfg->ucSEIIdfEn        = pInit->ucSEIIdfEn;
	pSeqCfg->bTileEn           = pTileCfg->bTileEn;
	pSeqCfg->eQLevel		   = pInit->eQualityLvl;

	pSeqCfg->bSimpleMergeDis   = 0;
	pSeqCfg->bTu4Dis           = 0;
	pSeqCfg->ucLevelIdc        = pInit->ucLevelIdc;

	pPicCfg->uiDepSlcEn        = 0;
	pPicCfg->uiNumExSlcHdrBit  = 0;
	pPicCfg->uiOutPresFlg      = 0;
	pPicCfg->uiListModPreFlg   = 0;
	pPicCfg->uiCabacInitPreFlg = 0;
	pPicCfg->uiWeiPredFlg      = 0;
	pPicCfg->uiEntroCodSyncEn  = 0;
	pPicCfg->uiSlcSegHdrExtPreFlg = 0;
	pPicCfg->uiPpsSlcChrQpOfsPreFlg = 0;
	pPicCfg->uiCuQpDeltaEnabledFlag = 1; // 1 : sraq or mbqp enable, 0 : others
	pPicCfg->eNxtPicType        = IDR_SLICE;
	pPicCfg->uiPrePicCnt         = 0xffffffff;

	pSeqCfg->sao_attr.sao_en         = pInit->uiSAO;
	pSeqCfg->sao_attr.sao_luma_flag  = pInit->uiSaoLumaFlag;
	pSeqCfg->sao_attr.sao_chroma_flag  = pInit->uiSaoChromaFlag;
	if (pSeqCfg->sao_attr.sao_en) {
		pSeqCfg->sao_attr.sao_merge_Penalty = 4;
		pSeqCfg->sao_attr.sao_boundary = 1;
	} else {
		pSeqCfg->sao_attr.sao_merge_Penalty = 0;
		pSeqCfg->sao_attr.sao_boundary = 0;
	}
	//pPicCfg->sRdoAttr.rdo_cost_bias_SKIP  = 8;
	//pPicCfg->sRdoAttr.rdo_cost_bias_MERGE  = 8;

	pRowRc->ucMinCostTh = 7;
	if (h26x_getChipId() == 98528) {
		pRowRc->bRRCMode = 0;
		pRowRc->ucTileQPStep[0] = 3;
		pRowRc->ucTileQPStep[1] = 3;
		pRowRc->bTileQPRst = 0;
		pRowRc->ucZeroBitMod = 4;
	}
	for (i = 0; i < H26X_MAX_TILE_NUM; i++) {
		#if RRC_BY_FRAME_LEVEL
		for (j = 0; j < MAX_RRC_FRAME_LEVEL; j++)
			pRowRc->usRefFrmCoeff[i][j] = 8;
		#else
		pRowRc->usRefFrmCoeff[i][0] = 8;
		pRowRc->usRefFrmCoeff[i][1] = 8;
		#endif
	}
	pRowRc->ucNDQPStep = (UINT8)gRRCNDQPStep;
	pRowRc->ucNDQPRange = (UINT8)gRRCNDQPRange;
	// Tuba 20181022: add initial value of row rc
	pRowRc->stCfg.ucMinQP[0] = pRowRc->stCfg.ucMinQP[1] = 0;
	pRowRc->stCfg.ucMaxQP[0] = pRowRc->stCfg.ucMaxQP[1] = 51;

	// lineoffset //
	if (pInit->bFBCEn)
	{
		pVdoCtx->uiRecYLineOffset = SIZE_64X(pSeqCfg->usHeight)/4;
		pVdoCtx->uiRecCLineOffset = SIZE_64X(pSeqCfg->usHeight)/8;
	}
	else
	{
		pVdoCtx->uiRecYLineOffset = SIZE_64X(pSeqCfg->usWidth)/4;
		pVdoCtx->uiRecCLineOffset = SIZE_64X(pSeqCfg->usWidth)/4;
	}

    if(pSeqCfg->bTileEn)
    {
        pSeqCfg->ucTileNum = pTileCfg->ucTileNum;

        for (i = 0; i < pSeqCfg->ucTileNum; i++)
            pSeqCfg->uiTileWidth[i] = pTileCfg->uiTileWidth[i];

        for (i = 0; i < pSeqCfg->ucTileNum; i++) {
            pVdoCtx->uiSideInfoLineOffset[i] = SIZE_32X((((pSeqCfg->uiTileWidth[i]+63)>>6) << 3));
            pVdoCtx->uiSizeSideInfo[i] = pVdoCtx->uiSideInfoLineOffset[i] * ((pSeqCfg->usHeight+63)>>6);
        }
    }else{
        pVdoCtx->uiSideInfoLineOffset[0] = SIZE_32X((((pSeqCfg->usWidth+63)/64) << 3));
        pVdoCtx->uiSizeSideInfo[0] = pVdoCtx->uiSideInfoLineOffset[0] * ((pSeqCfg->usHeight+63)>>6);
        for (i = 1; i < H26X_MAX_TILE_NUM; i++) {
            pVdoCtx->uiSideInfoLineOffset[i] = 0;
            pVdoCtx->uiSizeSideInfo[i] = 0;
        }
    }
	pFuncCtx->stSliceSplit.uiNaluNum = 1;
    if (pSeqCfg->bTileEn) {
        pFuncCtx->stSliceSplit.uiNaluNum *= pSeqCfg->ucTileNum;
    }
	pFuncCtx->stSliceSplit.stExe.uiSliceRowNum = pSeqCfg->usLcuHeight;

	pSeqCfg->frm_crop_info.frm_crop_enable = pInit->frm_crop_info.frm_crop_enable;
	pSeqCfg->frm_crop_info.display_w = pInit->frm_crop_info.display_w;
	pSeqCfg->frm_crop_info.display_h = pInit->frm_crop_info.display_h;

    return H26XENC_SUCCESS;
}


INT32 h265Enc_preparePicCfg(H265ENC_CTX *pVdoCtx, H265ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	UINT32 layer_order_idx;
	UINT8  temporal_id;
	int st_idx_no_ref = (pSeqCfg->bEnableLt) ? (int)(pSeqCfg->uiNumStRps - 1) : -1;

	if(pInfo->SdeCfg.bEnable == FALSE) {
		if ((pInfo->uiSrcYLineOffset & 0xf) || (pInfo->uiSrcCLineOffset & 0xf)) {
			DBG_ERR("Error : LineOffset should be 16X ! (%d, %d)\r\n", (int)(pInfo->uiSrcYLineOffset), (int)(pInfo->uiSrcCLineOffset));
			return H26XENC_FAIL;
		}
	}
	if (h26x_getChipId() == 98528) {
		if ((pInfo->uiSrcYLineOffset > H265_MAX_SRC_LOFT_528) || (pInfo->uiSrcCLineOffset > H265_MAX_SRC_LOFT_528)) {
			DBG_ERR("Error : LineOffset > 65536 ! (%d, %d)\r\n", (int)(pInfo->uiSrcYLineOffset), (int)(pInfo->uiSrcCLineOffset));
			return H26XENC_FAIL;
		}
	} else {
		if ((pInfo->uiSrcYLineOffset > H265_MAX_SRC_LOFT) || (pInfo->uiSrcCLineOffset > H265_MAX_SRC_LOFT)) {
			DBG_ERR("Error : LineOffset > 262144 ! (%d, %d)\r\n", (int)(pInfo->uiSrcYLineOffset), (int)(pInfo->uiSrcCLineOffset));
			return H26XENC_FAIL;
		}
	}

	pPicCfg->uiFrameSkip   = (pInfo->bSkipFrmEn && !IS_ISLICE(pPicCfg->ePicType)) ? 1 : 0;

	/* POC */
	pSeqCfg->cPocLsb[0] = 0;
	pSeqCfg->cPocLsb[1] = 0;
    if (IS_IDRSLICE(pPicCfg->ePicType)) {
        pPicCfg->iPoc = 0;   // IDR //
    }
    pSeqCfg->cPocLsb[0] = (UINT32)(pPicCfg->iPoc) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxPoc));

	//update temporal id
	layer_order_idx = 0;
	if (pSeqCfg->ucSVCLayer < 3) {
		temporal_id = (pPicCfg->uiNthInGop + 0) % pSeqCfg->ucSVCLayer;
	} else {
		UINT32 layer_order[4] = {pSeqCfg->Tid1x, pSeqCfg->Tid2x, pSeqCfg->Tid1x, pSeqCfg->Tid4x}; //set temporal layer order
		if (pInfo->ePicType == IDR_SLICE) {
			temporal_id = 0;
			pSeqCfg->uilayer_order_cnt = 0; //reset count
		} else if (pSeqCfg->uiLTRpicFlg == 1 && pSeqCfg->u32LtInterval > 0 && (pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0) { //long-term
			temporal_id = pSeqCfg->TidLt;
			pSeqCfg->uilayer_order_cnt = 0; //reset count
		} else if (pSeqCfg->uiEn2xP == 0) {
			temporal_id = pSeqCfg->Tid1x;
		} else {
			layer_order_idx = (pSeqCfg->uilayer_order_cnt) % (pSeqCfg->uiEn4xP == 1 ? 4 : 2);
			temporal_id = layer_order[layer_order_idx];
			pSeqCfg->uilayer_order_cnt++;
		}
	}
	pPicCfg->uiTemporalId[0] = temporal_id;

	//update short-term rps set
	if ( IS_IDRSLICE(pPicCfg->ePicType)) {
		pSeqCfg->uiSTRpsIdx = 0;//don't care
	} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid1x) {
		STRPS *rps = &pSeqCfg->short_term_ref_pic_set[0];
		//if (pSeqCfg->iPocLT[0] >= 0 && (rps->DeltaPocS0[0] + pPicCfg->iPoc) == pSeqCfg->iPocLT[0]) {
		if (pSeqCfg->iPocLT[0] >= 0 && (rps->DeltaPocS0[0] + pPicCfg->iPoc) == pSeqCfg->iPocLT[0] && 1 != pPicCfg->iPoc) {
			if (st_idx_no_ref < 0) {
				DBG_ERR("err[%d]: st_idx not exist \r\n", (int)(__LINE__));
			}
			pSeqCfg->uiSTRpsIdx = st_idx_no_ref;
		} else if (layer_order_idx == 2 && pSeqCfg->bEnableLt == 0) {
			pSeqCfg->uiSTRpsIdx = 2;
		} else if (layer_order_idx == 2 && pSeqCfg->bEnableLt == 1 && pSeqCfg->uiLTRPreRef == 0) {
			pSeqCfg->uiSTRpsIdx = (((pPicCfg->iPoc + 1) - 4) != pSeqCfg->iPocLT[0] && (((pPicCfg->iPoc + 1) % pSeqCfg->uiLTInterval) != 0)) ? 2 : 0;
		} else if(layer_order_idx == 2 && pSeqCfg->bEnableLt == 1 && pSeqCfg->uiLTRPreRef == 1){
			pSeqCfg->uiSTRpsIdx = ( (((pPicCfg->iPoc + 1 - 4) % pSeqCfg->uiLTInterval) != 0 ) ) ? 2 : 0;
		} else {
			pSeqCfg->uiSTRpsIdx = 0;
		}
	} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid2x) {
		STRPS *rps = &pSeqCfg->short_term_ref_pic_set[1];
		if (pSeqCfg->iPocLT[0] >= 0 && (rps->DeltaPocS0[0] + pPicCfg->iPoc) == pSeqCfg->iPocLT[0]) {
			if (st_idx_no_ref < 0) {
				DBG_ERR("err[%d]: st_idx not exist \r\n", (int)(__LINE__));
			}
			pSeqCfg->uiSTRpsIdx = st_idx_no_ref;
		} else {
			pSeqCfg->uiSTRpsIdx = 1;
		}
	} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->TidLt) {
		if (st_idx_no_ref < 0) {
			DBG_ERR("err[%d]: st_idx not exist \r\n", (int)(__LINE__));
		}
		pSeqCfg->uiSTRpsIdx = st_idx_no_ref;
	} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid4x && pPicCfg->ePicType != IDR_SLICE) {
		STRPS *rps = &pSeqCfg->short_term_ref_pic_set[3];
		if (pSeqCfg->iPocLT[0] >= 0 && (rps->DeltaPocS0[0] + pPicCfg->iPoc) == pSeqCfg->iPocLT[0]) {
			if (st_idx_no_ref < 0) {
				DBG_ERR("err[%d]: st_idx not exist \r\n", (int)(__LINE__));
			}
			pSeqCfg->uiSTRpsIdx = st_idx_no_ref;
		} else {
			pSeqCfg->uiSTRpsIdx = 3;
		}
	} else {
		pSeqCfg->uiSTRpsIdx = 0;
	}

	//update long-term used by curr
	pSeqCfg->uiUsedbyCurrLTFlag[0] = 0;
	if ((int)pSeqCfg->uiSTRpsIdx == st_idx_no_ref) {
		if (pPicCfg->uiTemporalId[0] == pSeqCfg->TidLt) {
			pSeqCfg->uiUsedbyCurrLTFlag[0] = 1;
		} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid2x) {
			if (pSeqCfg->iPocLT[0] == (pPicCfg->iPoc - 2)) {
				pSeqCfg->uiUsedbyCurrLTFlag[0] = 1;
			}
		} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid1x) {
			if (pSeqCfg->iPocLT[0] == (pPicCfg->iPoc - 1)) {
				pSeqCfg->uiUsedbyCurrLTFlag[0] = 1;
			}
		} else if (pPicCfg->uiTemporalId[0] == pSeqCfg->Tid4x) {
			if (pSeqCfg->iPocLT[0] == (pPicCfg->iPoc - 4)) {
				pSeqCfg->uiUsedbyCurrLTFlag[0] = 1;
			}
		} else {
			DBG_ERR("error : no choose lt pic\r\n");
		}
	}

    return H26XENC_SUCCESS;
}

void h265Enc_updatePicCnt(H26XENC_VAR *pVar)
{
	H265ENC_CTX   *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	//pPicCfg->uiPicCnt += 1; // every picture ++
	pPicCfg->uiNthInGop++;
	pPicCfg->iPoc += 1;
	pComnCtx->uiPicCnt++;
}


void h265Enc_modifyRefFrm(H265ENC_CTX *pVdoCtx, H26XENC_VAR *pVar, BOOL bRecFBCEn)
{

	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	//H26XEncAddr   *pAddr   = &pComnCtx->stVaAddr;
 	int i,j;
	UINT32 PreRefPOC,PreRefIsIntra; // To avoid Rec & Ref is the same //
	int iColPOC, iColRefPOC, iCurrPOC, iCurrRefPOC, discalefactor;
	H26XFRM_IDX eIfrmRefIdx={0};

	// generate rec and ref frame index //
	pVdoCtx->eRecIdx	= (pSeqCfg->uiEn2xP && pSeqCfg->ucSVCLayer>1 && pPicCfg->uiTemporalId[0] == pSeqCfg->Tid1x  ) ? FRM_IDX_NON_REF : FRM_IDX_ST_0 ;
	pVdoCtx->eRefIdx = FRM_IDX_ST_0;
	pVdoCtx->uiSvcLable	= (pSeqCfg->uiEn2xP && pSeqCfg->ucSVCLayer>1 && pPicCfg->uiTemporalId[0] == pSeqCfg->Tid1x  ) ? (pSeqCfg->uiEn2xP + pSeqCfg->uiEn4xP) : 0;
	if (pSeqCfg->uiEn4xP) {
		int NthInGop = (pSeqCfg->u32LtInterval) ? ( (pPicCfg->iPoc)  %  (int)pSeqCfg->u32LtInterval ) : pPicCfg->iPoc;
		if ((NthInGop % 4) == 2) {
			pVdoCtx->eRecIdx = FRM_IDX_ST_1;
			pVdoCtx->uiSvcLable = 1;
		}

		if ((NthInGop % 4) == 3) {
			if (pPicCfg->prev_frm_is_skip[pVdoCtx->eRefIdx] == 0) pVdoCtx->eRefIdx = FRM_IDX_ST_1;
			else pVdoCtx->eRefIdx = FRM_IDX_ST_0;
		}
	}
	if (pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0)) {
		pVdoCtx->eRecIdx	= (pSeqCfg->uiLTRPreRef) ? FRM_IDX_LT_0 : ((pPicCfg->iPoc == 0) ? FRM_IDX_LT_0 : FRM_IDX_ST_0);
		pVdoCtx->eRefIdx = FRM_IDX_LT_0;
	}

	if (pPicCfg->uiFrameSkip == 0 && pVdoCtx->eRecIdx < FRM_IDX_LT_0) {
		pPicCfg->uiLastRecIsLT = 0;
	}

	if (pVdoCtx->eRecIdx == FRM_IDX_LT_0) {
		pPicCfg->uiLastRecIsLT = 1;
	}

	if (IS_ISLICE(pPicCfg->ePicType) && pComnCtx->uiPicCnt > 0){
        if (!(pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0))) {
    		//pAddr->eRefIdx = FRM_IDX_ST_0;
    		if (pSeqCfg->uiEn4xP && (pPicCfg->uiLastRec == FRM_IDX_ST_1)) {
				eIfrmRefIdx = pPicCfg->uiLastRec;
    		}
			else {
    			eIfrmRefIdx = pVdoCtx->eRefIdx;
			}
    		pVdoCtx->eRefIdx = pPicCfg->uiLastRec;
        }
	}
	pPicCfg->uiLastRec = pVdoCtx->eRecIdx;

	pPicCfg->uiColRefIsLT = pPicCfg->uiRefAndRecIsLT[pVdoCtx->eRefIdx];
	pPicCfg->uiRefAndRecIsLT[pVdoCtx->eRecIdx] = 0;
	//search lt ref frame
	for (i = 0; i < (int)FRM_IDX_MAX; i++) {
		for (j = 0; j < (int)pSeqCfg->uiNumLTPic; j++) {
			if (pSeqCfg->uiUsedbyCurrLTFlag[j] && pSeqCfg->iPocLT[j] != -1 && pSeqCfg->iPocLT[j] == pPicCfg->iRefAndRecPOC[i]) {

				//set curr Ref frame is LT
				pPicCfg->uiRefAndRecIsLT[pVdoCtx->eRecIdx] = 1;
			}
		}
	}

	PreRefPOC = pPicCfg->iRefAndRecPOC[pVdoCtx->eRefIdx];
	pPicCfg->iRefAndRecPOC[pVdoCtx->eRecIdx] = pPicCfg->iPoc;

	if (IS_IDRSLICE(pPicCfg->ePicType)) {
		for (i = 0; i < FRM_IDX_MAX; i++) {
			if ((pVdoCtx->eRecIdx) != (UINT32)i) {
				pPicCfg->iRefAndRecPOC[i] = -1;//clear all
			}
		}
	}

	//set col ref poc
	iColRefPOC = pPicCfg->iColRefAndRecPOC[pVdoCtx->eRefIdx];
	pPicCfg->iColRefAndRecPOC[pVdoCtx->eRecIdx] = PreRefPOC;

	//set curr frame is intra
	PreRefIsIntra = pPicCfg->uiRefAndRecIsIntra[pVdoCtx->eRefIdx];
	pPicCfg->uiRefAndRecIsIntra[pVdoCtx->eRecIdx] = (pPicCfg->ePicType == I_SLICE || pPicCfg->ePicType == IDR_SLICE);

	//set distScaleFactor
	iCurrPOC = pPicCfg->iPoc;
	iColPOC = iCurrRefPOC = PreRefPOC;
	if (PreRefIsIntra || pPicCfg->uiRefAndRecIsIntra[pVdoCtx->eRecIdx]) {
		discalefactor = 256;
	} else if (pPicCfg->uiRefAndRecIsLT[pVdoCtx->eRecIdx] || pPicCfg->uiColRefIsLT) {
		discalefactor = 256;  //if  ( bIsCurrRefLongTerm ||  bIsColRefLongTerm )
	} else {
		int iDiffPocD = iColPOC - iColRefPOC;
		int iDiffPocB = iCurrPOC - iCurrRefPOC;
		if (iDiffPocD == iDiffPocB) {
			discalefactor = 256;
		} else {
			int iTDB      = clip(-128, 127, iDiffPocB);      //clip3
			int iTDD      = clip(-128, 127, iDiffPocD);      //clip3
			if (iTDD != 0) {
				int iX        = (0x4000 + abs(iTDD / 2)) / iTDD;
				discalefactor = clip(-4096, 4095, (iTDB * iX + 32) >> 6);    //clip3
			} else {
				discalefactor = 256;
			}
		}
	}
	pSeqCfg->uiDistFactor = discalefactor;

    /*
	DBG_DUMP("DistFactor=%d CurrPoc=%d CurrRefPOC=%d ColPoc=%d ColRefPoc=%d\r\n", (int)(discalefactor), (int)(iCurrPOC), (int)(iCurrRefPOC), (int)(iColPOC), (int)(iColRefPOC));
	DBG_DUMP("bIsCurrRefLT=%d bIsColRefLT=%d\r\n", (int)(pAddr->uiRefAndRecIsLT[pVdoCtx->eRecIdx]), (int)(pPicCfg->uiColRefIsLT));
	DBG_DUMP("strpsIdx=%d RefPoc=%d Poc=%d RefIdx=%d, RecIdx=%d TLid=%d\r\n", (int)(pSeqCfg->uiSTRpsIdx), (int)(PreRefPOC), (int)(pPicCfg->iPoc), (int)(pVdoCtx->eRefIdx), (int)(pVdoCtx->eRecIdx), (int)(pPicCfg->uiTemporalId[0]));
	for (i = 0; i < (int)FRM_IDX_MAX; i++) {
		DBG_DUMP("Frame buffer[%d] POC=%d  \r\n", (int)(i), (int)(pAddr->iRefAndRecPOC[i]));
	}
	*/

    if(pSeqCfg->bTileEn)
    {
        UINT32 read_extra, write_extra, recout;
        recout = (pVdoCtx->eRecIdx != FRM_IDX_NON_REF);
        if (pSeqCfg->ucSVCLayer && IS_ISLICE(pPicCfg->ePicType) && pComnCtx->uiPicCnt > 0 && (pSeqCfg->uiGopNum % 2 == 0) && !(pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0))) {
            read_extra = (pPicCfg->uiRecExtraStatus >> eIfrmRefIdx) & 0x1;
            write_extra = (pPicCfg->uiRecExtraStatus >> pVdoCtx->eRecIdx) & 0x1;
            //DBG_IND("(i)recout = %d, r = %d, w = %d\r\n", (int)(recout), (int)(read_extra), (int)(write_extra));
            if(eIfrmRefIdx == pVdoCtx->eRecIdx)
            {
                write_extra = (read_extra == 0);
            }
        }
        else {
            read_extra = (pPicCfg->uiRecExtraStatus >> pVdoCtx->eRefIdx) & 0x1;
            write_extra = (pPicCfg->uiRecExtraStatus >> pVdoCtx->eRecIdx) & 0x1;
            //DBG_IND("recout = %d, r = %d, w = %d\r\n", (int)(recout), (int)(read_extra), (int)(write_extra));
            if(pVdoCtx->eRefIdx == pVdoCtx->eRecIdx)
            {
                write_extra = (read_extra == 0);
            }
        }
        if(recout == 0)
        {
            write_extra = 1;
        }
        if (pPicCfg->uiFrameSkip == 0) {
	        pPicCfg->uiRecExtraEnable = (write_extra << 1) | read_extra;
	        pPicCfg->uiRecExtraStatus &= ~(1 << pVdoCtx->eRecIdx);
	        pPicCfg->uiRecExtraStatus |= (write_extra << pVdoCtx->eRecIdx);
		}
        //DBG_IND("pic(%d) ref(%d) rec(%d) st = 0x%08x, 0x%08x\r\n", pComnCtx->uiPicCnt,
        //    (((pSeqCfg->ucSVCLayer && IS_ISLICE(pPicCfg->ePicType) && pComnCtx->uiPicCnt > 0 && (pSeqCfg->uiGopNum % 2 == 0) && !(pSeqCfg->u32LtInterval && ((pPicCfg->iPoc % pSeqCfg->u32LtInterval) == 0))))? eIfrmRefIdx: pVdoCtx->eRefIdx),
        //    pVdoCtx->eRecIdx,pPicCfg->uiRecExtraStatus,pPicCfg->uiRecExtraEnable);
    }else{
        pPicCfg->uiRecExtraEnable = 0;
        pPicCfg->uiRecExtraStatus = 0;
        //DBG_IND("pic(%d) ref(%d) rec(%d) st = 0x%08x, 0x%08x\r\n", pPicCfg->uiPicCnt,pVdoCtx->eRefIdx,
        //    pVdoCtx->eRecIdx,pPicCfg->uiRecExtraStatus,pPicCfg->uiRecExtraEnable);
    }

    pPicCfg->bFBCEn[pVdoCtx->eRecIdx] = bRecFBCEn;
	pPicCfg->uiPrePicCnt = pComnCtx->uiPicCnt;
}

void h265Enc_updateRowRcCfg( H26XENC_VAR *pVar)
{
    H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
    H26XEncRowRc *pRowRc = &pFuncCtx->stRowRc;
	#if (0 == RRC_BY_FRAME_LEVEL)
	H265ENC_CTX *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	UINT32 is_p_frm = IS_PSLICE(pPicCfg->ePicType);
	#endif
	INT32 iPrevPicBitsBias;

    if(pRowRc->stCfg.bEnable == 0) {
        pRowRc->uiPredBits = 0;
        return ;
    }

	#if (0 == RRC_BY_FRAME_LEVEL)
	iPrevPicBitsBias = pRowRc->iPrevPicBitsBias[is_p_frm];
	#else
	iPrevPicBitsBias = pRowRc->iPrevPicBitsBias[pRowRc->ucFrameLevel];
	#endif

    pRowRc->uiPlannedStop = ((pRowRc->uiPredBits - iPrevPicBitsBias) * gH265RowRCStopFactor / 100) >> 3;
    //pRowRc->uiPlannedStop = ((pRowRc->uiPredBits - iPrevPicBitsBias) * 5 / 100) >> 3;
    pRowRc->uiPlannedTop  = ((pRowRc->uiPredBits - iPrevPicBitsBias) * 1) >> 3;
    pRowRc->uiPlannedBot  = (((pRowRc->uiPredBits - iPrevPicBitsBias) * 97 + 50) / 100) >> 3;

	DBG_IND("uiPlannedStop = %d,%d,%d\r\n", (int)(pRowRc->uiPlannedStop), (int)(pRowRc->uiPlannedTop), (int)(pRowRc->uiPlannedBot));
}

