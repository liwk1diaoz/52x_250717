#define __MODULE__          H26XENC
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/type.h"

#include "kdrv_vdocdc_dbg.h"

#include "h26x_def.h"
//#include "h26x_common.h"
#include "h26x_bitstream.h"

#include "h26xenc_api.h"
#include "h26xenc_wrap.h"
#include "h26xenc_int.h"
#include "h26xenc_rc.h"

#include "h265enc_int.h"

#if USE_JND_DEFAULT_VALUE
    H26XEncJndCfg gEncJNDCfg = {0,  3,  0,  20};
    int gUseDefaultJNDCfg = 1;
#endif

extern rc_cb g_rc_cb;


INT32 H26XEnc_setUsrQpCfg(H26XENC_VAR *pVar, H26XEncUsrQpCfg *pUsrQp)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter not ready //

	memcpy(&pFuncCtx->stUsrQp.stCfg, pUsrQp, sizeof(H26XEncUsrQpCfg));

	if (pUsrQp->uiQpMapAddr != pFuncCtx->stUsrQp.uiIntQpMapAddr)
		memcpy((UINT32 *)pFuncCtx->stUsrQp.uiIntQpMapAddr, (UINT32 *)pUsrQp->uiQpMapAddr, pFuncCtx->stUsrQp.uiUsrQpMapSize);

	h26xEnc_wrapUsrQpCfg(pRegSet, &pFuncCtx->stUsrQp);

	return H26XENC_SUCCESS;
}

INT32 H26XEnc_setPSNRCfg(H26XENC_VAR *pVar, BOOL bEnable)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	pFuncCtx->bPSNREn = bEnable;

	h26xEnc_wrapPSNRCfg(pRegSet, pFuncCtx->bPSNREn);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setSliceSplitCfg(H26XENC_VAR *pVar, H26XEncSliceSplitCfg *pSliceSplit)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	memcpy(&pFuncCtx->stSliceSplit.stCfg, pSliceSplit, sizeof(H26XEncSliceSplitCfg));

	h26xEnc_wrapSliceSplitCfg(pRegSet, &pFuncCtx->stSliceSplit);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setVarCfg(H26XENC_VAR *pVar, H26XEncVarCfg *pVarCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	memcpy(&pFuncCtx->stVar, pVarCfg, sizeof(H26XEncVarCfg));

	h26xEnc_wrapVarCfg(pRegSet, &pFuncCtx->stVar);

	return H26XENC_SUCCESS;
}

INT32 h26xEnc_setMaskInitCfg(H26XENC_VAR *pVar, H26XEncMaskInitCfg *pMaskInitCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stMask.stInitCfg, pMaskInitCfg, sizeof(H26XEncMaskInitCfg));

	h26xEnc_wrapMaskInitCfg(pRegSet, &pFuncCtx->stMask);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setMaskWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncMaskWinCfg *pMaskWinCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stMask.stWinCfg[ucIdx], pMaskWinCfg, sizeof(H26XEncMaskWinCfg));

	//DBG_DUMP("h26XEnc_setMaskCfg:%d, %d, %d, %d, %d\r\n", ucIdx, pMaskCfg->ucDid, pMaskCfg->ucPalSel, pMaskCfg->ucLineHitOpt, pMaskCfg->usAlpha);
	h26xEnc_wrapMaskWinCfg(pRegSet, &pFuncCtx->stMask, ucIdx);

	return H26XENC_SUCCESS;
}
INT32 h26XEnc_setGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

	// check parameter //

	memcpy(&pFuncCtx->stGdr.stCfg, pGdrCfg, sizeof(H26XEncGdrCfg));

	pFuncCtx->stGdr.usStart = 0;

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRoiCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncRoiCfg *pRoiCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	if (pRoiCfg->bEnable == TRUE)
	{
		pFuncCtx->stRoi.ucNumber += (pFuncCtx->stRoi.stCfg[ucIdx].bEnable == FALSE);
	}
	else
	{
		pFuncCtx->stRoi.ucNumber -= (pFuncCtx->stRoi.stCfg[ucIdx].bEnable == TRUE);
	}

	memcpy(&pFuncCtx->stRoi.stCfg[ucIdx], pRoiCfg, sizeof(H26XEncRoiCfg));

	h26xEnc_wrapRoiCfg(pRegSet, &pFuncCtx->stRoi, ucIdx);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRowRcCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stRowRc.stCfg, pRowRcCfg, sizeof(H26XEncRowRcCfg));

	h26xEnc_wrapRowRcCfg(pRegSet, &pFuncCtx->stRowRc);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stAq.stCfg, pAqCfg, sizeof(H26XEncAqCfg));

	h26xEnc_wrapAqCfg(pRegSet, &pFuncCtx->stAq);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setLpmCfg(H26XENC_VAR *pVar, H26XEncLpmCfg *pLpmCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stLpm.stCfg, pLpmCfg, sizeof(H26XEncLpmCfg));

	h26xEnc_wrapLpmCfg(pRegSet, &pFuncCtx->stLpm);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRndCfg(H26XENC_VAR *pVar, H26XEncRndCfg *pRndCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stRnd.stCfg, pRndCfg, sizeof(H26XEncRndCfg));

	h26xEnc_wrapRndCfg(pRegSet, &pFuncCtx->stRnd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setScdCfg(H26XENC_VAR *pVar, H26XEncScdCfg *pScdCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stScd.stCfg, pScdCfg, sizeof(H26XEncScdCfg));

	h26xEnc_wrapScdCfg(pRegSet, &pFuncCtx->stScd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgRgbCfg(H26XENC_VAR *pVar, H26XEncOsgRgbCfg *pOsgRgbCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stOsg.stRgb, pOsgRgbCfg, sizeof(H26XEncOsgRgbCfg));

	h26xEnc_wrapOsgRgbCfg(pRegSet, &pFuncCtx->stOsg.stRgb);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgPalCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPalCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stOsg.stPal[ucIdx], pOsgPalCfg, sizeof(H26XEncOsgPalCfg));

	h26xEnc_wrapOsgPalCfg(pRegSet, ucIdx, &pFuncCtx->stOsg.stPal[ucIdx]);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgChromaAlphaCfg(H26XENC_VAR *pVar, UINT8 ucVal)
{
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;
    h26xEnc_wrapOsgChromaAlpha( pRegSet, ucVal);
	return H26XENC_SUCCESS;
}


INT32 h26XEnc_setOsgWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWinCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;
	if (pOsgWinCfg->bEnable == TRUE) {
		if (pOsgWinCfg->stDisp.ucMode == 0) {
			if (pOsgWinCfg->stGrap.uiAddr == 0) {
				DBG_ERR("OSG graphic address cannot set to 0 while enable OSG graphic window\r\n");
				return H26XENC_FAIL;
			}
			else
				h26x_flushCache(pOsgWinCfg->stGrap.uiAddr, pOsgWinCfg->stGrap.usLofs * pOsgWinCfg->stGrap.usHeight);
		}
		else {
			if (pOsgWinCfg->stDisp.ucMode > 1) {
				DBG_ERR("OSG display mode not support %d \r\n", pOsgWinCfg->stDisp.ucMode);
				return H26XENC_FAIL;
			}
		}
	}

	memcpy(&pFuncCtx->stOsg.stWin[ucIdx], pOsgWinCfg, sizeof(H26XEncOsgWinCfg));

	h26xEnc_wrapOsgWinCfg(pRegSet, ucIdx, &pFuncCtx->stOsg.stWin[ucIdx]);

	return H26XENC_SUCCESS;
}

