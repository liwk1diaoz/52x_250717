/*
    H264ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H264RegSet

    @file       h264enc_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h26x_common.h"

#include "h264enc_int.h"
#include "h264enc_wrap.h"

#ifdef __FREERTOS
    int gH264RowRCStopFactor = 5;
#else
    int gH264RowRCStopFactor = 300;
#endif
int gFixSPSLog2Poc = 0;
#define RC_StartFrame 2
#define MAX_b_RowSizeNew 23
#define MAX_b_CompNew    5
#define MAX_RowSizeNew   ((1LL << MAX_b_RowSizeNew) - 1)
#define MAX_CompSizeNew  ((1LL << (MAX_b_RowSizeNew - MAX_b_CompNew)) - 1)
#ifndef max
#define max(a, b)           (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)           (((a) < (b)) ? (a) : (b))
#endif

static INT32 i_Pred_FixU_Tab[7][8] =
{
	{ 150, 177, 208, 244, 287,  337,  396,  465 },
	{ 157, 192, 235, 288, 353,  433,  530,  649 },
	{ 163, 208, 265, 338, 431,  550,  701,  894 },
	{ 170, 225, 298, 395, 523,  693,  918, 1216 },
	{ 176, 242, 333, 458, 629,  865, 1189, 1635 },
	{ 182, 260, 370, 528, 752, 1072, 1527, 2176 },
	{ 189, 278, 411, 606, 894, 1318, 1944, 2868 }
};

static INT32 i_Pred_FixD_Tab[7][8] =
{
	{ 109, 93, 79, 67, 57, 49, 41, 35 },
	{ 104, 85, 70, 57, 46, 38, 31, 25 },
	{ 100, 79, 62, 48, 38, 30, 23, 18 },
	{  97, 73, 55, 42, 31, 24, 18, 13 },
	{  93, 68, 49, 36, 26, 19, 14, 10 },
	{  90, 63, 44, 31, 22, 15, 11,  8 },
	{  87, 59, 40, 27, 18, 12,  8,  6 }
};

static INT32 i_Beta2ModU_Tab[16] =
{
	192, 189, 186, 183, 180, 177, 174, 171, 168, 165, 162, 159, 156, 153, 150, 147
};

static INT32 i_Beta2ModD_Tab[16] =
{
	85, 87, 88, 90, 91, 93, 94, 96, 98, 99, 101, 103, 105, 107, 109, 111
};


//! Initialize encoder configurate
/*!
    Convert pH264EncInit to SeqCfg and PicCfg

    @return
        - @b H264ENC_SUCCESS: init success
        - @b H264ENC_FAIL   : init fail
*/
void h264Enc_initCfg(H264ENC_INIT *pInit, H26XENC_VAR *pVar)
{
	H264ENC_CTX  *pVdoCtx  = pVar->pVdoCtx;	
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	H26XEncAddr   *pAddr   = &pVdoCtx->stAddr;
	H26XEncRcCfg  *pRcCfg  = &pVdoCtx->stRcCfg;
	
	// sequence config
	pSeqCfg->eProfile		= pInit->eProfile;
	pSeqCfg->eEntropyMode	= pInit->eEntropyMode;
	
	pSeqCfg->usWidth		= pInit->uiWidth;
	pSeqCfg->usHeight		= pInit->uiHeight;	
	pSeqCfg->usMbWidth  	= SIZE_16X(pSeqCfg->usWidth) / 16;
	pSeqCfg->usMbHeight 	= SIZE_16X(pSeqCfg->usHeight) / 16;
	pSeqCfg->uiTotalMBs 	= pSeqCfg->usMbWidth * pSeqCfg->usMbHeight;	
	
	pSeqCfg->usSarWidth		= pInit->uiSarWidth;
	pSeqCfg->usSarHeight	= pInit->uiSarHeight;
	pSeqCfg->uiDisplayWidth	= pInit->uiDisplayWidth;
	pSeqCfg->uiGopNum		= pInit->uiGopNum;		
	pSeqCfg->usLog2MaxFrm	= Clip3_JM(4, 16, (int)(log2bin(4096)));
	pSeqCfg->usLog2MaxPoc	= Clip3_JM(4, 16, (int)(log2bin(2 * pSeqCfg->uiGopNum - 1)));
		
	pSeqCfg->ucColorRange	= pInit->ucColorRange;	
	pSeqCfg->cChrmQPIdx		= pInit->cChrmQPIdx;	
	pSeqCfg->cSecChrmQPIdx	= pInit->cSecChrmQPIdx;	
	pSeqCfg->ucDisLFIdc		= pInit->ucDisLFIdc;
	pSeqCfg->cDBAlpha       = pInit->cDBAlpha;
	pSeqCfg->cDBBeta        = pInit->cDBBeta;		
	pSeqCfg->bTrans8x8En	= pInit->bTrans8x8En;
	
	pSeqCfg->ucSVCLayer     = pInit->ucSVCLayer;
	pSeqCfg->uiLTRInterval	= pInit->uiLTRInterval;
	pSeqCfg->bLTRPreRef		= pInit->bLTRPreRef;
	
	pSeqCfg->uiBitRate		= pInit->uiBitRate;	// Bit rate (bits per second)		
	pSeqCfg->uiFrmRate 		= pInit->uiFrmRate;	// Frame rate (frames per second * 1000)
	pSeqCfg->ucInitIQP		= pInit->ucIQP;
	pSeqCfg->ucInitPQP		= pInit->ucPQP;	

	pSeqCfg->bFBCEn			= pInit->bFBCEn;
	pSeqCfg->bGrayEn		= pInit->bGrayEn;
    pSeqCfg->bGrayColorEn	= pInit->bGrayColorEn;
	pSeqCfg->bFastSearchEn	= pInit->bFastSearchEn;
	pSeqCfg->bHwPaddingEn	= pInit->bHwPaddingEn;
	pSeqCfg->ucRotate		= pInit->ucRotate;	

	// picture config //
	pPicCfg->ePicType		= IDR_SLICE;	
	pPicCfg->config_loop_filter = 1;	

	// rate control config //
	pRcCfg->ucMaxIQp 		= pInit->ucMaxIQp;
	pRcCfg->ucMinIQp 		= pInit->ucMinIQp;
	pRcCfg->ucMaxPQp 		= pInit->ucMaxPQp;
	pRcCfg->ucMinPQp 		= pInit->ucMinPQp;

	// lineoffset //
	if (pInit->bFBCEn)
	{
		pAddr->uiRecYLineOffset = SIZE_16X(pSeqCfg->usHeight)/4;
		pAddr->uiRecCLineOffset = SIZE_16X(pSeqCfg->usHeight)/8;
	}
	else
	{	
		// need to check 64x or 16x //
		pAddr->uiRecYLineOffset = pInit->uiRecLineOffset/4;
		pAddr->uiRecCLineOffset = pInit->uiRecLineOffset/4;
	}
	pAddr->uiSideInfoLineOffset = ((pSeqCfg->usWidth + 63)/64);
	//pAddr->uiSideInfoLineOffset = (SIZE_32X(((pSeqCfg->usWidth + 63)/64)*4))/4;
}

