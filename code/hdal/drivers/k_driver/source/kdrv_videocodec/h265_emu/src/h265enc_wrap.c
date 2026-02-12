/*
    H.26x wrapper operation.

    @file       h265enc_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"
#include "h26xenc_int.h"

#include "h265enc_wrap.h"
//#include "h26xenc_rate_control.h"

#define FIX_POINT_LAMBDA    1

#define clip(low, high, val)    ((val)>(high)?(high):((val)<(low)?(low):(val)))
//#define abs(a)   ((a > 0)? a : -a)


void h265Enc_wrapSeqCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx, UINT32 uiPaBsdmaAddr)
{
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	UINT32 pps_init_qp, log2_mincu_size;
	UINT32 bias_plane = 0;//15
	UINT32 bias_dc = 0;//15

	save_to_reg(&pRegSet->BSDMA_CMD_BUF_ADDR, uiPaBsdmaAddr, 0, 32);

	save_to_reg(&pRegSet->FUNC_CFG[0], 0, 0, 1);	// codec type //
	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 4, 1);	// ae_en //
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bFastSearchEn, 5, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->ucRotate, 9, 2);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bGrayEn, 11, 2);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bHwPaddingEn, 18, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 15, 1); // nal length

	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->uiTempMvpEn, 0, 1);   // temporoal_mvp_enable
	save_to_reg(&pRegSet->FUNC_CFG[1], 1, 1, 1);    // g_fastIntra-1
	save_to_reg(&pRegSet->FUNC_CFG[1], 1, 5, 1);    // strong filter flag
	save_to_reg(&pRegSet->FUNC_CFG[1], 6, 16,3);    // temporoal_mvp_enable
	save_to_reg(&pRegSet->FUNC_CFG[1], 1, 2, 1);    // CABAC_CLEAN_FracBits
	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->bSimpleMergeDis, 20,1);    // simple merge disable
	save_to_reg(&pRegSet->FUNC_CFG[1], 0, 21,1);    // AE 32B burst
	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->bTu4Dis, 31,1);    // tu4 disable
	save_to_reg(&pRegSet->TIMEOUT_CNT_MAX, 0x25800, 0, 24);

    // 520
    // 1. search range: 0 (luma: +-28, chroma: +-16
    //      max frame width: 6656 for tile mode, 1536 for non-tile mode
    // 2. search range: 1 (luma: +-36, chroma: +-20 for tile mode)
    //                    (luma: +-20, chroma: +-12 for non-tile mode)
    //      max frame width: 4736 for tile mode, 2048 for non-tile mode
    // 528
    // 1. search range0: 0 (luma: +-28, chroma: +-16
    //      max frame width: 11776 for tile mode, 2560 for non-tile mode
    // 2. search range0: 1 (luma: +-36, chroma: +-20 for tile mode)
    //                     (luma: +-20, chroma: +-12 for non-tile mode)
    //      max frame width: 9856 for tile mode, 4096 for non-tile mode
    // 3. search range1: 0 (depend on search range0 settings)
    // 4. search range1: 1 (luma: +-52, chroma: +-28 for tile mode)
    //                     (luma: +-36, chroma: +-20 for non-tile mode)
    //      max frame width: 6656 for tile mode, 2176 for non-tile mode

    if (pSeqCfg->bTileEn) {
        if (pSeqCfg->bD2dEn) {
            save_to_reg(&pRegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
        } else {
            if (h26x_getHwVersion() == HW_VERSION_520) {
                if (pSeqCfg->usWidth <= H26X_SR36_MAX_W)
                    save_to_reg(&pRegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
                else
                    save_to_reg(&pRegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
            } else {
                save_to_reg(&pRegSet->FUNC_CFG[1], 1, 22, 1);  // flexible search range enable //
            }
        }
    } else {
        if (h26x_getHwVersion() == HW_VERSION_520) {
            if (pSeqCfg->usWidth <= H26X_MAX_W_WITHOUT_TILE_V36)
                save_to_reg(&pRegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
            else
                save_to_reg(&pRegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
        } else {
            if (pSeqCfg->usWidth <= H26X_MAX_W_WITHOUT_TILE_V36_528)
                save_to_reg(&pRegSet->FUNC_CFG[1], 1, 22, 1);  // flexible search range enable //
            else if (pSeqCfg->usWidth <= H26X_MAX_W_WITHOUT_TILE_V28_528)
                save_to_reg(&pRegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
            else
                save_to_reg(&pRegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
        }
    }

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pVdoCtx->uiRecYLineOffset,   0, 16);
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pVdoCtx->uiRecCLineOffset,  16, 16);

	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET, (pVdoCtx->uiSideInfoLineOffset[0])>>2, 0, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET, (pVdoCtx->uiSideInfoLineOffset[1])>>2, 16, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET2, (pVdoCtx->uiSideInfoLineOffset[2])>>2, 0, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET2, (pVdoCtx->uiSideInfoLineOffset[3])>>2, 16, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET3, (pVdoCtx->uiSideInfoLineOffset[4])>>2, 0, 16);

	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->uiDisplayWidth,	 0, 16);
	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->usHeight,  16, 16);

	save_to_reg(&pRegSet->SEQ_CFG[1], pSeqCfg->usWidth,       0, 16);
	save_to_reg(&pRegSet->SEQ_CFG[1], SIZE_16X(pSeqCfg->usHeight), 16, 16);

    if (pSeqCfg->bTileEn) {
    	save_to_reg(&pRegSet->TILE_CFG[0],  pSeqCfg->ucTileNum-1, 0, 3);
    }
	save_to_reg(&pRegSet->TILE_CFG[1],  pSeqCfg->uiTileWidth[0], 0, 16);
	save_to_reg(&pRegSet->TILE_CFG[1],  pSeqCfg->uiTileWidth[1], 16, 16);
	save_to_reg(&pRegSet->TILE_CFG[2],  pSeqCfg->uiTileWidth[2], 0, 16);
	save_to_reg(&pRegSet->TILE_CFG[2],  pSeqCfg->uiTileWidth[3], 16, 16);

	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->ilf_attr.ilf_db_en, 0, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->ilf_attr.ilf_across_slice_en, 2, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->ilf_attr.llf_bata_offset_div2, 4, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->ilf_attr.llf_alpha_c0_offset_div2, 8, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->ilf_attr.ilf_across_tile_en, 12, 1);

	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_en, 16, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_luma_flag, 17, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_chroma_flag, 18, 1);
    save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_en, 20, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_merge_Penalty, 24, 4);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->sao_attr.sao_boundary, 21, 1);

	pps_init_qp = 0;
	save_to_reg(&pRegSet->AEAD_CFG, 0,  0, 1);	// SPS delta_pic_order_always_zero_flag
	save_to_reg(&pRegSet->AEAD_CFG, 0,  1, 1);	// PPS pic_order_present_flag
	save_to_reg(&pRegSet->AEAD_CFG, 0,  2, 1);	// PPS redundant_pic_cnt_present_flag
	save_to_reg(&pRegSet->AEAD_CFG, pps_init_qp,  3, 7);	// PPS init_qp_minus26
	#if 0
	//MinCuDQPSize = 64x64, bit[29:28] = 2;
	//MinCuDQPSize = 32x32, bit[29:28] = 1;
	//MinCuDQPSize = 16x16, bit[29:28] = 0;
	log2_mincu_size = (pSeqCfg->uiUsrQpSize == 0) ? 2 : 0;
    #else
    log2_mincu_size = pSeqCfg->uiUsrQpSize;
    #endif
	save_to_reg(&pRegSet->AEAD_CFG, log2_mincu_size,  24, 2);	// log2_mincu_size
    save_to_reg(&pRegSet->AEAD_CFG, pPicCfg->uiCuQpDeltaEnabledFlag, 26, 1);   // cu_qp_delta_en

	save_to_reg(&pRegSet->GDR_CFG[0], pSeqCfg->usLcuHeight, 16, 16);

	// roi : set ROI maximum window number //
	save_to_reg(&pRegSet->ROI_CFG[0], 10, 16, 4);
	// rrc : enable row rc calualte //
	//save_to_reg(&pRegSet->RRC_CFG[0], 1, 1, 1);
	save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);	// calculate rrc, //
    save_to_reg(&pRegSet->RMD_CFG, bias_plane, 0, 4);
    save_to_reg(&pRegSet->RMD_CFG, bias_dc, 4, 4);

	if (1) {
		//--ime_left_amvp_mod=6 --IMEScale64C=14 --IMEScale64P=27 --IMEScale32C=14 --IMEScale32P=27 --IMEScale16C=7
		//2017.7.21 : Fnadi : modify ime_left_amvp_mod,ime_scale_64p,ime_scale_32p for bitrate
		UINT32 amvp_mode = 0;
		UINT32 ime_scale_64c = 14, ime_scale_64p = 28, ime_scale_32c = 14, ime_scale_32p = 28, ime_scale_16c = 7;
		save_to_reg(&pRegSet->IME_CFG, (amvp_mode == 6), 0, 1);
		save_to_reg(&pRegSet->IME_CFG, ime_scale_64c, 1, 5);
		save_to_reg(&pRegSet->IME_CFG, ime_scale_64p, 6, 5);
		save_to_reg(&pRegSet->IME_CFG, ime_scale_32c, 11, 5);
		save_to_reg(&pRegSet->IME_CFG, ime_scale_32p, 16, 5);
		save_to_reg(&pRegSet->IME_CFG, ime_scale_16c, 21, 5);
	}

}

#if FIX_POINT_LAMBDA
#define LAMBDA_BASE 0x80000
#define WEIGHT_BASE 0x20000
static const UINT32 gLambdaTblFix[2][52] = {
    {
        18678,      23533,      29649,      37356,      47065,
        59298,      74711,      94130,      118596,     149422,
        188260,     237193,     298844,     376520,     474386,
        597688,     753040,     948771,     1195377,    1506080,
        1897542,    2390753,    3012160,    3795084,    4781507,
        6024321,    7590169,    9563013,    12048642,   15180337,
        19126026,   24097283,   30360674,   38252052,   48194566,
        60721348,   76504105,   96389132,   121442697,  153008210,
        192778264,  242885393,  306016420,  385556529,  485770787,
        612032840,  771113058,  971541574,  1224065679, 1542226116,
        1943083147U, 2448131359U
    },
    {
        17993,      22670,      28562,      35986,      45339,
        57124,      71972,      90679,      114248,     143943,
        181357,     228496,     287887,     362714,     456991,
        575773,     725429,     913983,     1151546,    1450857,
        1827966,    2303092,    2901715,    3655931,    4606185,
        5803429,    7311862,    9212369,    11606858,   14623725,
        18424739,   23213716,   29247449,   36849477,   46427432,
        58494899,   73698954,   92854864,   116989798,  147397909,
        185709728,  233979596,  294795818,  371419456,  467959191,
        589591636,  742838912,  935918383,  1179183271, 1485677825,
        1871836765U, 2358366542U,
     }
};
static const UINT32 gWeightCFix[29] = {
    8192,     10321,    13004,    16384,    20643,
    26008,    32768,    41285,    52016,    65536,
    82570,    104032,   131072,   165140,   208064,
    262144,   330281,   416128,   524288,   660561,
    832255,   1048576,  1321123,  1664511,  2097152,
    2642246,  3329021,  4194304,  5284492,
};
#else
double uiLambdaTbl[2][52] = {
	{
		//I frame
		0.03562500, 0.04488469, 0.05655116, 0.07125000,
		0.08976937, 0.11310232, 0.14250000, 0.17953875,
		0.22620465, 0.28500000, 0.35907750, 0.45240930,
		0.57000000, 0.71815500, 0.90481860, 1.14000000,
		1.43631000, 1.80963720, 2.28000000, 2.87261999,
		3.61927440, 4.56000000, 5.74523999, 7.23854880,
		9.12000000, 11.49047998, 14.47709759, 18.24000000,
		22.98095995, 28.95419519, 36.48000000, 45.96191990,
		57.90839038, 72.96000000, 91.92383980, 115.81678075,
		145.92000000, 183.84767960, 231.63356150, 291.84000000,
		367.69535920, 463.26712301, 583.68000000, 735.39071840,
		926.53424601, 1167.36000000, 1470.78143681, 1853.06849203,
		2334.72000000, 2941.56287361, 3706.13698405, 4669.44000000
	},
	{
		//P frame
		0.03431875, 0.04323892, 0.05447762, 0.06863750,
		0.08647783, 0.10895524, 0.13727500, 0.17295566,
		0.21791048, 0.27455000, 0.34591132, 0.43582096,
		0.54910000, 0.69182265, 0.87164192, 1.09820000,
		1.38364530, 1.74328384, 2.19640000, 2.76729059,
		3.48656767, 4.39280000, 5.53458119, 6.97313534,
		8.78560000, 11.06916238, 13.94627068, 17.57120000,
		22.13832475, 27.89254136, 35.14240000, 44.27664950,
		55.78508273, 70.28480000, 88.55329901, 111.57016546,
		140.56960000, 177.10659802, 223.14033091, 281.13920000,
		354.21319603, 446.28066183, 562.27840000, 708.42639206,
		892.56132366, 1124.55680000, 1416.85278412, 1785.12264732,
		2249.11360000, 2833.70556824, 3570.24529464, 4498.22720000,
	}
};

double g_WeightC[12 + 1 + 16] = { //-12 ~ +16 // = 29
	0.06250000, 0.07874507, 0.09921257, 0.12500000,
	0.15749013, 0.19842513, 0.25000000, 0.31498026,
	0.39685026, 0.50000000, 0.62996052, 0.79370053,
	1.00000000, 1.25992105, 1.58740105, 2.00000000,
	2.51984210, 3.17480210, 4.00000000, 5.03968420,
	6.34960421, 8.00000000, 10.07936840, 12.69920842,
	16.00000000, 20.15873680, 25.39841683, 32.00000000,
	40.31747360
};
#endif

const UINT8 g_aucChromaScale[58] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32,
	33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44,
	45, 46, 47, 48, 49, 50, 51
};

#if FIX_POINT_LAMBDA
static void set_lambda(H26XENC_VAR *pVar, H26XRegSet *pRegSet)
{
	H265ENC_CTX  *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	//H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	INT32 qpc;
    UINT32 weight;
    UINT32 lambda_cb, lambda_cr;

	// Cb
	qpc = clip(0, 57, (INT32)pPicCfg->ucSliceQP + pPicCfg->iQpCbOffset);
	weight = gWeightCFix[pPicCfg->ucSliceQP - g_aucChromaScale[qpc] + 12]; // takes into account of the chroma qp mapping and chroma qp Offset
	lambda_cb = gLambdaTblFix[IS_PSLICE(pPicCfg->ePicType)][pPicCfg->ucSliceQP];
	lambda_cb = lambda_cb / weight + 2;
	lambda_cb = lambda_cb / 4;
	save_to_reg(&pRegSet->ILF_CFG[1], (INT32)lambda_cb, 0, 14);

	// Cr
	qpc = clip((INT32)0, (INT32)57, (INT32)((INT32)pPicCfg->ucSliceQP + pPicCfg->iQpCrOffset));
	weight = gWeightCFix[pPicCfg->ucSliceQP - g_aucChromaScale[qpc] + 12]; // takes into account of the chroma qp mapping and chroma qp Offset
	lambda_cr = gLambdaTblFix[IS_PSLICE(pPicCfg->ePicType)][pPicCfg->ucSliceQP];
	lambda_cr = lambda_cr / weight + 2;
	lambda_cr = lambda_cr / 4;
	save_to_reg(&pRegSet->ILF_CFG[1], (INT32)lambda_cr, 16, 14);
}
#else
static void set_lambda(H26XENC_VAR *pVar, H26XRegSet *pRegSet)
{
	H265ENC_CTX  *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	//H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	INT32 qpc;
	double weight = 1.0;
	double lambda_cb, lambda_cr;

	// Cb
	qpc = clip(0, 57, (INT32)pPicCfg->ucSliceQP + pPicCfg->iQpCbOffset);
	weight = g_WeightC[pPicCfg->ucSliceQP - g_aucChromaScale[qpc] + 12]; // takes into account of the chroma qp mapping and chroma qp Offset
	lambda_cb = (double)uiLambdaTbl[IS_PSLICE(pPicCfg->ePicType)][pPicCfg->ucSliceQP];
	lambda_cb = lambda_cb / weight;
	lambda_cb = lambda_cb + 0.5;
	save_to_reg(&pRegSet->ILF_CFG[1], (INT32)lambda_cb, 0, 14);

	// Cr
	qpc = clip((INT32)0, (INT32)57, (INT32)((INT32)pPicCfg->ucSliceQP + pPicCfg->iQpCrOffset));
	weight = g_WeightC[pPicCfg->ucSliceQP - g_aucChromaScale[qpc] + 12]; // takes into account of the chroma qp mapping and chroma qp Offset
	lambda_cr = (double)uiLambdaTbl[IS_PSLICE(pPicCfg->ePicType)][pPicCfg->ucSliceQP];
	lambda_cr = lambda_cr / weight;
	lambda_cr = lambda_cr + 0.5;
	save_to_reg(&pRegSet->ILF_CFG[1], (INT32)lambda_cr, 16, 14);
}
#endif

INT32 h265Enc_wrapPicCfg(H26XRegSet *pRegSet, H265ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H265ENC_CTX  *pVdoCtx  = (H265ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	H26XEncAddr   *pAddr   = &pComnCtx->stVaAddr;
    H26XTileSet *pRegSetTile = (H26XTileSet*) &pRegSet->TILE_CFG[0];
	
    int dime_dist_scale = 1;
    UINT32 uiSideInfoSize = 0;
    UINT8 i;

    // for CIM coverity
    if (pVdoCtx->eRecIdx >= FRM_IDX_MAX || pVdoCtx->eRefIdx >= FRM_IDX_MAX ) {
        //print err msg at h265Enc_prepareOnePicture
        return H26XENC_FAIL;
    }

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->SRC_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcYAddr), 0, 32);
	save_to_reg(&pRegSet->SRC_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcCAddr), 0, 32);
	save_to_reg(&pRegSet->REC_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REC_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRefIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRefIdx]), 0, 32);
/*
    DBG_DUMP("SRC_Y_ADDR = 0x%08x\r\n", pRegSet->SRC_Y_ADDR);
    DBG_DUMP("SRC_C_ADDR = 0x%08x\r\n", pRegSet->SRC_C_ADDR);
    DBG_DUMP("eRecIdx = %d\r\n", (int)(pVdoCtx->eRecIdx));
    DBG_DUMP("eRefIdx = %d\r\n", (int)(pVdoCtx->eRefIdx));
    DBG_DUMP("REC_Y_ADDR = 0x%08x\r\n", pRegSet->REC_Y_ADDR);
    DBG_DUMP("REC_C_ADDR = 0x%08x\r\n", pRegSet->REC_C_ADDR);
    DBG_DUMP("REF_Y_ADDR = 0x%08x\r\n", pRegSet->REF_Y_ADDR);
    DBG_DUMP("REF_C_ADDR = 0x%08x\r\n", pRegSet->REF_C_ADDR);
*/
	if (pInfo->bSrcOutEn)
	{
		save_to_reg(&pRegSet->TNR_OUT_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutYAddr), 0, 32);
		save_to_reg(&pRegSet->TNR_OUT_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutCAddr), 0, 32);

		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutYLineOffset/4,  0, 16);
		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutCLineOffset/4, 16, 16);
	}
	save_to_reg(&pRegSet->COL_MVS_WR_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->COL_MVS_RD_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[pVdoCtx->eRefIdx]), 0, 32);

    save_to_reg(&pRegSet->SIDE_INFO_WR_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRecIdx]), 0, 32);
    save_to_reg(&pRegSet->SIDE_INFO_RD_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRefIdx]), 0, 32);

	save_to_reg(&pRegSet->TILE_CFG[0], pPicCfg->uiRecExtraEnable & 0x1, 8, 1);
	save_to_reg(&pRegSet->TILE_CFG[0], (pPicCfg->uiRecExtraEnable >> 1) & 0x1, 4, 1);

    if(pPicCfg->uiRecExtraEnable & 0x1)
    {
        for (i = 0; i < pSeqCfg->ucTileNum-1; i++) {
            save_to_reg(&pRegSetTile->TILE_EXTRA_RD_Y_ADDR[i], 
                        h26x_getPhyAddr(pAddr->uiTileExtraY[i][pVdoCtx->eRefIdx]), 0, 32);
            save_to_reg(&pRegSetTile->TILE_EXTRA_RD_C_ADDR[i], 
                        h26x_getPhyAddr(pAddr->uiTileExtraC[i][pVdoCtx->eRefIdx]), 0, 32);
        }

        save_to_reg(&pRegSet->SIDE_INFO_RD_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[1][pVdoCtx->eRefIdx]), 0, 32);
    }
    if (pPicCfg->uiRecExtraEnable & 0x2){
        for (i = 0; i < pSeqCfg->ucTileNum-1; i++) {
            save_to_reg(&pRegSetTile->TILE_EXTRA_WR_Y_ADDR[i],
                        h26x_getPhyAddr(pAddr->uiTileExtraY[i][pVdoCtx->eRecIdx]), 0, 32);
            save_to_reg(&pRegSetTile->TILE_EXTRA_WR_C_ADDR[i],
                        h26x_getPhyAddr(pAddr->uiTileExtraC[i][pVdoCtx->eRecIdx]), 0, 32);
        }

        save_to_reg(&pRegSet->SIDE_INFO_WR_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[1][pVdoCtx->eRecIdx]), 0, 32);
    }
    for (i = 0; i < pSeqCfg->ucTileNum-1; i++) {
        uiSideInfoSize += pVdoCtx->uiSizeSideInfo[i];
        save_to_reg(&pRegSetTile->TILE_SIDE_INFO_WR_ADDR[i], 
                    (pRegSet->SIDE_INFO_WR_ADDR_0 + uiSideInfoSize), 0, 32);
        save_to_reg(&pRegSetTile->TILE_SIDE_INFO_RD_ADDR[i], 
                    (pRegSet->SIDE_INFO_RD_ADDR_0 + uiSideInfoSize), 0, 32);
    }

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->RRC_WR_ADDR, h26x_getPhyAddr(pAddr->uiRRC[0][0]), 0, 32);
		save_to_reg(&pRegSet->RRC_RD_ADDR, h26x_getPhyAddr(pAddr->uiRRC[0][1]), 0, 32);
		H26X_SWAP(pAddr->uiRRC[0][0], pAddr->uiRRC[0][1], UINT32);
	}
	else
	{
		save_to_reg(&pRegSet->RRC_WR_ADDR, h26x_getPhyAddr(pAddr->uiRRC[1][0]), 0, 32);
		save_to_reg(&pRegSet->RRC_RD_ADDR, h26x_getPhyAddr(pAddr->uiRRC[1][1]), 0, 32);
		H26X_SWAP(pAddr->uiRRC[1][0], pAddr->uiRRC[1][1], UINT32);
	}
	save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr(pAddr->uiNaluLen), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_ADDR, h26x_getPhyAddr(pInfo->uiBsOutBufAddr), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, pInfo->uiBsOutBufSize, 0, 32);
	save_to_reg(&pRegSet->NAL_HDR_LEN_TOTAL_LEN, pPicCfg->uiPicHdrLen, 0, 32);
	
	if (pInfo->SdeCfg.bEnable) {
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 2, 1); 	// bSrcYDeCmpsEn
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 3, 1);	// bSrcCDeCmpsEn
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->SdeCfg.uiYLofst/4,  0, 16);
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->SdeCfg.uiCLofst/4, 16, 16);
	}
	else {
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 2, 1); 	// bSrcYDeCmpsEn
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 3, 1);	// bSrcCDeCmpsEn
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcYLineOffset/4,  0, 16);
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcCLineOffset/4, 16, 16);
	}
	
	save_to_reg(&pRegSet->FUNC_CFG[0], pPicCfg->uiFrameSkip, 6, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSrcOutMode, 16, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcCbCrIv,   17, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcD2DEn,    20, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSrcD2DMode, 21, 3);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSkipFrmMode,24, 1);

	save_to_reg(&pRegSet->FUNC_CFG[0], pPicCfg->bFBCEn[pVdoCtx->eRecIdx], 13, 1); //REC_FBC_EN
    save_to_reg(&pRegSet->FUNC_CFG[0], (pComnCtx->uiLastHwInt == H26X_FINISH_INT) ? pPicCfg->bFBCEn[pVdoCtx->eRefIdx] : 0, 19, 1); //REF_FBC_EN

	save_to_reg(&pRegSet->ILF_CFG[0], (pPicCfg->uiFrameSkip) ? 0 : pSeqCfg->ilf_attr.ilf_db_en, 0, 1); // dis_loop_filter_idc
	save_to_reg(&pRegSet->ILF_CFG[0], (pPicCfg->uiFrameSkip) ? 0 : pSeqCfg->sao_attr.sao_en, 16, 1);// sao_en

    save_to_reg(&pRegSet->DSF_CFG, pSeqCfg->uiDistFactor, 0, 13);
    save_to_reg(&pRegSet->DSF_CFG, dime_dist_scale, 20, 1);
	#if 0
    //96510 not support ( bIsCurrRefLongTerm != bIsColRefLongTerm ) case
    if (pVaAddr->uiRefAndRecIsLT[pVdoCtx->eRecIdx] != pPicCfg->uiColRefIsLT) {
        save_to_reg(&pRegSet->DSF_CFG, 0, 16, 1);
        save_to_reg(&pRegSet->DSF_CFG, 0, 17, 1);
    } else
    #endif
    {
        save_to_reg(&pRegSet->DSF_CFG, pPicCfg->uiRefAndRecIsLT[pVdoCtx->eRecIdx], 16, 1);
        save_to_reg(&pRegSet->DSF_CFG, pPicCfg->uiColRefIsLT, 17, 1);
    }
    //DBG_DUMP("DSF_CFG = 0x%08x, 16 = %d %d\r\n", pRegSet->DSF_CFG,pVaAddr->uiRefAndRecIsLT[pVdoCtx->eRecIdx],pPicCfg->uiColRefIsLT);
    //DBG_DUMP("eRecIdx = %d %d\r\n", (int)(pVdoCtx->eRefIdx), (int)(pVdoCtx->eRecIdx));

	save_to_reg(&pRegSet->PIC_CFG, IS_PSLICE(pPicCfg->ePicType), 0, 1);
	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->uiSliceTempMvpEn, 0, 1);   // temporoal_mvp_enable
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 4, 1);	// REC_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 5, 1);	// COL_MVS_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 6, 1);	// SIDEINFO_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, pInfo->bSrcOutEn, 7, 1);
	if (pSeqCfg->bTileEn)
		save_to_reg(&pRegSet->PIC_CFG, pFuncCtx->stSliceSplit.stCfg.uiSliceRowNum, 16, 16);
    if(1){
        save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx == pVdoCtx->eRefIdx), 8, 1);
    }else{
        save_to_reg(&pRegSet->PIC_CFG, 0, 8, 1);
    }

    //DBG_DUMP("PIC_CFG = 0x%08x\r\n", pRegSet->PIC_CFG);

	save_to_reg(&pRegSet->QP_CFG[0], pPicCfg->ucSliceQP, 0, 6);
	save_to_reg(&pRegSet->QP_CFG[0], pPicCfg->iQpCbOffset,8, 6);
	save_to_reg(&pRegSet->QP_CFG[0], pPicCfg->iQpCrOffset, 16, 6);

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_minIQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_maxIQp, 8, 6);
	}
	else
	{
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_minPQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_maxPQp, 8, 6);
	}

	if (pFuncCtx->stAq.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->SRAQ_CFG[0], (IS_ISLICE(pPicCfg->ePicType)) ? pFuncCtx->stAq.stCfg.ucIStr : pFuncCtx->stAq.stCfg.ucPStr, 12, 4);
		save_to_reg(&pRegSet->SRAQ_CFG[1], pFuncCtx->stAq.stCfg.ucAslog2, 0, 6);
	}

	if (pFuncCtx->stLpm.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->LPM_CFG, (IS_ISLICE(pPicCfg->ePicType)) ? 0 : pFuncCtx->stLpm.stCfg.ucRdoStopEn, 12, 2);
	}
	//sao_rdo_chroma_lambda
	set_lambda(pVar, pRegSet);

	save_to_reg(&pRegSet->OSG_CFG[0x484/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr0), 0, 32);
	save_to_reg(&pRegSet->OSG_CFG[0x480/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr1), 0, 32);

    return H26XENC_SUCCESS;
}

