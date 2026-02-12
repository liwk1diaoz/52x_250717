#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <string.h>
#endif

#include "kdrv_videoenc/kdrv_videoenc.h"

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_comn.h"
#include "kdrv_vdocdc_emode.h"

#include "vemd_api.h"

#include "h264enc_int.h"
#include "h265enc_int.h"

vemd_info_t g_vemd_info = {0};

int vemd_get_func_info(vemd_func_t *pstfunc)
{	
	if (g_enc_info[g_vemd_info.enc_id].b_enable != TRUE) {
		DBG_ERR("set enc_id(%d) not enable already\r\n", g_vemd_info.enc_id);
		return -1;
	}	
	else {
		H26XENC_VAR *pVar = &g_enc_info[g_vemd_info.enc_id].enc_var;
		H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

		pstfunc->eCodecType = pVar->eCodecType;
		
		memcpy(&pstfunc->stSliceSplit, &pFuncCtx->stSliceSplit, sizeof(H26XEncSliceSplit));
		memcpy(&pstfunc->stGdr, &pFuncCtx->stGdr, sizeof(H26XEncGdr));
		memcpy(&pstfunc->stRoi, &pFuncCtx->stRoi, sizeof(H26XEncRoi));
		memcpy(&pstfunc->stRowRc, &pFuncCtx->stRowRc, sizeof(H26XEncRowRc));
		memcpy(&pstfunc->stAq, &pFuncCtx->stAq, sizeof(H26XEncAq));
		memcpy(&pstfunc->stLpm, &pFuncCtx->stLpm, sizeof(H26XEncLpm));
		memcpy(&pstfunc->stRnd, &pFuncCtx->stRnd, sizeof(h26XEncRnd));
		memcpy(&pstfunc->stMAq, &pFuncCtx->stMAq, sizeof(H26XEncMotAq));
		memcpy(&pstfunc->stJnd, &pFuncCtx->stJnd, sizeof(H26XEncJnd));
		memcpy(&pstfunc->stRqp, &pFuncCtx->stQR, sizeof(H26XEncQpRelatedCfg));
		memcpy(&pstfunc->stVar, &pFuncCtx->stVar, sizeof(H26XEncVarCfg));

		if (pstfunc->eCodecType == VCODEC_H264) {
			H264ENC_CTX   *pVdoCtx = (H264ENC_CTX *)pVar->pVdoCtx;

			memcpy(&pstfunc->stFro.st264, &pVdoCtx->stFroCfg, sizeof(H264EncFroCfg));
			memcpy(&pstfunc->stRdo.st264, &pVdoCtx->stRdo, sizeof(H264EncRdo));
		}
		else if (pstfunc->eCodecType == VCODEC_H265){
			H265ENC_CTX   *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;

			memcpy(&pstfunc->stFro.st265, &pVdoCtx->stFroCfg, sizeof(H265EncFroCfg));
			memcpy(&pstfunc->stRdo.st265, &pVdoCtx->stRdo, sizeof(H265EncRdo));
		}
		else {
			DBG_ERR("codec type not h264 or h265\r\n");
		}
	}

	return 0;
}