void h264Enc_initRrc(H264ENC_CTX *pVdoCtx, H26XEncRowRc *pRowRc, BOOL bIsPFrm, INT32 iCurFrameNo, UINT8  usInitQp, UINT32 uiAvgQP)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	INT32 iDiffRowQP = 0;
	INT32 iModU_idx = 0;
	INT32 iShift = 0;
	INT32 iShift_3 = 0;
	INT32 iShift_2 = 0;
	INT32 iScaleShift = 0;
	INT32 iBeta;
	INT32 iBeta2ModU_idx;
	//ROW_RC_NEW
    INT32 iMinBitsRow = (pSeqCfg->usWidth + 8) >> 4;
    INT32 iTargetBitsLeft;
    INT32 iTargetBitsScale_ori;


    //iTargetBitsScale_ori = pRowRc->iTargetBitsScale;

	if(pRowRc->stCfg.usRRCMode == 1){

        //printf("[%s][%d] planned_top = %d,i_all_ref_pred_tmpl = 0x%08x 0x%08x %d\r\n", __FUNCTION__, __LINE__,pRowRc->uiPlannedTop, (pRowRc->i_all_ref_pred_tmpl >> 32) & 0xFFFFFFFF,(pRowRc->i_all_ref_pred_tmpl >> 0) & 0xFFFFFFFF, (int)pRowRc->i_all_ref_pred_tmpl);

		//iBeta = -1 * m_Mcprc.m_RCSeq.m_picPara[m_RcV1.curr_slice_type].m_beta;
		iBeta = pRowRc->iBeta;//test
		//printf("iBeta = %d\n", iBeta);
		iBeta2ModU_idx = iBeta < 1191 ?  0 :
						 iBeta < 1240 ?  1 :
						 iBeta < 1295 ?  2 :
						 iBeta < 1355 ?  3 :
						 iBeta < 1423 ?  4 :
						 iBeta < 1500 ?  5 :
						 iBeta < 1586 ?  6 :
						 iBeta < 1686 ?  7 :
						 iBeta < 1800 ?  8 :
						 iBeta < 1934 ?  9 :
						 iBeta < 2092 ? 10 :
						 iBeta < 2282 ? 11 :
						 iBeta < 2515 ? 12 :
						 iBeta < 2808 ? 13 :
						 iBeta < 3186 ? 14 : 15;
		pRowRc->m_ModU_Frm[bIsPFrm] = i_Beta2ModU_Tab[iBeta2ModU_idx];
		pRowRc->m_ModD_Frm[bIsPFrm] = i_Beta2ModD_Tab[iBeta2ModU_idx];
		if (iCurFrameNo >= RC_StartFrame)
		{

			//Init thresholds
			pRowRc->iTH_0 = (pRowRc->uiPlannedTop * 31) >> 7;
			pRowRc->iTH_M = (pRowRc->uiPlannedTop * 102) >> 7;

			pRowRc->iTH_0 = max(pRowRc->iTH_0, iMinBitsRow >> 1);
			pRowRc->iTH_M = max(pRowRc->iTH_M, iMinBitsRow << 1);
			pRowRc->iTH_M *= -1;
			pRowRc->iTH_1 = pRowRc->m_ModD_Frm[bIsPFrm] - 128;
			pRowRc->iTH_2 = pRowRc->m_ModU_Frm[bIsPFrm] - 128;
			pRowRc->iTH_3 = (pRowRc->iTH_1 * 256) >> 7;
			pRowRc->iTH_4 = (pRowRc->iTH_2 * 256) >> 7;
			pRowRc->iTH_1 = (pRowRc->iTH_1 * 77) >> 7;
			pRowRc->iTH_2 = (pRowRc->iTH_2 * 77) >> 7;
			pRowRc->iTH_1 += 128;
			pRowRc->iTH_2 += 128;
			pRowRc->iTH_3 += 128;
			pRowRc->iTH_4 += 128;

			//Fix RowPredSum by LastFrmAvgQPDiff
			iModU_idx = pRowRc->m_ModU_Frm[bIsPFrm] < 154 ? 0 :
				            pRowRc->m_ModU_Frm[bIsPFrm] < 160 ? 1 :
				            pRowRc->m_ModU_Frm[bIsPFrm] < 167 ? 2 :
				            pRowRc->m_ModU_Frm[bIsPFrm] < 173 ? 3 :
				            pRowRc->m_ModU_Frm[bIsPFrm] < 180 ? 4 :
				            pRowRc->m_ModU_Frm[bIsPFrm] < 186 ? 5 : 6;


			while (pRowRc->i_all_ref_pred_tmpl > MAX_CompSizeNew)
			{
				pRowRc->i_all_ref_pred_tmpl >>= 1;
				iShift++;
			}
			pRowRc->iLastFrmAvgQPDiff = usInitQp - uiAvgQP;
			if (pRowRc->iLastFrmAvgQPDiff < 0)
			{
				iDiffRowQP = min(-1 * pRowRc->iLastFrmAvgQPDiff - 1, 6);
				pRowRc->i_all_ref_pred_tmpl *= i_Pred_FixD_Tab[iModU_idx][iDiffRowQP];
				pRowRc->i_all_ref_pred_tmpl >>= 7;
			}
			else if (pRowRc->iLastFrmAvgQPDiff > 0)
			{
				iDiffRowQP = min(pRowRc->iLastFrmAvgQPDiff - 1, 6);
				pRowRc->i_all_ref_pred_tmpl *= i_Pred_FixU_Tab[iModU_idx][iDiffRowQP];
				pRowRc->i_all_ref_pred_tmpl >>= 7;
			}
			if (iShift > 0)
			{
				pRowRc->i_all_ref_pred_tmpl <<= iShift;
			}

			//Get TargetBitsScale
			pRowRc->uiPlannedTop = min(pRowRc->uiPlannedTop, MAX_RowSizeNew);

            //printf("[%s][%d]frame_size_planned_top = %d, MAX_RowSizeNew = %d,\n", __FUNCTION__, __LINE__,pRowRc->uiPlannedTop,MAX_RowSizeNew);


			while ((pRowRc->uiPlannedTop > ((1LL << (MAX_b_RowSizeNew - 3)) - 1)) ||
			   (pRowRc->i_all_ref_pred_tmpl > ((1LL << 31) - 1)))
			{
				pRowRc->uiPlannedTop >>= 1;
				pRowRc->i_all_ref_pred_tmpl >>= 1;
                iShift_3++;
                //printf("[%s][%d]frame_size_planned_top = %d, MAX_b_RowSizeNew = %d\n", __FUNCTION__, __LINE__,pRowRc->uiPlannedTop,MAX_b_RowSizeNew);
			}

            if(pRowRc->i_all_ref_pred_tmpl != 0){
                pRowRc->iTargetBitsScale = ((pRowRc->uiPlannedTop << 10) + (pRowRc->i_all_ref_pred_tmpl >> 1)) / pRowRc->i_all_ref_pred_tmpl;
            }else{
                //printf("%s %d, error! %d\n",__FUNCTION__,__LINE__,pRowRc->i_all_ref_pred_tmpl);
            }

			if (iShift_3 > 0)
			{
				pRowRc->uiPlannedTop <<= iShift_3;
				pRowRc->i_all_ref_pred_tmpl <<= iShift_3;
                //printf("[%s][%d]frame_size_planned_top = %d, iShift_3 = %d\n", __FUNCTION__, __LINE__,pRowRc->uiPlannedTop,iShift_3);
			}
			iScaleShift = pRowRc->i_all_ref_pred_tmpl < (1 << 11) ? 9 : 4;
			pRowRc->iTargetBitsScale = min(pRowRc->iTargetBitsScale, 1 << ((7 + iScaleShift) - 1));

			//Fix RowPredSum by TargetBitsScale
			iTargetBitsLeft = pRowRc->uiPlannedTop << 3;
			while (pRowRc->i_all_ref_pred_tmpl > MAX_CompSizeNew)
			{
				pRowRc->i_all_ref_pred_tmpl >>= 1;
				iShift_2++;
			}
			pRowRc->i_all_ref_pred_tmpl *= pRowRc->iTargetBitsScale;
			pRowRc->i_all_ref_pred_tmpl >>= 7;
			if (iShift_2 > 0)
			{
				pRowRc->i_all_ref_pred_tmpl <<= iShift_2;
			}
			pRowRc->i_all_ref_pred_tmpl = max(pRowRc->i_all_ref_pred_tmpl, iTargetBitsLeft);
		}

			//adjust scale by qpdiff
			if (pRowRc->iLastFrmAvgQPDiff < 0)
			{
				iDiffRowQP = min(-1 * pRowRc->iLastFrmAvgQPDiff - 1, 6);
				pRowRc->iTargetBitsScale *= i_Pred_FixD_Tab[iModU_idx][iDiffRowQP];
				pRowRc->iTargetBitsScale >>= 7;
			}
			else if (pRowRc->iLastFrmAvgQPDiff > 0)
			{
				iDiffRowQP = min(pRowRc->iLastFrmAvgQPDiff - 1, 6);
				pRowRc->iTargetBitsScale *= i_Pred_FixU_Tab[iModU_idx][iDiffRowQP];
				pRowRc->iTargetBitsScale >>= 7;
			}
	}

