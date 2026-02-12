/*
    H264ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H264RegSet

    @file       h264enc_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "nvt_vdocdc_dbg.h"

#include "h26x_common.h"

#include "h264enc_int.h"
#include "h264enc_wrap.h"

#ifdef __FREERTOS
    int gH264RowRCStopFactor = 5;
#else
    int gH264RowRCStopFactor = 5;
#endif
int gFixSPSLog2Poc = 0;

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
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;

	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	H26XEncAddr   *pAddr   = &pComnCtx->stVaAddr;

	// sequence config
	pSeqCfg->eProfile		= pInit->eProfile;
	pSeqCfg->eEntropyMode	= pInit->eEntropyMode;
	//check compatible
	if ((pSeqCfg->eProfile == PROFILE_BASELINE) && (pSeqCfg->eEntropyMode == CABAC)) {
		DBG_WRN("BASELINE profile NOT support CABAC, change to CAVLC\r\n");
		pSeqCfg->eEntropyMode = CAVLC;
	}
	pSeqCfg->ucLevelIdc     = pInit->ucLevelIdc;

	pSeqCfg->usWidth		= pInit->uiWidth;
	pSeqCfg->usHeight		= pInit->uiHeight;
	pSeqCfg->usMbWidth  	= SIZE_16X(pSeqCfg->usWidth) / 16;
	pSeqCfg->usMbHeight 	= SIZE_16X(pSeqCfg->usHeight) / 16;
	pSeqCfg->uiTotalMBs 	= pSeqCfg->usMbWidth * pSeqCfg->usMbHeight;

	pSeqCfg->uiDisplayWidth	= pInit->uiDisplayWidth;
	pSeqCfg->uiGopNum		= pInit->uiGopNum;
    pSeqCfg->usLog2MaxFrm   = Clip3_JM(4, 16, (int)(log2bin(4096)));
    if (gFixSPSLog2Poc) {
        int max_poc = log2bin(2 * pSeqCfg->uiGopNum - 1);
        if (gFixSPSLog2Poc <  max_poc) {
            pSeqCfg->usLog2MaxPoc	= Clip3_JM(4, 16, (int)(log2bin(2 * pSeqCfg->uiGopNum - 1)));
            // warning
        }
        else
        	pSeqCfg->usLog2MaxPoc   = Clip3_JM(4, 16, gFixSPSLog2Poc);
    }
    else {
	pSeqCfg->usLog2MaxPoc	= Clip3_JM(4, 16, (int)(log2bin(4096)));
    }

	pSeqCfg->cChrmQPIdx		= pInit->cChrmQPIdx;
	pSeqCfg->cSecChrmQPIdx	= pInit->cSecChrmQPIdx;
	pSeqCfg->ucDisLFIdc		= pInit->ucDisLFIdc;
	pSeqCfg->cDBAlpha       = pInit->cDBAlpha;
	pSeqCfg->cDBBeta        = pInit->cDBBeta;
	pSeqCfg->bTrans8x8En	= pInit->bTrans8x8En;
	pSeqCfg->bForwardRecChrmEn = pInit->bForwardRecChrmEn;

	pSeqCfg->ucSVCLayer     = pInit->ucSVCLayer;
	pSeqCfg->uiLTRInterval	= pInit->uiLTRInterval;
	pSeqCfg->bLTRPreRef		= pInit->bLTRPreRef;
	pSeqCfg->ucNumRefIdxL0  = 1 + (pInit->ucSVCLayer == 2) + (pInit->uiLTRInterval != 0);

	pSeqCfg->uiBitRate		= pInit->uiBitRate;	// Bit rate (bits per second)
	pSeqCfg->uiFrmRate 		= pInit->uiFrmRate;	// Frame rate (frames per second * 1000)
	pSeqCfg->ucInitIQP		= pInit->ucIQP;
	pSeqCfg->ucInitPQP		= pInit->ucPQP;

	pSeqCfg->bFBCEn			= pInit->bFBCEn;
	pSeqCfg->bGrayEn		= pInit->bGrayEn;
	pSeqCfg->bFastSearchEn	= pInit->bFastSearchEn;
	pSeqCfg->bHwPaddingEn	= pInit->bHwPaddingEn;
	pSeqCfg->ucRotate		= pInit->ucRotate;
	pSeqCfg->bD2dEn			= pInit->bD2dEn;

	pSeqCfg->bVUIEn			= pInit->bVUIEn;
	pSeqCfg->usSarWidth		= pInit->usSarWidth;
	pSeqCfg->usSarHeight	= pInit->usSarHeight;
	pSeqCfg->ucColorRange	= pInit->ucColorRange;
    pSeqCfg->ucMatrixCoef   = pInit->ucMatrixCoef;
    pSeqCfg->ucTransferCharacteristics = pInit->ucTransferCharacteristics;
    pSeqCfg->ucColourPrimaries = pInit->ucColourPrimaries;
    pSeqCfg->ucVideoFormat  = pInit->ucVideoFormat;
    pSeqCfg->bTimeingPresentFlag = pInit->bTimeingPresentFlag;

	// picture config //
	if (pInit->bGDRIFrm == 1) {
		pPicCfg->ePicType		= I_SLICE;
		pPicCfg->eNxtPicType	= I_SLICE;
	} else {
		pPicCfg->ePicType		= IDR_SLICE;
		pPicCfg->eNxtPicType	= IDR_SLICE;
	}
	pPicCfg->config_loop_filter = 1;

	if (pSeqCfg->eProfile == PROFILE_BASELINE)
	{
		pSeqCfg->bDirect8x8En = FALSE;
		pSeqCfg->bTrans8x8En = FALSE;
	}
	else if (pSeqCfg->eProfile == PROFILE_MAIN)
	{
		pSeqCfg->bDirect8x8En = TRUE;
		pSeqCfg->bTrans8x8En = FALSE;
	}
	else if (pSeqCfg->eProfile == PROFILE_HIGH)
	{
		pSeqCfg->bDirect8x8En = TRUE;
	}
	else
		;

	// lineoffset //
	if (pInit->bFBCEn)
	{
		pAddr->uiRecYLineOffset = SIZE_16X(pSeqCfg->usHeight)/4;
		pAddr->uiRecCLineOffset = SIZE_16X(pSeqCfg->usHeight)/8;
	}
	else
	{
		// need to check 64x or 16x //
		//pAddr->uiRecYLineOffset = pInit->uiRecLineOffset/4;
		//pAddr->uiRecCLineOffset = pInit->uiRecLineOffset/4;
		pAddr->uiRecYLineOffset = SIZE_64X(pSeqCfg->usWidth)/4;
		pAddr->uiRecCLineOffset = SIZE_64X(pSeqCfg->usWidth)/4;
	}
	pAddr->uiSideInfoLineOffset = ((pSeqCfg->usWidth + 63)/64);
	//pAddr->uiSideInfoLineOffset = (SIZE_32X(((pSeqCfg->usWidth + 63)/64)*4))/4;
	pSeqCfg->frm_crop_info.frm_crop_enable = pInit->frm_crop_info.frm_crop_enable;
	pSeqCfg->frm_crop_info.display_w = pInit->frm_crop_info.display_w;
	pSeqCfg->frm_crop_info.display_h = pInit->frm_crop_info.display_h;
}

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