#if 0
INT32 h26XEnc_setMotAqCfg(H26XENC_VAR *pVar, H26XEncMotAqCfg *pMAqCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stMAq.stCfg, pMAqCfg, sizeof(H26XEncMotAqCfg));

	h26xEnc_wrapMotAqCfg(pRegSet, &pFuncCtx->stMAq);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setMotAddrCfg(H26XENC_VAR *pVar, H26XEncMotAddrCfg *pMotAddrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	if (pMotAddrCfg->ucMotBufNum > 3) {
		DBG_ERR("motion address buffer(%d) more than 3\r\n", pMotAddrCfg->ucMotBufNum);
		return H26XENC_FAIL;
	}

	memcpy(&pFuncCtx->stMAq.stAddrCfg, pMotAddrCfg, sizeof(H26XEncMotAddrCfg));

	h26xEnc_wrapMotAddrCfg(pRegSet, &pFuncCtx->stMAq.stAddrCfg);

	return H26XENC_SUCCESS;
}
#endif

INT32 h26XEnc_setMotionAqInitCfg(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;


	h26xEnc_wrapMotionAqInitCfg(pRegSet, &pFuncCtx->stMAq);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setMotionAqCfg(H26XENC_VAR *pVar, H26XEncMotionAqCfg *pMAqCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stMAq.stCfg, pMAqCfg, sizeof(H26XEncMotionAqCfg));

	h26xEnc_wrapMotionAqCfg(pRegSet, &pFuncCtx->stMAq);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setJndCfg(H26XENC_VAR *pVar, H26XEncJndCfg *pJndCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stJnd.stCfg, pJndCfg, sizeof(H26XEncJndCfg));

	h26xEnc_wrapJndCfg(pRegSet, &pFuncCtx->stJnd);

	return H26XENC_SUCCESS;
}
#if 0
INT32 h26XEnc_setRcInit(H26XENC_VAR *pVar, H26XEncRCParam *pRcParam)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	pRcParam->uiPicSize = SIZE_16X(pComnCtx->uiWidth) * SIZE_16X(pComnCtx->uiHeight);
	pRcParam->uiLTRInterval = pComnCtx->uiLTRInterval;
	pRcParam->uiSVCLayer = pComnCtx->ucSVCLayer;

	if (g_rc_cb.h26xEnc_RcInit(&pComnCtx->stRc, pRcParam) != 0) {
		pComnCtx->ucRcMode = 0;
		return H26XENC_FAIL;
	}

	pComnCtx->ucRcMode = pRcParam->uiRCMode;

	if (pComnCtx->ucRcMode == H26X_RC_EVBR) {
		H26XEncMotAqCfg sMAqCfg = {0};

		sMAqCfg.ucMode = 1;
		sMAqCfg.uc8x8to16x16Th = 2;
		sMAqCfg.ucDqpRoiTh = 3;
		sMAqCfg.cDqp[0] = pRcParam->iMotionAQStrength;
		sMAqCfg.cDqp[1] = pRcParam->iMotionAQStrength;
		sMAqCfg.cDqp[2] = -16;
		sMAqCfg.cDqp[3] = -16;
		sMAqCfg.cDqp[4] = -16;
		sMAqCfg.cDqp[5] = -16;

		h26XEnc_setMotAqCfg(pVar, &sMAqCfg);
	}

	if (pComnCtx->ucRcMode == H26X_RC_FixQp) {
		H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

		pFuncCtx->stRowRc.stCfg.bEnable = 0;
	}

	return H26XENC_SUCCESS;
}
#endif