#if 1
    if(pRowRc->iTargetBitsScale != iTargetBitsScale_ori)
    {
        //printf("[%s][%d] error!!! iTargetBitsScale = %d , %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTargetBitsScale,iTargetBitsScale_ori);
    }
#else
    pRowRc->iTargetBitsScale = iTargetBitsScale_ori;
#endif
/*
    printf("[%s][%d] planned_top = %d\r\n", __FUNCTION__, __LINE__,pRowRc->uiPlannedTop);
    printf("[%s][%d] init_qp = %d\r\n", __FUNCTION__, __LINE__,usInitQp);
    printf("[%s][%d] beta = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iBeta);
    printf("[%s][%d] th_0 = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_0);
    printf("[%s][%d] th_1 = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_1);
    printf("[%s][%d] th_2 = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_2);
    printf("[%s][%d] th_3 = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_3);
    printf("[%s][%d] th_4 = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_4);
    printf("[%s][%d] th_M = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTH_M);
    printf("[%s][%d] mod_u_frm = %d\r\n", __FUNCTION__, __LINE__,pRowRc->m_ModU_Frm[bIsPFrm]);
    printf("[%s][%d] mod_d_Frm = %d\r\n", __FUNCTION__, __LINE__,pRowRc->m_ModD_Frm[bIsPFrm]);

    printf("[%s][%d] iTargetBitsScale = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iTargetBitsScale);
    printf("[%s][%d] iLastFrmAvgQPDiff = %d\r\n", __FUNCTION__, __LINE__,pRowRc->iLastFrmAvgQPDiff);
    printf("[%s][%d] i_all_ref_pred_tmpl = 0x%08x %08x %d\r\n", __FUNCTION__, __LINE__,pRowRc->i_all_ref_pred_tmpl >> 32, pRowRc->i_all_ref_pred_tmpl & 0xFFFFFFFF,(int) pRowRc->i_all_ref_pred_tmpl);
*/


	//~ROW_RC_NEW
}