void h265Enc_wrapRdoCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx)
{
	H265EncRdoCfg *pRdoCfg = &pVdoCtx->stRdoCfg;

	save_to_reg(&pRegSet->RDO_CFG[0], 1, 0, 1);	// rdo enable //

    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_32L & 0x1f, 2 ,5);//Intra Mode  TU 32 Luma   Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_16L & 0x1f, 7 ,5);//Intra Mode  TU 16 Luma   Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_08L & 0x1f, 12 ,5);//Intra Mode TU  8 Luma   Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_04L & 0x1f, 17 ,5);//Intra Mode TU  4 Luma   Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_16C & 0x1f, 22 ,5);//Intra Mode TU 16 Chroma Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->rate_bias_I_08C & 0x1f, 27 ,5);//Intra Mode TU  8 Chroma Rate Bias in I Frame

    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_I_04C & 0x1f, 0 ,5); //Intra Mode TU  4 Chroma Rate Bias in I Frame
    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_IP_16L & 0x1f,5 ,5); //Intra Mode TU 16 Luma   Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_IP_08L & 0x1f,10 ,5);//Intra Mode TU  8 Luma   Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_IP_04L & 0x1f,15 ,5);//Intra Mode TU  4 Luma   Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_IP_08C & 0x1f,20,5); //Intra Mode TU  8 Chroma Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->rate_bias_IP_04C & 0x1f,25 ,5);//Intra Mode TU  4 Chroma Rate Bias in P Frame

    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_32L & 0x1f, 0 ,5); //Inter Mode TU 32 Luma   Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_16L & 0x1f, 5 ,5); //Inter Mode TU 16 Luma   Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_08L & 0x1f, 10 ,5);//Inter Mode TU  8 Chroma Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_16C & 0x1f, 15 ,5);//Inter Mode TU 16 Chroma Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_08C & 0x1f, 20 ,5);//Inter Mode TU  8 Chroma Rate Bias in P Frame
    save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->rate_bias_P_04C & 0x1f, 25 ,5);//Inter Mode TU  4 Chroma Rate Bias in P Frame

    save_to_reg(&pRegSet->RDO_CFG[3], pRdoCfg->cost_bias_skip , 16 ,5);
    save_to_reg(&pRegSet->RDO_CFG[3], pRdoCfg->cost_bias_merge , 24 ,5);

    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I32, 0, 4);
    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I16, 4, 4);
    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I08, 8, 4);
    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I32O, 12, 4);
    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I16O, 16, 4);
    save_to_reg(&pRegSet->RDO_CFG_4, pRdoCfg->global_motion_penalty_I08O, 20, 4);
}