INT32 h26XEnc_setSdecCfg(H26XENC_VAR *pVar, H26XEncSdeCfg *pSdecCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stSdec.stCfg, pSdecCfg, sizeof(H26XEncSdeCfg));

	h26xEnc_wrapSdeCfg(pRegSet, &pFuncCtx->stSdec);

	return H26XENC_SUCCESS;
}

void h26XEnc_SetIspRatio(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatio)
{	
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

    memcpy(&pComnCtx->stIspRatio, pIspRatio, sizeof(H26XEncIspRatioCfg));
}

UINT32 h26XEnc_getUsrQpAddr(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

	return pFuncCtx->stUsrQp.uiIntQpMapAddr;
}

void h26XEnc_setUsrQpAddr(H26XENC_VAR *pVar, UINT32 uiAddr)
{
	H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;
    pFuncCtx->stUsrQp.uiIntQpMapAddr = uiAddr;
}


INT32 h26XEnc_setBgrCfg(H26XENC_VAR *pVar, H26XEncBgrCfg *pBgrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stBgr.stCfg, pBgrCfg, sizeof(H26XEncBgrCfg));

	h26xEnc_wrapBgrCfg(pRegSet, &pFuncCtx->stBgr);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRmdCfg(H26XENC_VAR *pVar, H26XEncRmdCfg *pIraCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stRmd.stCfg, pIraCfg, sizeof(H26XEncRmdCfg));

	h26xEnc_wrapRmdCfg(pRegSet, &pFuncCtx->stRmd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setTnrCfg(H26XENC_VAR *pVar, H26XEncTnrCfg *pTnrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stTnr.stCfg, pTnrCfg, sizeof(H26XEncTnrCfg));

	h26xEnc_wrapTnrCfg(pRegSet, &pFuncCtx->stTnr);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setLambdaCfg(H26XENC_VAR *pVar, H26XEncLambdaCfg *pLambdaCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pVar->uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stLambda.stCfg, pLambdaCfg, sizeof(H26XEncLambdaCfg));

	h26xEnc_wrapLambdaCfg(pRegSet, &pFuncCtx->stLambda);

	return H26XENC_SUCCESS;
}





#if 0


UINT32 h26xEnc_getVaAPBAddr(H26XENC_VAR *pVar)
{
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;

	return pComnCtx->stVaAddr.uiAPBAddr;
}

UINT32 h26xEnc_getVaLLCAddr(H26XENC_VAR *pVar)
{
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;

	return pComnCtx->stVaAddr.uiLLCAddr;
}

void h26xEnc_getNaluLenResult(H26XENC_VAR *pVar, H26XEncNaluLenResult *pResult)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;

	pResult->uiSliceNum = pFuncCtx->stSliceSplit.uiNaluNum;
	pResult->uiVaAddr = pComnCtx->stVaAddr.uiNaluLen;
}

void h26xEnc_getGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

	memcpy(pGdrCfg, &pFuncCtx->stGdr.stCfg, sizeof(H26XEncGdrCfg));
}

void h26xEnc_getAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

	memcpy(pAqCfg, &pFuncCtx->stAq.stCfg, sizeof(H26XEncAqCfg));
}

UINT32 h26xEnc_getBsLen(H26XENC_VAR *pVar)
{
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;
	UINT32 uiBsLen = h26x_getDbg2(0);
	UINT32 uiDMALen = h26x_getDramBurstLen();

	uiBsLen = (uiBsLen > uiDMALen)? (uiBsLen-uiDMALen):0; // the last bytes have not actually DMA to RAM, so minus

	if (uiBsLen > 0) {
		h26x_flushCache(pComnCtx->stVaAddr.uiBsOutAddr, uiBsLen);
	}

	return uiBsLen;
}
#endif
UINT32 h26xEnc_getMaqMotAddr(H26XENC_VAR *pVar, UINT32 i)
{
	H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

	return pFuncCtx->stMAq.uiMotAddr[i];
}
UINT32 h26xEnc_getMaqMotSize(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

	return pFuncCtx->stMAq.uiMotSize;
}

void h26xEnc_getIspRatioCfg(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatioCfg)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	
	memcpy(pIspRatioCfg, &pComnCtx->stIspRatio, sizeof(H26XEncIspRatioCfg));
}