void h264Enc_updatePicCfg(H264ENC_CTX *pVdoCtx)
{
//	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	pPicCfg->uiPicCnt++;
#if 0
	if (pPicCfg->ePicType == IDR_SLICE)
	{
		pPicCfg->usFrmNum = 0;
		pPicCfg->usFrmForIdrPicId = (pPicCfg->usFrmForIdrPicId + 1)%0xfff;
		pPicCfg->sPoc = 0;
	}
	else
	{
		if (pPicCfg->uiNalRefIdc)
		{
			pPicCfg->usFrmNum = (UINT32)(pPicCfg->usFrmNum + 1) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxFrm));
			pPicCfg->sPoc = (UINT32)(pPicCfg->sPoc + 1) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxPoc));
		}
	}
#endif
}


INT32 h264Enc_setRowRcCfg2(H26XENC_VAR *pVar, H26XEncRowRc *pRowRc)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
    pFuncCtx->stRowRc.uiPlannedTop = pRowRc->uiPlannedTop;
    pFuncCtx->stRowRc.iBeta = pRowRc->iBeta;
    pFuncCtx->stRowRc.usInitQp = pRowRc->usInitQp;
    #if 0
    pFuncCtx->stRowRc.i_all_ref_pred_tmpl = pRowRc->i_all_ref_pred_tmpl;
    pFuncCtx->stRowRc.iTargetBitsScale = pRowRc->iTargetBitsScale;
    #else
    if(pFuncCtx->stRowRc.i_all_ref_pred_tmpl != pRowRc->i_all_ref_pred_tmpl)
    {
        //printf("[%s][%d] error! i_all_ref_pred_tmpl = %d %d\r\n", __FUNCTION__, __LINE__,(int)pFuncCtx->stRowRc.i_all_ref_pred_tmpl,
        //    pFuncCtx->stRowRc.i_all_ref_pred_tmpl,pRowRc->i_all_ref_pred_tmpl);

    }
    //pFuncCtx->stRowRc.iTargetBitsScale = pRowRc->iTargetBitsScale;

    #endif
    //printf("[%s][%d] planned_top = %d\r\n", __FUNCTION__, __LINE__,pFuncCtx->stRowRc.uiPlannedTop);
    //printf("[%s][%d] iBeta = %d\r\n", __FUNCTION__, __LINE__,pFuncCtx->stRowRc.iBeta);
    //printf("[%s][%d] i_all_ref_pred_tmpl = %d\r\n", __FUNCTION__, __LINE__,(int)pFuncCtx->stRowRc.i_all_ref_pred_tmpl);
    return 0;

}