void h265Enc_wrapFroCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx)
{
	H265EncFroCfg *pFroCfg = &pVdoCtx->stFroCfg;

	save_to_reg(&pRegSet->FUNC_CFG[0], pFroCfg->bEnable, 7, 1); // fro enable //
	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->uiDC[0][0][1], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->ucST[0][0][1], 12, 4);
	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->ucMX[0][0][1], 20, 4);

    save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->uiDC[0][0][2], 0, 9);
    save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->ucST[0][0][2], 12, 5);
    save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->ucMX[0][0][2], 20, 3);

    save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->uiDC[0][1][1], 0, 9);
    save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->ucST[0][1][1], 12, 4);
    save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->ucMX[0][1][1], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->uiDC[0][1][2], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->ucST[0][1][2], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->ucMX[0][1][2], 20, 3);

	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->uiDC[1][0][1], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->ucST[1][0][1], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->ucMX[1][0][1], 20, 3);

	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->uiDC[1][0][2], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->ucST[1][0][2], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->ucMX[1][0][2], 20, 2);

	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->uiDC[1][1][1], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->ucST[1][1][1], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->ucMX[1][1][1], 20, 3);

	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->uiDC[1][1][2], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->ucST[1][1][2], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->ucMX[1][1][2], 20, 2);

	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->uiDC[0][2][1], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->ucST[0][2][1], 12, 4);
	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->ucMX[0][2][1], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->uiDC[0][2][2], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->ucST[0][2][2], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->ucMX[0][2][2], 20, 3);

	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->uiDC[1][2][1], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->ucST[1][2][1], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->ucMX[1][2][1], 20, 3);

	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->uiDC[1][2][2], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->ucST[1][2][2], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->ucMX[1][2][2], 20, 2);

	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][0][1],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][0][2],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][1][1],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][1][2],	24, 8);

	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][0][1],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][0][2],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][1][1],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][1][2],	24, 8);

	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[0][2][1],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[0][2][2],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[1][2][1],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[1][2][2],	24, 8);

	save_to_reg(&pRegSet->FRO_CFG[15], pFroCfg->uiDC[FRO_Y][FRO_IntraI][FRO_Y32], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[15], pFroCfg->ucST[FRO_Y][FRO_IntraI][FRO_Y32], 12, 3);
	save_to_reg(&pRegSet->FRO_CFG[15], pFroCfg->ucMX[FRO_Y][FRO_IntraI][FRO_Y32], 16, 5);
	save_to_reg(&pRegSet->FRO_CFG[15], pFroCfg->ucAC[FRO_Y][FRO_IntraI][FRO_Y32], 24, 8);

	save_to_reg(&pRegSet->FRO_CFG[16], pFroCfg->uiDC[FRO_Y][FRO_IntraI][FRO_Y4], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[16], pFroCfg->ucST[FRO_Y][FRO_IntraI][FRO_Y4], 9, 6);
	save_to_reg(&pRegSet->FRO_CFG[16], pFroCfg->ucMX[FRO_Y][FRO_IntraI][FRO_Y4], 16, 2);
	save_to_reg(&pRegSet->FRO_CFG[16], pFroCfg->ucAC[FRO_Y][FRO_IntraI][FRO_Y4], 24, 8);

	save_to_reg(&pRegSet->FRO_CFG[17], pFroCfg->uiDC[FRO_Y][FRO_IntraP][FRO_Y4], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[17], pFroCfg->ucST[FRO_Y][FRO_IntraP][FRO_Y4], 9, 6);
	save_to_reg(&pRegSet->FRO_CFG[17], pFroCfg->ucMX[FRO_Y][FRO_IntraP][FRO_Y4],16, 2);
	save_to_reg(&pRegSet->FRO_CFG[17], pFroCfg->ucAC[FRO_Y][FRO_IntraP][FRO_Y4],24, 8);

	save_to_reg(&pRegSet->FRO_CFG[18], pFroCfg->uiDC[FRO_C][FRO_IntraI][FRO_C16], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[18], pFroCfg->ucST[FRO_C][FRO_IntraI][FRO_C16], 9, 4);
	save_to_reg(&pRegSet->FRO_CFG[18], pFroCfg->ucMX[FRO_C][FRO_IntraI][FRO_C16],16, 4);
	save_to_reg(&pRegSet->FRO_CFG[18], pFroCfg->ucAC[FRO_C][FRO_IntraI][FRO_C16],24, 8);

	save_to_reg(&pRegSet->FRO_CFG[19], pFroCfg->uiDC[FRO_Y][FRO_InterP][FRO_Y32],0,9);
	save_to_reg(&pRegSet->FRO_CFG[19], pFroCfg->ucST[FRO_Y][FRO_InterP][FRO_Y32],12, 3);
	save_to_reg(&pRegSet->FRO_CFG[19], pFroCfg->ucMX[FRO_Y][FRO_InterP][FRO_Y32],16, 5);
	save_to_reg(&pRegSet->FRO_CFG[19], pFroCfg->ucAC[FRO_Y][FRO_InterP][FRO_Y32],24, 8);

	save_to_reg(&pRegSet->FRO_CFG[20], pFroCfg->uiDC[FRO_C][FRO_InterP][FRO_C16], 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[20], pFroCfg->ucST[FRO_C][FRO_InterP][FRO_C16], 9, 4);
	save_to_reg(&pRegSet->FRO_CFG[20], pFroCfg->ucMX[FRO_C][FRO_InterP][FRO_C16],16, 4);
    save_to_reg(&pRegSet->FRO_CFG[20], pFroCfg->ucAC[FRO_C][FRO_InterP][FRO_C16],24, 8);
}

void h265Enc_wrapQpRelatedCfg(H26XRegSet *pRegSet, H265EncQpRelatedCfg *pQpCfg)
{
	//MinCuDQPSize = 64x64, bit[29:28] = 2;
	//MinCuDQPSize = 32x32, bit[29:28] = 1;
	//MinCuDQPSize = 16x16, bit[29:28] = 0;
	//save_to_reg(&pRegSet->AEAD_CFG, pQpCfg->ucMaxCuDQPDepth,  24, 2);	// log2_mincu_size
    save_to_reg(&pRegSet->FUNC_CFG[1], pQpCfg->ucImeCoherentQP,             12, 2);
    save_to_reg(&pRegSet->FUNC_CFG[1], pQpCfg->ucRdoCoherentQP,             14, 2);
    save_to_reg(&pRegSet->FUNC_CFG[1], pQpCfg->ucQpMergeMethod,             6, 2);
}