#if 0
void h264Enc_updatePicCfg(H264ENC_CTX *pVdoCtx)
{	
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
		
	if (pPicCfg->ePicType == IDR_SLICE) {		
		pPicCfg->usFrmForIdrPicId = (pPicCfg->usFrmForIdrPicId + 1)%0xfff;		
	}

	if (pPicCfg->uiNalRefIdc) {
		pPicCfg->usFrmNum = (UINT32)(pPicCfg->usFrmNum + 1) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxFrm));
	}

    pPicCfg->sPoc = (UINT32)(pPicCfg->sPoc + 1) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxPoc));
}

void h264Enc_modifyRefFrm(H264ENC_CTX *pVdoCtx, UINT32 uiPicCnt, BOOL bRecFBCEn)
{	
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;			

	// generate rec and ref frame index //
	pVdoCtx->eRefIdx = (pPicCfg->bLastRecIsLT) ? FRM_IDX_LT_0 : ((pVdoCtx->eRecIdx == FRM_IDX_NON_REF) ? FRM_IDX_ST_0 : pVdoCtx->eRecIdx);
	pVdoCtx->eRecIdx = (pPicCfg->uiNalRefIdc) ? FRM_IDX_ST_0 : FRM_IDX_NON_REF;
	pVdoCtx->uiSvcLable = (pPicCfg->uiNalRefIdc) ? 0 : pSeqCfg->ucSVCLayer;
		
	if (pSeqCfg->ucSVCLayer == 2) {
		if ((pPicCfg->sRefPoc % 4) == 3)
			pVdoCtx->eRefIdx = FRM_IDX_ST_1;

		if ((pPicCfg->sRefPoc % 4) == 2)
			pVdoCtx->uiSvcLable = 1;
	}		
	
	if (pSeqCfg->uiLTRInterval) {
		if (pPicCfg->sRefPoc == 0) {
			if (pSeqCfg->bLTRPreRef)
				pVdoCtx->eRecIdx = FRM_IDX_LT_0;
			else
				pVdoCtx->eRecIdx = (pPicCfg->sPoc) ? FRM_IDX_ST_0 : FRM_IDX_LT_0;

			pVdoCtx->eRefIdx = FRM_IDX_LT_0;
		}		
	}	

	if (pSeqCfg->ucSVCLayer == 2) {
		if ((pPicCfg->sRefPoc % 4) == 2)
			pVdoCtx->eRecIdx = FRM_IDX_ST_1;		
	}

	if (pVdoCtx->eRecIdx < FRM_IDX_LT_0) {
		pPicCfg->bLastRecIsLT = FALSE;
	}

	if (pVdoCtx->eRecIdx == FRM_IDX_LT_0) {
		pPicCfg->bLastRecIsLT = TRUE;
	}
	
	pPicCfg->bFBCEn[pVdoCtx->eRecIdx] = bRecFBCEn;
}

void h264Enc_updateRowRcCfg(SLICE_TYPE ePicType, H26XEncRowRc *pRowRc)
{	
    pRowRc->uiPlannedStop = (pRowRc->uiPredBits* gH264RowRCStopFactor / 100) / 8;
	//pRowRc->uiPlannedStop = (pRowRc->uiPredBits* 5 / 100) / 8;
	pRowRc->uiPlannedTop  = (pRowRc->uiPredBits * 1) / 8;
	pRowRc->uiPlannedBot  = (pRowRc->uiPredBits * 97 + 50) / 100 / 8;
}
#endif
