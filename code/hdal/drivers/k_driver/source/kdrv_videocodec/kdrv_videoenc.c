/**
 * @file kdrv_videoenc.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2018
 */

#include "comm/timer.h"
#include "comm/hwclock.h"

#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/util.h"

#include "kdrv_type.h"

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_comn.h"
#include "kdrv_vdocdc_thread.h"

#if defined(__LINUX)
#ifdef VDOCDC_LL
#include "kdrv_vdocdc_thread.h"
#endif
#elif defined(__FREERTOS)
#include <string.h>
#include "nvt_vdocdc_drv.h"
#endif

#include "h26x.h"
#include "h26x_common.h"
#include "h264enc_api.h"
#include "h265enc_api.h"
#include "h26xenc_api.h"
#include "h26xenc_wrap.h"

#include "../kdrv_builtin/nvt_jpg/include/jpeg.h"
#include "../kdrv_builtin/nvt_jpg/include/jpg_enc.h"
#include "h26xenc_rc.h"

#include "vemd_api.h"

#include "kdrv_builtin/jpgenc_builtin.h"
#include "kdrv_builtin/vdoenc_builtin.h"

vdoenc_info g_enc_info[KDRV_VDOENC_ID_MAX];
#ifdef __KERNEL__
BOOL vdoenc_1st_open[BUILTIN_VDOENC_PATH_ID_MAX] = {1, 1, 1, 1, 1, 1};
#endif

H26XEncRowRcCfg g_rrc_cfg = {1,
			{0, 4},
			0,
			{2, 4},
			{1, 1},
			{1, 1},
			{51, 51},
			0};
H26XEncAqCfg g_aq_cfg = {1,
			0,
			3, 3,
			8, -8,
			36, 2, 2, 2,
			{-120,-112,-104,-96,-88,-80,-72,-64,-56,-48,-40,-32,-24,-16,-8,7,15,23,31,39,47,55,63,71,79,87,95,103,111,119},
			0,
			3, 3,
			1, 1
			};
H264EncRdoCfg g_264_rdo_cfg = {1, 8, 8, 8, 8, 8, 8};
H265EncRdoCfg g_265_rdo_cfg = {0, 0, 0, 0, 0, 14, 28, 14, 28, 7};

static UINT64 get_timer(void)
{
	return hwclock_get_counter();
}

INT32 kdrv_videoenc_open(UINT32 chip, UINT32 engine)
{
	if (engine == KDRV_VIDEOCDC_ENGINE0) {
		memset(&g_enc_info, 0, sizeof(vdoenc_info)*KDRV_VDOENC_ID_MAX);

#if defined(__FREERTOS)
		nvt_vdocdc_drv_init();
#endif
	}
	else if (engine == KDRV_VIDEOCDC_ENGINE_JPEG) {
	    jpeg_open();
	}
	else {
		DBG_ERR("kdrv_videoenc_open engine id(%d) failed\r\n", (int)(engine));
	}

	return 0;
}

INT32 kdrv_videoenc_close(UINT32 chip, UINT32 engine)
{
	if (engine == KDRV_VIDEOCDC_ENGINE0) {
#ifdef VDOCDC_LL
		while(kdrv_vdocdc_get_enc_job_done() == FALSE) {
			vos_util_delay_ms(10);
		}
#endif
	    memset(&g_enc_info, 0, sizeof(vdoenc_info)*KDRV_VDOENC_ID_MAX);

#if defined (__FREERTOS)
		nvt_vdocdc_drv_remove();
#endif
	}
    else if (engine == KDRV_VIDEOCDC_ENGINE_JPEG) {
	    jpeg_close();
	}
	else {
		DBG_ERR("kdrv_videoenc_close engine id(%d) failed\r\n", (int)(engine));
	}

	return 0;
}

INT32 kdrv_videoenc_trigger(UINT32 id, KDRV_VDOENC_PARAM *p_enc_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	if (p_enc_param == NULL) {
		DBG_ERR("p_enc_param is NULL\r\n");
		return -1;
	}

	if (KDRV_DEV_ID_ENGINE(id) == KDRV_VIDEOCDC_ENGINE_JPEG) {

		// set p_cb_func as 0 currently, waiting for KH non-blocking implementation
		if (jpeg_add_queue(KDRV_DEV_ID_CHANNEL(id) ,JPEG_CODEC_MODE_ENC,(void *)p_enc_param, p_cb_func, p_user_data) != E_OK) {
			//p_enc_param->encode_err = 1;
			//if (p_cb_func != NULL) {
			//	p_cb_func->callback(p_enc_param, p_user_data);
			//}
			return -1;
		} else {
			//p_enc_param->encode_err = 0;
			//if (p_cb_func != NULL) {
			//	p_cb_func->callback(p_enc_param, p_user_data);
			//}
    		return 0;
		}
	}
	else if (KDRV_DEV_ID_ENGINE(id) == KDRV_VIDEOCDC_ENGINE0) {
		//H26XEncResultCfg sResult = {0};
#ifdef VDOCDC_LL
#else
		UINT32 interrupt = 0;
		UINT32 re_trigger = 0;
#endif

		if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdoenc trigger channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}

		if ((g_vemd_info.enable == TRUE) && (g_vemd_info.enc_id == (int)KDRV_DEV_ID_CHANNEL(id))) {
			H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)g_enc_info[g_vemd_info.enc_id].enc_var.pFuncCtx;
			H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)g_enc_info[g_vemd_info.enc_id].enc_var.pComnCtx;
			H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

			if (g_vemd_info.slice_en == 1) {
				h26XEnc_setSliceSplitCfg(&g_enc_info[g_vemd_info.enc_id].enc_var, &g_vemd_info.func.stSliceSplit.stCfg);
			}

			if (g_vemd_info.gdr_en == 1) {
				h26XEnc_setGdrCfg(&g_enc_info[g_vemd_info.enc_id].enc_var, &g_vemd_info.func.stGdr.stCfg);
			}

			if (g_vemd_info.roi_en == 1) {
				int i;

				for (i = 0; i < H26X_MAX_ROI_W; i++) {
					h26XEnc_setRoiCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, i, &g_vemd_info.func.stRoi.stCfg[i]);
				}
			}

			if (g_vemd_info.rrc_en == 1) {
				memcpy(&pFuncCtx->stRowRc.stCfg, &g_vemd_info.func.stRowRc.stCfg, sizeof(H26XEncRowRcCfg));

				pFuncCtx->stRowRc.ucMinCostTh = g_vemd_info.func.stRowRc.ucMinCostTh;
				if (g_vemd_info.func.stRowRc.ucInitQp != 0)
					pFuncCtx->stRowRc.ucInitQp = g_vemd_info.func.stRowRc.ucInitQp;
				if (g_vemd_info.func.stRowRc.uiPlannedStop != 0)
					pFuncCtx->stRowRc.uiPlannedStop = g_vemd_info.func.stRowRc.uiPlannedStop;
				if (g_vemd_info.func.stRowRc.uiPlannedTop != 0)
					pFuncCtx->stRowRc.uiPlannedTop = g_vemd_info.func.stRowRc.uiPlannedTop;
				if (g_vemd_info.func.stRowRc.uiPlannedBot != 0)
					pFuncCtx->stRowRc.uiPlannedBot = g_vemd_info.func.stRowRc.uiPlannedBot;
				if (g_vemd_info.func.stRowRc.iPrevPicBitsBias[0] != 0)
					pFuncCtx->stRowRc.iPrevPicBitsBias[0] = g_vemd_info.func.stRowRc.iPrevPicBitsBias[0];
				if (g_vemd_info.func.stRowRc.iPrevPicBitsBias[1] != 0)
					pFuncCtx->stRowRc.iPrevPicBitsBias[1] = g_vemd_info.func.stRowRc.iPrevPicBitsBias[1];

				if (g_vemd_info.func.stRowRc.uiPredBits != 0)
					pFuncCtx->stRowRc.uiPredBits = g_vemd_info.func.stRowRc.uiPredBits;
			}

			if (g_vemd_info.aq_en == 1) {
				UINT8 ucAslog2 = pFuncCtx->stAq.stCfg.ucAslog2;

				memcpy(&pFuncCtx->stAq, &g_vemd_info.func.stAq, sizeof(H26XEncAq));

				if (pFuncCtx->stAq.stCfg.ucAslog2 == 0)
					pFuncCtx->stAq.stCfg.ucAslog2 = ucAslog2;

				h26xEnc_wrapAqCfg(pRegSet, &pFuncCtx->stAq);
			}

			if (g_vemd_info.lpm_en == 1) {
				memcpy(&pFuncCtx->stLpm, &g_vemd_info.func.stLpm, sizeof(H26XEncLpm));
				h26xEnc_wrapLpmCfg(pRegSet, &pFuncCtx->stLpm);
			}

			if (g_vemd_info.rnd_en == 1) {
				memcpy(&pFuncCtx->stRnd, &g_vemd_info.func.stRnd, sizeof(h26XEncRnd));
				h26xEnc_wrapRndCfg(pRegSet, &pFuncCtx->stRnd);
			}

			if (g_vemd_info.maq_en == 1) {
				memcpy(&pFuncCtx->stMAq.stCfg, &g_vemd_info.func.stMAq.stCfg, sizeof(H26XEncMotAqCfg));
				h26xEnc_wrapMotAqCfg(pRegSet, &pFuncCtx->stMAq);
			}

			if (g_vemd_info.jnd_en == 1) {
				memcpy(&pFuncCtx->stJnd, &g_vemd_info.func.stJnd, sizeof(H26XEncJnd));
				h26xEnc_wrapJndCfg(pRegSet, &pFuncCtx->stJnd);
			}

			if (g_vemd_info.rqp_en == 1) {
				memcpy(&pFuncCtx->stQR, &g_vemd_info.func.stRqp, sizeof(H26XEncQpRelatedCfg));
				h26xEnc_wrapQpRelatedCfg(pRegSet, &pFuncCtx->stQR);
			}

			if (g_vemd_info.var_en == 1) {
				memcpy(&pFuncCtx->stVar, &g_vemd_info.func.stVar.stCfg, sizeof(H26XEncVarCfg));
				h26xEnc_wrapVarCfg(pRegSet, &pFuncCtx->stVar);
			}

			if (g_vemd_info.fro_en == 1) {
				if (g_vemd_info.func.eCodecType == VCODEC_H264)
					h264Enc_setFroCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &g_vemd_info.func.stFro.st264);
				else
					h265Enc_setFroCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &g_vemd_info.func.stFro.st265);
			}

			if (g_vemd_info.rdo_en == 1) {
				if (g_vemd_info.func.eCodecType == VCODEC_H264)
					h264Enc_InitRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &g_vemd_info.func.stRdo.st264);
				else
					h265Enc_InitRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &g_vemd_info.func.stRdo.st265);
			}

			g_vemd_info.enable = FALSE;
		}

		if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].vdoenc_3dnr_cb != NULL) {
			H26XEncTnrCfg stCfg = {0};
			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].vdoenc_3dnr_cb(KDRV_DEV_ID_CHANNEL(id), (UINT32)&stCfg);
			h26XEnc_setTnrCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
		}

		if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264) {
			H264ENC_INFO sInfo = {0};

			sInfo.ePicType = (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_rst_i_frm) ? IDR_SLICE : h264Enc_getNxtPicType(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);
			sInfo.uiSrcYAddr = p_enc_param->y_addr;
			sInfo.uiSrcCAddr = p_enc_param->c_addr;
			sInfo.uiSrcYLineOffset = p_enc_param->y_line_offset;
			sInfo.uiSrcCLineOffset = p_enc_param->c_line_offset;
			sInfo.bSrcCbCrIv = p_enc_param->src_cbcr_iv;
			sInfo.bSrcOutEn = p_enc_param->src_out_en;
			sInfo.ucSrcOutMode = p_enc_param->src_out_mode;
			sInfo.uiSrcOutYAddr = p_enc_param->src_out_y_addr;
			sInfo.uiSrcOutCAddr = p_enc_param->src_out_c_addr;
			sInfo.uiSrcOutYLineOffset = p_enc_param->src_out_y_line_offset;
			sInfo.uiSrcOutCLineOffset = p_enc_param->src_out_c_line_offset;
			sInfo.uiBsOutBufAddr = p_enc_param->bs_addr_1;
			sInfo.uiBsOutBufSize = p_enc_param->bs_size_1;
			sInfo.uiNalLenOutAddr = p_enc_param->nalu_len_addr;
			sInfo.uiSrcTimeStamp = p_enc_param->time_stamp;
			sInfo.bSkipFrmEn = p_enc_param->skip_frm_en;
			sInfo.bGetSeqHdrEn = IS_ISLICE(sInfo.ePicType);//p_enc_param->get_seq_hdr_en;
			sInfo.SdeCfg.bEnable = p_enc_param->st_sdc.enable;
			sInfo.SdeCfg.uiWidth = p_enc_param->st_sdc.width;
			sInfo.SdeCfg.uiHeight = p_enc_param->st_sdc.height;
			sInfo.SdeCfg.uiYLofst = p_enc_param->st_sdc.y_lofst;
			sInfo.SdeCfg.uiCLofst = p_enc_param->st_sdc.c_lofst;
			sInfo.bSrcD2DEn = p_enc_param->src_d2d_en;
			sInfo.ucSrcD2DMode = p_enc_param->src_d2d_mode;
			sInfo.uiSrcD2DStrpSize[0] = p_enc_param->src_d2d_strp_size[0];
			sInfo.uiSrcD2DStrpSize[1] = p_enc_param->src_d2d_strp_size[1];
			sInfo.uiSrcD2DStrpSize[2] = p_enc_param->src_d2d_strp_size[2];

			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_is_i_frm = IS_ISLICE(sInfo.ePicType);
			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].setup_time = get_timer();

			if (h264Enc_prepareOnePicture(&sInfo, &g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var) == 0) {
				p_enc_param->encode_err = 1;
				if (p_cb_func != NULL) {
					p_cb_func->callback(p_enc_param, p_user_data);
				}
				return -1;
			}
#ifdef VDOCDC_LL
			kdrv_vdocdc_add_job(VDOCDC_ENC_MODE, id, h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var), p_cb_func, p_user_data, (void *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, (void *)p_enc_param);
#else
			{
				H26XEncResultCfg sResult = {0};
				H26XEncNaluLenResult nalu_len_rslt = {0};
				H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.pComnCtx;
				UINT64 sse;

				h26x_lock();
				h26x_enableClk();

				if (H26X_ENC_MODE)
					h26x_setEncDirectRegSet(h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));
				else
					h26x_setEncLLRegSet(1, h26x_getPhyAddr(h26xEnc_getVaLLCAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var)));

				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].start_time = get_timer();

				h26x_start();

				// nt98528 low latency patch //
				if ((h26x_getChipId() == 98528) && (p_enc_param->src_d2d_en)) h26x_low_latency_patch(h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));

				interrupt = h26x_waitINT();
				// re-trigger
				if (interrupt == H26X_BSOUT_INT && p_enc_param->bs_addr_2 && (p_enc_param->bs_size_2 >> 7)) {
					DBG_DUMP("[%d]bsout buffer full, re-trigger\r\n", (int)KDRV_DEV_ID_CHANNEL(id));
					re_trigger = 1;
					h26x_setNextBsBuf(h26x_getPhyAddr(p_enc_param->bs_addr_2), (p_enc_param->bs_size_2 >> 7) << 7);
					interrupt = h26x_waitINT();
				}

				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].end_time = get_timer();

				h264Enc_getResult(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, H26X_ENC_MODE, &sResult, interrupt, re_trigger, &sInfo, p_enc_param->bs_addr_2);

				if (interrupt & H26X_BSOUT_INT) {
					UINT32 uiEncRatio;
					H26XENC_VAR *pVar = (H26XENC_VAR *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var;
					H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
					uiEncRatio = h264Enc_getEncodeRatio(pVar);
					pComnCtx->uiEncRatio = uiEncRatio;
				} else {
					H26XENC_VAR *pVar = (H26XENC_VAR *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var;
					H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
					pComnCtx->uiEncRatio = 0;
				}

				p_enc_param->temproal_id = sResult.uiSvcLable;
				p_enc_param->encode_err = ((interrupt & h26x_getIntEn()) != 0x00000001);
				p_enc_param->re_encode_en = (interrupt != H26X_FINISH_INT);
				p_enc_param->nxt_frm_type = sResult.ucNxtPicType;
				p_enc_param->base_qp = sResult.ucQP;
				p_enc_param->bs_size_1 = sResult.uiBSLen;
				p_enc_param->frm_type = sResult.ucPicType;
				p_enc_param->encode_time = sResult.uiHwEncTime;
				p_enc_param->motion_ratio = sResult.uiMotionRatio;
				#if defined(__LINUX)
				sse = (((UINT64)sResult.uiYPSNR[1])<<32) | sResult.uiYPSNR[0];
				p_enc_param->y_mse = (UINT32)div_u64(sse, pComnCtx->uiWidth * pComnCtx->uiHeight);
				sse = (((UINT64)sResult.uiUPSNR[1])<<32) | sResult.uiUPSNR[0];
				p_enc_param->u_mse = (UINT32)div_u64(sse, (pComnCtx->uiWidth / 2) * (pComnCtx->uiHeight / 2));
				sse = (((UINT64)sResult.uiVPSNR[1])<<32) | sResult.uiVPSNR[0];
				p_enc_param->v_mse = (UINT32)div_u64(sse, (pComnCtx->uiWidth / 2) * (pComnCtx->uiHeight / 2));
				#else
				sse = (((UINT64)sResult.uiYPSNR[1])<<32) | sResult.uiYPSNR[0];
				p_enc_param->y_mse = (UINT32)(sse / pComnCtx->uiWidth / pComnCtx->uiHeight);
				sse = (((UINT64)sResult.uiUPSNR[1])<<32) | sResult.uiUPSNR[0];
				p_enc_param->u_mse = (UINT32)(sse / (pComnCtx->uiWidth / 2) / (pComnCtx->uiHeight / 2));
				sse = (((UINT64)sResult.uiVPSNR[1])<<32) | sResult.uiVPSNR[0];
				p_enc_param->v_mse = (UINT32)(sse / (pComnCtx->uiWidth / 2) / (pComnCtx->uiHeight / 2));
				#endif
				p_enc_param->evbr_still_flag = sResult.bEVBRStillFlag;

				// nalu len
				h26xEnc_getNaluLenResult(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &nalu_len_rslt);

				p_enc_param->nalu_num       = nalu_len_rslt.uiSliceNum;
				p_enc_param->nalu_size_addr = nalu_len_rslt.uiVaAddr;
			}

			if (p_cb_func != NULL) {
				p_cb_func->callback(p_enc_param, p_user_data);
			}
#endif
		}
        else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
			H265ENC_INFO sInfo = {0};

			sInfo.ePicType = (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_rst_i_frm) ? IDR_SLICE :
                                h265Enc_getNxtPicType(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);

			sInfo.uiSrcYAddr = p_enc_param->y_addr;
			sInfo.uiSrcCAddr = p_enc_param->c_addr;
			sInfo.uiSrcYLineOffset = p_enc_param->y_line_offset;
			sInfo.uiSrcCLineOffset = p_enc_param->c_line_offset;
			sInfo.bSrcCbCrIv = p_enc_param->src_cbcr_iv;
			sInfo.bSrcOutEn = p_enc_param->src_out_en;
			sInfo.ucSrcOutMode = p_enc_param->src_out_mode;
			sInfo.uiSrcOutYAddr = p_enc_param->src_out_y_addr;
			sInfo.uiSrcOutCAddr = p_enc_param->src_out_c_addr;
			sInfo.uiSrcOutYLineOffset = p_enc_param->src_out_y_line_offset;
			sInfo.uiSrcOutCLineOffset = p_enc_param->src_out_c_line_offset;
			sInfo.uiBsOutBufAddr = p_enc_param->bs_addr_1;
			sInfo.uiBsOutBufSize = p_enc_param->bs_size_1;
			sInfo.uiNalLenOutAddr = p_enc_param->nalu_len_addr;
			sInfo.uiSrcTimeStamp = p_enc_param->time_stamp;
			sInfo.bSkipFrmEn = p_enc_param->skip_frm_en;
			sInfo.bGetSeqHdrEn = IS_ISLICE(sInfo.ePicType);//p_enc_param->get_seq_hdr_en;
			sInfo.SdeCfg.bEnable = p_enc_param->st_sdc.enable;
			sInfo.SdeCfg.uiWidth = p_enc_param->st_sdc.width;
			sInfo.SdeCfg.uiHeight = p_enc_param->st_sdc.height;
			sInfo.SdeCfg.uiYLofst = p_enc_param->st_sdc.y_lofst;
			sInfo.SdeCfg.uiCLofst = p_enc_param->st_sdc.c_lofst;
			sInfo.bSrcD2DEn = p_enc_param->src_d2d_en;
			sInfo.ucSrcD2DMode = p_enc_param->src_d2d_mode;
			sInfo.uiSrcD2DStrpSize[0] = p_enc_param->src_d2d_strp_size[0];
			sInfo.uiSrcD2DStrpSize[1] = p_enc_param->src_d2d_strp_size[1];
			sInfo.uiSrcD2DStrpSize[2] = p_enc_param->src_d2d_strp_size[2];

			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_is_i_frm = IS_ISLICE(sInfo.ePicType);
			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].setup_time = get_timer();

			if (h265Enc_prepareOnePicture(&sInfo, &g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var) == 0) {
				p_enc_param->encode_err = 1;
				if (p_cb_func != NULL) {
					p_cb_func->callback(p_enc_param, p_user_data);
				}
				return -1;
			}
#ifdef VDOCDC_LL
			kdrv_vdocdc_add_job(VDOCDC_ENC_MODE, id, h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var), p_cb_func, p_user_data, (void *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, (void *)p_enc_param);
#else
			{
				H26XEncResultCfg sResult = {0};
				H26XEncNaluLenResult nalu_len_rslt = {0};
				H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.pComnCtx;
				UINT64 sse;

				h26x_lock();
				h26x_enableClk();

				if (H26X_ENC_MODE)
					h26x_setEncDirectRegSet(h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));
				else
					h26x_setEncLLRegSet(1, h26x_getPhyAddr(h26xEnc_getVaLLCAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var)));

				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].start_time = get_timer();

				h26x_start();
				// nt98528 low latency patch //
				if ((h26x_getChipId() == 98528) && (p_enc_param->src_d2d_en)) h26x_low_latency_patch(h26xEnc_getVaAPBAddr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));

				interrupt = h26x_waitINT();
				// re-trigger
				if (interrupt == H26X_BSOUT_INT && p_enc_param->bs_addr_2 && (p_enc_param->bs_size_2 >> 7)) {
					DBG_DUMP("[%d]bsout buffer full, re-trigger\r\n", (int)KDRV_DEV_ID_CHANNEL(id));
					re_trigger = 1;
					h26x_setNextBsBuf(h26x_getPhyAddr(p_enc_param->bs_addr_2), (p_enc_param->bs_size_2 >> 7) << 7);
					interrupt = h26x_waitINT();
				}

				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].end_time = get_timer();

				h265Enc_getResult(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, H26X_ENC_MODE, &sResult, interrupt, re_trigger, &sInfo, p_enc_param->bs_addr_2);

				if (interrupt & H26X_BSOUT_INT) {
					UINT32 uiEncRatio;
					H26XENC_VAR *pVar = (H26XENC_VAR *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var;
					H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
					uiEncRatio = h265Enc_getEncodeRatio(pVar);
					pComnCtx->uiEncRatio = uiEncRatio;
				} else {
					H26XENC_VAR *pVar = (H26XENC_VAR *)&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var;
					H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
					pComnCtx->uiEncRatio = 0;
				}

				p_enc_param->temproal_id = sResult.uiSvcLable;
				p_enc_param->encode_err = ((interrupt & h26x_getIntEn()) != 0x00000001);
				p_enc_param->re_encode_en = (interrupt != H26X_FINISH_INT);
				p_enc_param->nxt_frm_type = sResult.ucNxtPicType;
				p_enc_param->base_qp = sResult.ucQP;
				p_enc_param->bs_size_1 = sResult.uiBSLen;
				p_enc_param->frm_type = sResult.ucPicType;
				p_enc_param->encode_time = sResult.uiHwEncTime;
				p_enc_param->motion_ratio = sResult.uiMotionRatio;
				#if defined(__LINUX)
				sse = (((UINT64)sResult.uiYPSNR[1])<<32) | sResult.uiYPSNR[0];
				p_enc_param->y_mse = (UINT32)div_u64(sse, pComnCtx->uiWidth * pComnCtx->uiHeight);
				sse = (((UINT64)sResult.uiUPSNR[1])<<32) | sResult.uiUPSNR[0];
				p_enc_param->u_mse = (UINT32)div_u64(sse, (pComnCtx->uiWidth / 2) * (pComnCtx->uiHeight / 2));
				sse = (((UINT64)sResult.uiVPSNR[1])<<32) | sResult.uiVPSNR[0];
				p_enc_param->v_mse = (UINT32)div_u64(sse, (pComnCtx->uiWidth / 2) * (pComnCtx->uiHeight / 2));
				#else
				sse = (((UINT64)sResult.uiYPSNR[1])<<32) | sResult.uiYPSNR[0];
				p_enc_param->y_mse = (UINT32)(sse / pComnCtx->uiWidth / pComnCtx->uiHeight);
				sse = (((UINT64)sResult.uiUPSNR[1])<<32) | sResult.uiUPSNR[0];
				p_enc_param->u_mse = (UINT32)(sse / (pComnCtx->uiWidth / 2) / (pComnCtx->uiHeight / 2));
				sse = (((UINT64)sResult.uiVPSNR[1])<<32) | sResult.uiVPSNR[0];
				p_enc_param->v_mse = (UINT32)(sse / (pComnCtx->uiWidth / 2) / (pComnCtx->uiHeight / 2));
				#endif
				p_enc_param->evbr_still_flag = sResult.bEVBRStillFlag;

				// nalu len
				h26xEnc_getNaluLenResult(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &nalu_len_rslt);

				p_enc_param->nalu_num       = nalu_len_rslt.uiSliceNum;
				p_enc_param->nalu_size_addr = nalu_len_rslt.uiVaAddr;
			}

			if (p_cb_func != NULL) {
				p_cb_func->callback(p_enc_param, p_user_data);
			}
#endif
		}
		else {
			DBG_ERR("encode trigger picture type(%d) error\r\n", (int)(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType));
		}
#ifdef VDOCDC_LL
#else
		#if 0
		DBG_DUMP("(%8x)(%d, %d)\r\n", (int)interrupt,(int)(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].start_time - g_enc_info[KDRV_DEV_ID_CHANNEL(id)].setup_time),
												(int)(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].end_time - g_enc_info[KDRV_DEV_ID_CHANNEL(id)].start_time));
		#endif
		if ((interrupt & h26x_getIntEn()) != 0x00000001)  {
			DBG_ERR("[%d]encode error interrupt(0x%08x)\r\n", (int)KDRV_DEV_ID_CHANNEL(id), (int)interrupt);
			h26x_module_reset();
			//h26x_prtReg();
			h26x_disableClk();
			h26x_unlock();
			return -1;
		}

		h26x_disableClk();
		h26x_unlock();

		if ((interrupt & H26X_SRC_DECMP_ERR_INT) != 0) {
			DBG_WARN("source decompress error interrupt(0x%08x)\r\n", (int)interrupt);
		}
#endif
		g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_rst_i_frm = FALSE;

		p_enc_param->re_trigger = re_trigger;
	}
	else {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)(KDRV_DEV_ID_ENGINE(id)));
		return -1;
	}
	return 0;
}

INT32 kdrv_videoenc_get(UINT32 id, KDRV_VDOENC_GET_PARAM_ID parm_id, VOID *param)
{
	INT32 ret = H26XENC_SUCCESS;

	if (parm_id != VDOENC_GET_JPEG_FREQ && parm_id != VDOENC_GET_MEM_SIZE && parm_id != VDOENC_GET_LL_MEM_SIZE &&
		parm_id != VDOENC_GET_AQ && parm_id != VDOENC_GET_ROWRC && parm_id != VDOENC_GET_RDO && parm_id != VDOENC_GET_RECONFRM_SIZE &&
		parm_id != VDOENC_GET_RECONFRM_NUM) {
		if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdoenc get channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}
	}

	if (parm_id == VDOENC_GET_COE) {
		//coe_get_config((PK_COE_CONFIG) param);
	} else if (parm_id == VDOENC_GET_JPEG_FREQ) {
		UINT32 *p_freq = param;
		*p_freq = jpeg_get_config(JPEG_CONFIG_ID_FREQ);
	} else {
		switch(parm_id) {
			case VDOENC_GET_MEM_SIZE:
				{
					KDRV_VDOENC_MEM_INFO *pinfo = (KDRV_VDOENC_MEM_INFO *)param;
					H26XEncMeminfo sInfo = {0};

					sInfo.uiWidth = pinfo->width;
					sInfo.uiHeight = pinfo->height;
					sInfo.ucSVCLayer = pinfo->svc_layer;
					sInfo.uiLTRInterval = pinfo->ltr_interval;
					//as ori
					sInfo.ucQualityLevel = pinfo->quality_level;
					sInfo.bTileEn = pinfo->tile_mode_en;
					sInfo.bD2dEn = pinfo->d2d_mode_en;
					sInfo.bGdcEn = pinfo->gdc_mode_en;
					sInfo.bColMvEn = pinfo->colmv_en;
					sInfo.bCommReconFrmBuf = pinfo->comm_recfrm_en;
					sInfo.bGDRIFrm = pinfo->gdr_i_frm_en;

					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)
						pinfo->size = h264Enc_queryMemSize(&sInfo);
					else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265)
						pinfo->size = h265Enc_queryMemSize(&sInfo);
				}
				break;
			case VDOENC_GET_DESC:
				{
					KDRV_VDOENC_DESC *pinfo = (KDRV_VDOENC_DESC *)param;

					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)
						h264Enc_getSeqHdr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &pinfo->addr, &pinfo->size);
					else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265)
						h265Enc_getSeqHdr(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &pinfo->addr, &pinfo->size);
				}
				break;
			case VDOENC_GET_ISIFRAME:
				*(UINT32 *)param = g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_is_i_frm;
				break;
			case VDOENC_GET_GOPNUM:
				{
					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)
						*(UINT32 *)param = h264Enc_getGopNum(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);
				}
				break;
			case VDOENC_GET_CBR:
			case VDOENC_GET_EVBR:
			case VDOENC_GET_VBR:
			case VDOENC_GET_FIXQP:
			case VDOENC_GET_RC:
				DBG_ERR("vdoenc get not ready id(%d) (TODO)\r\n", (int)(parm_id));
				break;
			case VDOENC_GET_3DNR:
				DBG_ERR("vdoenc get not suppot id(%d)\r\n", (int)(parm_id));
				break;
			case VDOENC_GET_GDR:
				{
					KDRV_VDOENC_GDR *pinfo = (KDRV_VDOENC_GDR *)param;
					H26XEncGdrCfg sCfg = {0};

					h26xEnc_getGdrCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);

					pinfo->enable = sCfg.bEnable;
					pinfo->period = sCfg.usPeriod;
					pinfo->number = sCfg.usNumber;
					pinfo->enable_gdr_i_frm= sCfg.bGDRIFrm;
				}
				break;
			case VDOENC_GET_AQ:
				{
					KDRV_VDOENC_AQ *pinfo = (KDRV_VDOENC_AQ *)param;
					H26XEncAqCfg sCfg = {0};

					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
						memcpy(&sCfg, &g_aq_cfg, sizeof(H26XEncAqCfg));
					} else {
						h26xEnc_getAqCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
					}

					pinfo->enable = sCfg.bEnable;
					pinfo->i_str = sCfg.ucIStr;
					pinfo->p_str = sCfg.ucPStr;
					pinfo->max_delta_qp = sCfg.cMaxDQp;
					pinfo->min_delta_qp = sCfg.cMinDQp;

					#if 0
					if (h26x_getChipId() == 98528) {
						pinfo->mode = sCfg.bAqMode;
						pinfo->i_str1 = sCfg.ucIStr1;
						pinfo->p_str1 = sCfg.ucPStr1;
						pinfo->i_str2 = sCfg.ucIStr2;
						pinfo->p_str2 = sCfg.ucPStr2;
					}
					#endif
				}
				break;
			case VDOENC_GET_BS_LEN:
				{
					*(UINT32 *)param = h26xEnc_getBsLen(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);
				}
				break;
			case VDOENC_GET_ISP_RATIO:
				{
					KDRV_VDOENC_ISP_RATIO *pinfo = (KDRV_VDOENC_ISP_RATIO *)param;
					H26XEncIspRatioCfg sCfg = {0};

					h26xEnc_getIspRatioCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);

					pinfo->ratio_base  = sCfg.ucRatioBse;
					pinfo->edge_ratio  = sCfg.ucEdgeRatio;
					pinfo->dn_2d_ratio = sCfg.ucDn2DRatio;
					pinfo->dn_3d_ratio = sCfg.ucDn3DRatio;
				}
				break;
			case VDOENC_GET_ROWRC:
				{
					KDRV_VDOENC_ROW_RC *pinfo = (KDRV_VDOENC_ROW_RC *)param;
					H26XEncRowRcCfg sCfg = {0};

					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
						memcpy(&sCfg, &g_rrc_cfg, sizeof(H26XEncRowRcCfg));
					} else {
						h26xEnc_getRowRcCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
					}

					pinfo->enable  = sCfg.bEnable;
					pinfo->i_qp_range = sCfg.ucQPRange[0];
					pinfo->p_qp_range = sCfg.ucQPRange[1];
					pinfo->i_qp_step = sCfg.ucQPStep[0];
					pinfo->p_qp_step = sCfg.ucQPStep[1];
					pinfo->min_i_qp = sCfg.ucMinQP[0];
					pinfo->min_p_qp = sCfg.ucMinQP[1];
					pinfo->max_i_qp = sCfg.ucMaxQP[0];
					pinfo->max_p_qp = sCfg.ucMaxQP[1];
				}
				break;
			case VDOENC_GET_RDO:
				{
					KDRV_VDOENC_RDO *pinfo = (KDRV_VDOENC_RDO *)param;
					pinfo->rdo_codec = (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)? VDOENC_RDO_CODEC_264: VDOENC_RDO_CODEC_265;

					if (pinfo->rdo_codec == VDOENC_RDO_CODEC_264) {
						H264EncRdoCfg stCfg = {0};

						if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
							memcpy(&stCfg, &g_264_rdo_cfg, sizeof(H264EncRdoCfg));
						} else {
							h264Enc_getRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
						}

						pinfo->rdo_param.rdo_264.avc_intra_4x4_cost_bias = stCfg.ucI_Y4_CB_P;
						pinfo->rdo_param.rdo_264.avc_intra_8x8_cost_bias = stCfg.ucI_Y8_CB_P;
						pinfo->rdo_param.rdo_264.avc_intra_16x16_cost_bias = stCfg.ucI_Y16_CB_P;
						pinfo->rdo_param.rdo_264.avc_inter_tu4_cost_bias = stCfg.ucP_Y4_CB;
						pinfo->rdo_param.rdo_264.avc_inter_tu8_cost_bias = stCfg.ucP_Y8_CB;
						pinfo->rdo_param.rdo_264.avc_inter_skip_cost_bias = stCfg.ucIP_CB_SKIP;
						pinfo->rdo_param.rdo_264.avc_inter_luma_coeff_cost_th = stCfg.ucP_Y_COEFF_COST_TH;
						pinfo->rdo_param.rdo_264.avc_inter_chroma_coeff_cost_th = stCfg.ucP_C_COEFF_COST_TH;
					} else if (pinfo->rdo_codec == VDOENC_RDO_CODEC_265) {
						H265EncRdoCfg stCfg = {0};

						if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
							memcpy(&stCfg, &g_265_rdo_cfg, sizeof(H265EncRdoCfg));
						} else {
							h265Enc_getRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
						}

						pinfo->rdo_param.rdo_265.hevc_intra_32x32_cost_bias = stCfg.global_motion_penalty_I32;
						pinfo->rdo_param.rdo_265.hevc_intra_16x16_cost_bias = stCfg.global_motion_penalty_I16;
						pinfo->rdo_param.rdo_265.hevc_intra_8x8_cost_bias = stCfg.global_motion_penalty_I08;
						pinfo->rdo_param.rdo_265.hevc_inter_skip_cost_bias = stCfg.cost_bias_skip;
						pinfo->rdo_param.rdo_265.hevc_inter_merge_cost_bias = stCfg.cost_bias_merge;
						pinfo->rdo_param.rdo_265.hevc_inter_64x64_cost_bias = stCfg.ime_scale_64c;
						pinfo->rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias = stCfg.ime_scale_64p;
						pinfo->rdo_param.rdo_265.hevc_inter_32x32_cost_bias = stCfg.ime_scale_32c;
						pinfo->rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias = stCfg.ime_scale_32p;
						pinfo->rdo_param.rdo_265.hevc_inter_16x16_cost_bias = stCfg.ime_scale_16c;
					}
				}
				break;
			case VDOENC_GET_LL_MEM_SIZE:
				{
					KDRV_VDOENC_LL_MEM_INFO *pinfo = (KDRV_VDOENC_LL_MEM_INFO *)param;
#ifdef VDOCDC_LL
					pinfo->size = kdrv_vdocdc_get_llc_mem();
#else
					pinfo->size = 0;
#endif
				}
				break;
			case VDOENC_GET_SRCOUTYUV_WH:
				{
					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264) {
						((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_w = SIZE_32X(((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_w);
						((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_h = SIZE_16X(((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_h);
					} else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
						((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_w = SIZE_64X(((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_w);
						((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_h = SIZE_64X(((KDRV_VDOENC_SRCOUTYUV_WH *)param)->align_h);
					}
				}
				break;
			case VDOENC_GET_RECONFRM_SIZE:
				{
					KDRV_VDOENC_MEM_INFO *pinfo = (KDRV_VDOENC_MEM_INFO *)param;
					H26XEncMeminfo sinfo = {0};

					sinfo.uiWidth = pinfo->width;
					sinfo.uiHeight = pinfo->height;
					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264) {
						pinfo->recfrm_size = h264Enc_queryRecFrmSize(&sinfo);
					} else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
						pinfo->recfrm_size = h265Enc_queryRecFrmSize(&sinfo);
					}
				}
				break;
			case VDOENC_GET_RECONFRM_NUM:
				{
					KDRV_VDOENC_MEM_INFO *pinfo = (KDRV_VDOENC_MEM_INFO *)param;
					H26XEncMeminfo sinfo = {0};

					sinfo.ucSVCLayer = pinfo->svc_layer;
					sinfo.uiLTRInterval = pinfo->ltr_interval;
					pinfo->recfrm_num = h26xEnc_queryRecFrmNum(&sinfo);
				}
				break;
			default:
				break;
		}
	}

	return (ret != H26XENC_SUCCESS);
}

INT32 kdrv_videoenc_set(UINT32 id, KDRV_VDOENC_SET_PARAM_ID parm_id, VOID *param)
{
	INT32 ret = H26XENC_SUCCESS;

	if (parm_id != VDOENC_SET_JPEG_FREQ && parm_id != VDOENC_SET_CODEC && parm_id != VDOENC_SET_INIT && parm_id != VDOENC_SET_LL_MEM) {
		if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdoenc set channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}
	}

	switch (parm_id) {
		case VDOENC_SET_JPEG_FREQ:
			{
				UINT32 freq = *((UINT32*)param);
				jpeg_set_config(JPEG_CONFIG_ID_FREQ, freq);
				DBG_ERR("kdrv_videoenc_set - VDOENC_SET_JPEG_FREQ\r\n");
			}
			break;
		case VDOENC_SET_CODEC:
			if (*(KDRV_VDOENC_TYPE *)param == VDOENC_TYPE_JPEG)
				;
			else if (*(KDRV_VDOENC_TYPE *)param == VDOENC_TYPE_H264)
				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType = VCODEC_H264;
			else if (*(KDRV_VDOENC_TYPE *)param == VDOENC_TYPE_H265)
				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType = VCODEC_H265;
			else
				DBG_ERR("kdrv_vidoeenc_set codec(%d) failed\r\n", (int)(*(KDRV_VDOENC_TYPE *)param));
			break;
		case VDOENC_SET_INIT:
			{
				KDRV_VDOENC_INIT *pinfo = (KDRV_VDOENC_INIT *)param;
				KDRV_VDOENC_PARAM *p_jpgenc_param = (KDRV_VDOENC_PARAM *)param;
				g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.uiEncId = KDRV_DEV_ID_CHANNEL(id);

				if (KDRV_DEV_ID_ENGINE(id) == KDRV_VIDEOCDC_ENGINE_JPEG) {
					if(KDRV_DEV_ID_CHANNEL(id)<(KDRV_VDOENC_ID_MAX+1))
					{
						jpg_enc_setdata(KDRV_DEV_ID_CHANNEL(id),JPG_TARGET_RATE,p_jpgenc_param->target_rate);
						jpg_enc_setdata(KDRV_DEV_ID_CHANNEL(id),JPG_VBR_QUALITY,p_jpgenc_param->quality);
					}
					else
					{
					    //invalid id
						ret = H26XENC_FAIL;
					}
					break;
				}

				if (h26x_efuse_check(pinfo->width, pinfo->height) != TRUE) {
					ret = H26XENC_FAIL;
				}
				else {
					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264) {
						H264ENC_INIT sInit = {0};
#ifdef __KERNEL__
						UINT32 pathID = KDRV_DEV_ID_CHANNEL(id);
						if (pinfo->builtin_init && vdoenc_1st_open[pathID]) {
							g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = TRUE;
							//DBG_DUMP("[%d] H264  KDRV init OK!!!\r\n", (UINT32)KDRV_DEV_ID_CHANNEL(id));
							//copy builtin enc_var to g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var
							VdoEnc_Builtin_GetEncVar((UINT32)KDRV_DEV_ID_CHANNEL(id), &(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));
							vdoenc_1st_open[pathID] = FALSE;
							return H26XENC_SUCCESS;
						}
#endif
						sInit.uiDisplayWidth = (pinfo->rotate == 1 || pinfo->rotate == 2) ? pinfo->height : pinfo->width;
						sInit.uiWidth  = ALIGN_CEIL_16(sInit.uiDisplayWidth);
						sInit.uiHeight = (pinfo->rotate == 1 || pinfo->rotate == 2) ? pinfo->width : pinfo->height;

						sInit.uiEncBufAddr = pinfo->buf_addr;
						sInit.uiEncBufSize = pinfo->buf_size;
						sInit.uiGopNum = pinfo->gop;
						sInit.ucDisLFIdc = pinfo->disable_db & 0x3; // remove 3'b100 (TileMode), which is H265 only
						sInit.cDBAlpha = pinfo->db_alpha;
						sInit.cDBBeta = pinfo->db_beta;
						sInit.cChrmQPIdx = pinfo->chrm_qp_idx;
						sInit.cSecChrmQPIdx = pinfo->sec_chrm_qp_idx;

						sInit.ucSVCLayer = pinfo->e_svc;
						sInit.uiLTRInterval = pinfo->ltr_interval;
						sInit.bLTRPreRef = pinfo->ltr_pre_ref;

						sInit.uiBitRate = pinfo->byte_rate*8;
						sInit.uiFrmRate = pinfo->frame_rate;

						sInit.ucIQP = pinfo->init_i_qp;
						sInit.ucPQP = pinfo->init_p_qp;
						sInit.ucMaxIQp = pinfo->max_i_qp;
						sInit.ucMinIQp = pinfo->min_i_qp;
						sInit.ucMaxPQp = pinfo->max_p_qp;
						sInit.ucMinPQp = pinfo->min_p_qp;
						sInit.iIPWeight = pinfo->ip_weight;
						sInit.uiStaticTime = pinfo->static_time;

						sInit.bFBCEn = (pinfo->bGDRIFrm == 1)? 0: pinfo->bFBCEn;
						sInit.bGrayEn = pinfo->gray_en;
						sInit.bFastSearchEn = 0;//pinfo->fast_search;	// nt98520 not support search range set to 1 : big range //
						sInit.bHwPaddingEn = pinfo->hw_padding_en;
						sInit.ucRotate = pinfo->rotate;
						sInit.bD2dEn = pinfo->bD2dEn;
						sInit.bColMvEn = pinfo->colmv_en;

						sInit.bGDRIFrm = pinfo->bGDRIFrm;

						switch(pinfo->e_profile) {
							case KDRV_VDOENC_PROFILE_BASELINE:
								sInit.eProfile = PROFILE_BASELINE;
								break;
							case KDRV_VDOENC_PROFILE_MAIN:
								sInit.eProfile = PROFILE_MAIN;
								break;
							case KDRV_VDOENC_PROFILE_HIGH:
								sInit.eProfile = PROFILE_HIGH;
								break;
							default:
							{
								DBG_ERR("init profile(%d) error, set to profile high10\r\n", (int)(pinfo->e_profile));
								sInit.eProfile = PROFILE_HIGH10;
							}
							break;
						}
						sInit.eEntropyMode = pinfo->e_entropy;
						sInit.ucLevelIdc = pinfo->level_idc;
						sInit.bTrans8x8En = 1;
						sInit.bForwardRecChrmEn = 0;

						sInit.bVUIEn = pinfo->bVUIEn;
						sInit.usSarWidth = pinfo->sar_width;
						sInit.usSarHeight = pinfo->sar_height;
						sInit.ucMatrixCoef = pinfo->matrix_coef;
						sInit.ucTransferCharacteristics = pinfo->transfer_characteristics;
						sInit.ucColourPrimaries = pinfo->colour_primaries;
						sInit.ucVideoFormat = pinfo->video_format;
						sInit.ucColorRange = pinfo->color_range;
						sInit.bTimeingPresentFlag = pinfo->time_present_flag;

						sInit.bRecBufComm = pinfo->comm_recfrm_en;
						sInit.uiRecBufAddr[0] = pinfo->recfrm_addr[0];
						sInit.uiRecBufAddr[1] = pinfo->recfrm_addr[1];
						sInit.uiRecBufAddr[2] = pinfo->recfrm_addr[2];

						sInit.frm_crop_info.frm_crop_enable = pinfo->frm_crop_info.frm_crop_enable;
						sInit.frm_crop_info.display_w = (pinfo->rotate == 1 || pinfo->rotate == 2)? pinfo->frm_crop_info.display_h: pinfo->frm_crop_info.display_w;
						sInit.frm_crop_info.display_h = (pinfo->rotate == 1 || pinfo->rotate == 2)? pinfo->frm_crop_info.display_w: pinfo->frm_crop_info.display_h;

						ret = h264Enc_initEncoder(&sInit, &g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);
					}
	                	else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
	    				H265ENC_INIT sInit = {0};
#ifdef __KERNEL__
						UINT32 pathID = KDRV_DEV_ID_CHANNEL(id);
						if (pinfo->builtin_init && vdoenc_1st_open[pathID]) {
							g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = TRUE;
							//DBG_DUMP("[%d] H265  KDRV init OK!!!\r\n", (UINT32)KDRV_DEV_ID_CHANNEL(id));
							//copy builtin enc_var to g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var
							VdoEnc_Builtin_GetEncVar((UINT32)KDRV_DEV_ID_CHANNEL(id), &(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var));
							vdoenc_1st_open[pathID] = FALSE;
							return H26XENC_SUCCESS;
						}
#endif

						sInit.uiDisplayWidth = (pinfo->rotate == 1 || pinfo->rotate == 2) ? pinfo->height : pinfo->width;
						sInit.uiWidth  = ALIGN_CEIL_16(sInit.uiDisplayWidth);
						sInit.uiHeight = (pinfo->rotate == 1 || pinfo->rotate == 2) ? pinfo->width : pinfo->height;

						sInit.bTileEn = pinfo->bTileEn;
						sInit.eQualityLvl = pinfo->quality_level;
						sInit.uiEncBufAddr = pinfo->buf_addr;
						sInit.uiEncBufSize = pinfo->buf_size;
						sInit.uiGopNum = pinfo->gop;
						sInit.uiUsrQpSize = 0;

						sInit.ucDisableDB = pinfo->disable_db;
						sInit.cDBAlpha = pinfo->db_alpha;
						sInit.cDBBeta = pinfo->db_beta;
						sInit.iQpCbOffset = pinfo->chrm_qp_idx;
						sInit.iQpCrOffset= pinfo->sec_chrm_qp_idx;

						sInit.ucSVCLayer = pinfo->e_svc;
						sInit.uiLTRInterval = pinfo->ltr_interval;
						sInit.uiLTRPreRef = pinfo->ltr_pre_ref;

						sInit.uiSAO = pinfo->sao_en;
						sInit.uiSaoLumaFlag = pinfo->sao_luma_flag;
						sInit.uiSaoChromaFlag = pinfo->sao_chroma_flag;

						sInit.uiBitRate = pinfo->byte_rate*8;
						sInit.uiFrmRate = pinfo->frame_rate;

						sInit.ucIQP = pinfo->init_i_qp;
						sInit.ucPQP = pinfo->init_p_qp;
						sInit.ucMaxIQp = pinfo->max_i_qp;
						sInit.ucMinIQp = pinfo->min_i_qp;
						sInit.ucMaxPQp = pinfo->max_p_qp;
						sInit.ucMinPQp = pinfo->min_p_qp;
						sInit.iIPQPoffset = pinfo->ip_weight;
						sInit.uiStaticTime = pinfo->static_time;

						sInit.bFBCEn = pinfo->bFBCEn;
						sInit.bGrayEn = pinfo->gray_en;
						sInit.bFastSearchEn = 0;//pinfo->fast_search;	// nt98520 not support search range set to 1 : big range //
						sInit.bHwPaddingEn = pinfo->hw_padding_en;
						sInit.ucRotate = pinfo->rotate;
						sInit.bD2dEn = pinfo->bD2dEn;
						sInit.bGdcEn = pinfo->gdc_mode_en;
						sInit.bColMvEn = pinfo->colmv_en;
						sInit.uiMultiTLayer = pinfo->multi_layer;
						sInit.ucLevelIdc = pinfo->level_idc;

						sInit.bVUIEn = pinfo->bVUIEn;
						sInit.usSarWidth = pinfo->sar_width;
						sInit.usSarHeight = pinfo->sar_height;
						sInit.ucMatrixCoef = pinfo->matrix_coef;
						sInit.ucTransferCharacteristics = pinfo->transfer_characteristics;
						sInit.ucColourPrimaries = pinfo->colour_primaries;
						sInit.ucVideoFormat = pinfo->video_format;
						sInit.ucColorRange = pinfo->color_range;
						sInit.bTimeingPresentFlag = pinfo->time_present_flag;

						sInit.bRecBufComm = pinfo->comm_recfrm_en;
						sInit.uiRecBufAddr[0] = pinfo->recfrm_addr[0];
						sInit.uiRecBufAddr[1] = pinfo->recfrm_addr[1];
						sInit.uiRecBufAddr[2] = pinfo->recfrm_addr[2];

						sInit.frm_crop_info.frm_crop_enable = pinfo->frm_crop_info.frm_crop_enable;
						sInit.frm_crop_info.display_w = (pinfo->rotate == 1 || pinfo->rotate == 2)? pinfo->frm_crop_info.display_h: pinfo->frm_crop_info.display_w;
						sInit.frm_crop_info.display_h = (pinfo->rotate == 1 || pinfo->rotate == 2)? pinfo->frm_crop_info.display_w: pinfo->frm_crop_info.display_h;

	    				ret = h265Enc_initEncoder(&sInit, &g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var);
	    			}
				}
				if (ret == H26XENC_SUCCESS)
					g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = TRUE;
				else {
					g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = FALSE;
					DBG_ERR("vdoenc set init channel fail\r\n");
				}
			}
			break;
		case VDOENC_SET_CBR:
			{
				KDRV_VDOENC_CBR *pinfo = (KDRV_VDOENC_CBR *)param;
				H26XEncRCParam sRcParam = {0};

				DBG_IND("[KDRV_VDOENC][%d] Set CBR Enable = %d, StaticTime = %u, ByteRate = %u, Fr = %u, GOP = %u, IQp = (%u, %u, %u), PQp = (%u, %u, %u), IPWei = %d, KP_Prd = %u, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, MAS = %d", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)pinfo->enable, (unsigned int)pinfo->static_time, (unsigned int)pinfo->byte_rate, (unsigned int)pinfo->frame_rate, (unsigned int)pinfo->gop, (unsigned int)pinfo->init_i_qp, (unsigned int)pinfo->min_i_qp, (unsigned int)pinfo->max_i_qp, (unsigned int)pinfo->init_p_qp, (unsigned int)pinfo->min_p_qp, (unsigned int)pinfo->max_p_qp, (unsigned int)pinfo->ip_weight, (unsigned int)pinfo->key_p_period, (unsigned int)pinfo->kp_weight, (unsigned int)pinfo->p2_weight, (unsigned int)pinfo->p3_weight, (unsigned int)pinfo->lt_weight, (int)pinfo->motion_aq_str);

				sRcParam.uiEncId = KDRV_DEV_ID_CHANNEL(id);
				sRcParam.uiRCMode = H26X_RC_CBR;
				sRcParam.uiInitIQp = pinfo->init_i_qp;
				sRcParam.uiMinIQp = pinfo->min_i_qp;
				sRcParam.uiMaxIQp = pinfo->max_i_qp;
				sRcParam.uiInitPQp = pinfo->init_p_qp;
				sRcParam.uiMinPQp = pinfo->min_p_qp;
				sRcParam.uiMaxPQp = pinfo->max_p_qp;
				sRcParam.uiBitRate = pinfo->byte_rate*8;
				if (pinfo->src_frame_rate > 0 && pinfo->encode_frame_rate > 0) {
					sRcParam.uiFrameRateBase = pinfo->frame_rate * pinfo->src_frame_rate / pinfo->encode_frame_rate;
					sRcParam.uiFrameRateIncr = 1000;
					sRcParam.uiGOP = pinfo->gop * pinfo->src_frame_rate / pinfo->encode_frame_rate;
				}
				else {
					sRcParam.uiFrameRateBase = pinfo->frame_rate;
					sRcParam.uiFrameRateIncr = 1000;
					sRcParam.uiGOP = pinfo->gop;
				}
				sRcParam.uiRowLevelRCEnable = 1;
				sRcParam.uiStaticTime = pinfo->static_time;
				sRcParam.iIPWeight = pinfo->ip_weight;
				sRcParam.uiKeyPPeriod = pinfo->key_p_period;
				sRcParam.iKPWeight = pinfo->kp_weight;
				sRcParam.iP2Weight = pinfo->p2_weight;
				sRcParam.iP3Weight = pinfo->p3_weight;
				sRcParam.iLTWeight = pinfo->lt_weight;
				sRcParam.iMotionAQStrength = pinfo->motion_aq_str;
				sRcParam.uiSvcBAMode = pinfo->svc_weight_mode;
				sRcParam.uiMaxFrameSize = pinfo->max_frame_size;
				sRcParam.uiBRTolerance = pinfo->br_tolerance;

				h26XEnc_setRcInit(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sRcParam);
			}
			break;
		case VDOENC_SET_EVBR:
			{
				KDRV_VDOENC_EVBR *pinfo = (KDRV_VDOENC_EVBR *)param;
				H26XEncRCParam sRcParam = {0};

				DBG_IND("[KDRV_VDOENC][%d] Set EVBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u, GOP = %d, KeyPPeriod = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, KPWei = %d, MotionAQ = %d, StillFr = %d, MotionR = %d, Psnr = (%d, %d, %d), P2_Wei = %d, P3_Wei = %d, LT_Wei = %d", (unsigned int)KDRV_DEV_ID_CHANNEL(id),(unsigned int) pinfo->enable, (unsigned int)pinfo->static_time, (unsigned int)pinfo->byte_rate,(unsigned int) pinfo->frame_rate, (unsigned int)pinfo->gop, (unsigned int)pinfo->key_p_period, (unsigned int)pinfo->init_i_qp, (unsigned int)pinfo->min_i_qp, (unsigned int)pinfo->max_i_qp, (unsigned int)pinfo->init_p_qp, (unsigned int)pinfo->min_p_qp, (unsigned int)pinfo->max_p_qp, (int)pinfo->ip_weight, (int)pinfo->kp_weight, (int)pinfo->motion_aq_st, (unsigned int)pinfo->still_frm_cnd, (unsigned int)pinfo->motion_ratio_thd, (unsigned int)pinfo->i_psnr_cnd, (unsigned int)pinfo->p_psnr_cnd, (unsigned int)pinfo->kp_psnr_cnd, (int)pinfo->p2_weight, (int)pinfo->p3_weight, (int)pinfo->lt_weight);

				sRcParam.uiEncId = KDRV_DEV_ID_CHANNEL(id);
				sRcParam.uiRCMode = H26X_RC_EVBR;
				sRcParam.uiInitIQp = pinfo->init_i_qp;
				sRcParam.uiMinIQp = pinfo->min_i_qp;
				sRcParam.uiMaxIQp = pinfo->max_i_qp;
				sRcParam.uiInitPQp = pinfo->init_p_qp;
				sRcParam.uiMinPQp = pinfo->min_p_qp;
				sRcParam.uiMaxPQp = pinfo->max_p_qp;
				sRcParam.uiBitRate = pinfo->byte_rate*8;
				sRcParam.uiFrameRateBase = pinfo->frame_rate;
				sRcParam.uiFrameRateIncr = 1000;
				sRcParam.uiGOP = pinfo->gop;
				sRcParam.uiRowLevelRCEnable = 1;
				sRcParam.uiStaticTime = pinfo->static_time;
				sRcParam.iIPWeight = pinfo->ip_weight;
				sRcParam.iKPWeight = pinfo->kp_weight;
				//sRcParam.uiChangePos = 100;	// remove to use Rate control globle : gChangePos , for support proc modification //
				sRcParam.uiKeyPPeriod = pinfo->key_p_period;
				sRcParam.iMotionAQStrength = pinfo->motion_aq_st;
				sRcParam.uiStillFrameCnd = pinfo->still_frm_cnd;
				sRcParam.uiMotionRatioThd = pinfo->motion_ratio_thd;
				sRcParam.uiIPsnrCnd = pinfo->i_psnr_cnd;
				sRcParam.uiPPsnrCnd = pinfo->p_psnr_cnd;
				sRcParam.uiKeyPPsnrCnd = pinfo->kp_psnr_cnd;
				sRcParam.uiMinStillIQp = pinfo->min_p_qp;
				sRcParam.iP2Weight = pinfo->p2_weight;
				sRcParam.iP3Weight = pinfo->p3_weight;
				sRcParam.iLTWeight = pinfo->lt_weight;
				sRcParam.uiSvcBAMode = pinfo->svc_weight_mode;
				sRcParam.uiMaxFrameSize = pinfo->max_frame_size;
				sRcParam.uiBRTolerance = pinfo->br_tolerance;

				h26XEnc_setRcInit(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sRcParam);
			}
			break;
		case VDOENC_SET_VBR:
			{
				KDRV_VDOENC_VBR *pinfo = (KDRV_VDOENC_VBR *)param;
				H26XEncRCParam sRcParam = {0};

				DBG_IND("[KDRV_VDOENC][%d] Set VBR Enable = %d, Policy = %d, StaticTime = %d, ByteRate = %d, Fr = %u, GOP = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, ChangePos = %d, KP_Prd = %d, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, MAS = %d", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)pinfo->enable, (unsigned int)pinfo->policy, (unsigned int)pinfo->static_time, (unsigned int)pinfo->byte_rate, (unsigned int)pinfo->frame_rate, (unsigned int)pinfo->gop, (unsigned int)pinfo->init_i_qp,(unsigned int) pinfo->min_i_qp, (unsigned int)pinfo->max_i_qp, (unsigned int)pinfo->init_p_qp, (unsigned int)pinfo->min_p_qp, (unsigned int)pinfo->max_p_qp, (int)pinfo->ip_weight,(unsigned int) pinfo->change_pos, (unsigned int)pinfo->key_p_period, (int)pinfo->kp_weight, (int)pinfo->p2_weight, (int)pinfo->p3_weight, (int)pinfo->lt_weight, (int)pinfo->motion_aq_str);

				sRcParam.uiEncId = KDRV_DEV_ID_CHANNEL(id);

				if (pinfo->policy == 1) {
					sRcParam.uiRCMode = H26X_RC_VBR2;
				} else {
					sRcParam.uiRCMode = H26X_RC_VBR;
				}
				sRcParam.uiInitIQp = pinfo->init_i_qp;
				sRcParam.uiMinIQp = pinfo->min_i_qp;
				sRcParam.uiMaxIQp = pinfo->max_i_qp;
				sRcParam.uiInitPQp = pinfo->init_p_qp;
				sRcParam.uiMinPQp = pinfo->min_p_qp;
				sRcParam.uiMaxPQp = pinfo->max_p_qp;
				sRcParam.uiBitRate = pinfo->byte_rate*8;
				sRcParam.uiFrameRateBase = pinfo->frame_rate;
				sRcParam.uiFrameRateIncr = 1000;
				sRcParam.uiGOP = pinfo->gop;
				sRcParam.uiRowLevelRCEnable = 1;
				sRcParam.uiStaticTime = pinfo->static_time;
				sRcParam.iIPWeight = pinfo->ip_weight;
				sRcParam.uiChangePos = pinfo->change_pos;
				sRcParam.uiKeyPPeriod = pinfo->key_p_period;
				sRcParam.iKPWeight = pinfo->kp_weight;
				sRcParam.iP2Weight = pinfo->p2_weight;
				sRcParam.iP3Weight = pinfo->p3_weight;
				sRcParam.iLTWeight = pinfo->lt_weight;
				sRcParam.iMotionAQStrength = pinfo->motion_aq_str;
				sRcParam.uiSvcBAMode = pinfo->svc_weight_mode;
				sRcParam.uiMaxFrameSize = pinfo->max_frame_size;
				sRcParam.uiBRTolerance = pinfo->br_tolerance;

				h26XEnc_setRcInit(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sRcParam);
			}
			break;
		case VDOENC_SET_FIXQP:
			{
				KDRV_VDOENC_FIXQP *pinfo = (KDRV_VDOENC_FIXQP *)param;
				H26XEncRCParam sRcParam = {0};

				DBG_IND("[KDRV_VDOENC][%d] Set FIXQP Enable = %d, Fr = %u, IFixQP = %d, PFixQP = %d", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)pinfo->enable, (unsigned int)pinfo->frame_rate, (unsigned int)pinfo->fix_i_qp, (unsigned int)pinfo->fix_p_qp);

				sRcParam.uiEncId = KDRV_DEV_ID_CHANNEL(id);
				sRcParam.uiRCMode = H26X_RC_FixQp;
				sRcParam.uiMinIQp = pinfo->fix_i_qp;
				sRcParam.uiFixIQp = pinfo->fix_i_qp;
				sRcParam.uiMaxIQp = pinfo->fix_i_qp;
				sRcParam.uiMinPQp = pinfo->fix_p_qp;
				sRcParam.uiFixPQp = pinfo->fix_p_qp;
				sRcParam.uiMaxPQp = pinfo->fix_p_qp;
				sRcParam.uiFrameRateBase = pinfo->frame_rate;
				sRcParam.uiFrameRateIncr = 1000;

				h26XEnc_setRcInit(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sRcParam);
			}
			break;
		case VDOENC_SET_ROWRC:
			{
				KDRV_VDOENC_ROW_RC *pinfo = (KDRV_VDOENC_ROW_RC *)param;
				H26XEncRowRcCfg sCfg = {0};

				sCfg.bEnable = pinfo->enable;
				sCfg.ucPredWt[0] = 0;
				sCfg.ucPredWt[1] = 4;
				sCfg.ucScale = 0;		//TODO
				sCfg.ucQPRange[0] = pinfo->i_qp_range;
				sCfg.ucQPRange[1] = pinfo->p_qp_range;
				sCfg.ucQPStep[0] = pinfo->i_qp_step;
				sCfg.ucQPStep[1] = pinfo->p_qp_step;
				sCfg.ucMinQP[0] = pinfo->min_i_qp;
				sCfg.ucMinQP[1] = pinfo->min_p_qp;
				sCfg.ucMaxQP[0] = pinfo->max_i_qp;
				sCfg.ucMaxQP[1] = pinfo->max_p_qp;
				sCfg.uiInitFrmCoeff = 0;	//TODO

				ret = h26XEnc_setRowRcCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set RowRC, en = %u, (range, step, min, max)=> I = (%u, %u, %u, %u), P = (%u, %u, %u, %u),  ret = %d\r\n", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)pinfo->enable, pinfo->i_qp_range, pinfo->i_qp_step, pinfo->min_i_qp, pinfo->max_i_qp, pinfo->p_qp_range, pinfo->p_qp_step, pinfo->min_p_qp, pinfo->max_p_qp, (int)ret);
			}
			break;
		case VDOENC_SET_QPMAP:
			{
				KDRV_VDOENC_USR_QP *pinfo = (KDRV_VDOENC_USR_QP *)param;
				H26XEncUsrQpCfg sCfg = {0};

				sCfg.bEnable = pinfo->enable;
				sCfg.uiQpMapAddr = (UINT32)pinfo->qp_map_addr;
				sCfg.uiQpMapSize = pinfo->qp_map_size;
				sCfg.uiQpMapLineOffset = pinfo->qp_map_loft;

				ret = H26XEnc_setUsrQpCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set QpMap = (%u, 0x%08x, %u, %u), ret = %d\r\n", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)pinfo->enable, (unsigned int)pinfo->qp_map_addr, (unsigned int)pinfo->qp_map_size, (unsigned int)pinfo->qp_map_loft, (int)ret);
			}
			break;
		case VDOENC_SET_3DNR:
			{
				H26XEncTnrCfg stCfg = {0};

				if (sizeof(KDRV_VDOENC_3DNR) != sizeof(H26XEncTnrCfg)) {
					DBG_ERR("KDRV_VDOENC_3DNR not equal to H26XEncTnrCfg\r\n");
					ret = H26XENC_FAIL;
				}
				else {
					memcpy((void *)&stCfg, param, sizeof(H26XEncTnrCfg));
					ret = h26XEnc_setTnrCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
				}
			}
			break;
		case VDOENC_SET_3DNRCB:
			{
				KDRV_VDOENC_3DNRCB *pCb = (KDRV_VDOENC_3DNRCB *)param;

				if (h26x_getChipId() == 98528) {
					g_enc_info[KDRV_DEV_ID_CHANNEL(id)].vdoenc_3dnr_cb = pCb->vdoenc_3dnr_cb;
				}
				else
					g_enc_info[KDRV_DEV_ID_CHANNEL(id)].vdoenc_3dnr_cb = NULL;
				//DBG_ERR("[KDRV_VDOENC][%d] vdoenc set not support id(%d)\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (int)(parm_id));
			}
			break;
		case VDOENC_SET_ROI:
			{
				unsigned int i;
				KDRV_VDOENC_ROI *pinfo = (KDRV_VDOENC_ROI *)param;

				for (i = 0; i < H26X_MAX_ROI_W; i++) {
					H26XEncRoiCfg sCfg = {0};

					sCfg.bEnable = pinfo->st_roi[i].enable;
					sCfg.usCoord_X = pinfo->st_roi[i].coord_X>>4;
					sCfg.usCoord_Y = pinfo->st_roi[i].coord_Y>>4;
					sCfg.usWidth = pinfo->st_roi[i].width>>4;
					sCfg.usHeight= pinfo->st_roi[i].height>>4;
					sCfg.cQP = pinfo->st_roi[i].qp;
					sCfg.ucMode = pinfo->st_roi[i].qp_mode;

					ret = h26XEnc_setRoiCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, i, &sCfg);
					DBG_IND("[KDRV_VDOENC][%d] set ROI[%d/%d] , en = %d, win = (%u, %u, %u, %u), qp = (%d, %u), ret = %d\r\n", (unsigned int)KDRV_DEV_ID_CHANNEL(id), (unsigned int)i, (H26X_MAX_ROI_W - 1), pinfo->st_roi[i].enable, pinfo->st_roi[i].coord_X, pinfo->st_roi[i].coord_Y, pinfo->st_roi[i].width, pinfo->st_roi[i].height, pinfo->st_roi[i].qp, pinfo->st_roi[i].qp_mode, (int)ret );
					if (ret != H26XENC_SUCCESS)
						break;
				}
			}
			break;
		case VDOENC_SET_SMART_ROI:
		case VDOENC_SET_MOT_ADDR:
			DBG_ERR("[KDRV_VDOENC][%d] vdoenc set not support id(%d) \r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (int)(parm_id));
			break;
		case VDOENC_SET_MD:
			{
				KDRV_VDOENC_MD_INFO *pinfo = (KDRV_VDOENC_MD_INFO *)param;
				H26XEncMdInfoCfg sCfg = {0};

				sCfg.uiMdWidth = pinfo->md_width;
				sCfg.uiMdHeight = pinfo->md_height;
				sCfg.uiMdLofs = pinfo->md_lofs;
				sCfg.uiMdBufAdr = pinfo->md_buf_adr;
				sCfg.uiRot = pinfo->rotation;
				sCfg.uiRoiXY = pinfo->roi_xy;
				sCfg.uiRoiWH = pinfo->roi_wh;

				ret = h26XEnc_setMdInfoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set MdInfo = (%u, %u, %u, 0x%x, 0x%x, 0x%x, 0x%x), ret = %d\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)),
					(unsigned int)(pinfo->md_width), (unsigned int)(pinfo->md_height), (unsigned int)(pinfo->md_lofs),
					(unsigned int)(pinfo->md_buf_adr), (unsigned int)pinfo->roi_xy, (unsigned int)pinfo->roi_wh,
					(unsigned int)(pinfo->rotation), (int)(ret));
			}
			break;
		case VDOENC_SET_SLICESPLIT:
			{
				KDRV_VDOENC_SLICE_SPLIT *pinfo = (KDRV_VDOENC_SLICE_SPLIT *)param;
				H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.pFuncCtx;
				//H26XEncSliceSplitCfg sCfg = {0};

				pFuncCtx->stSliceSplit.stCfg.bEnable = pinfo->enable;
				pFuncCtx->stSliceSplit.stCfg.uiSliceRowNum = pinfo->slice_row_num;
				//sCfg.bEnable = pinfo->enable;
				//sCfg.uiSliceRowNum = pinfo->slice_row_num;

				//ret = h26XEnc_setSliceSplitCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				//DBG_IND("[KDRV_VDOENC][%d] set SliceSplit = (%u, %u), ret = %d\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (unsigned int)(pinfo->enable), (unsigned int)(pinfo->slice_row_num), (int)(ret));
				DBG_IND("[KDRV_VDOENC][%d] set SliceSplit = (%u, %u)\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (unsigned int)(pinfo->enable), (unsigned int)(pinfo->slice_row_num));
			}
			break;
		case VDOENC_SET_GDR:
			{
				KDRV_VDOENC_GDR *pinfo = (KDRV_VDOENC_GDR *)param;
				H26XEncGdrCfg sCfg = {0};

				sCfg.bEnable = pinfo->enable;
				sCfg.usPeriod = pinfo->period;
				sCfg.usNumber = pinfo->number;
				sCfg.bGDRIFrm = pinfo->enable_gdr_i_frm;
				sCfg.bForceGDRQpEn = pinfo->enable_gdr_qp;
				sCfg.ucGDRQp = pinfo->gdr_qp;

				ret = h26XEnc_setGdrCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set GDR = (%u, %u, %u, %u), ret = %d\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (unsigned int)(pinfo->enable), (unsigned int)(pinfo->period), (unsigned int)(pinfo->number), (unsigned int)(pinfo->enable_gdr_i_frm), (int)(ret));
			}
			break;
		case VDOENC_SET_RESET_IFRAME:
			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_rst_i_frm = TRUE;
			DBG_IND("[KDRV_VDOENC][%d] set Reset I-Frame\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)));
			break;
		case VDOENC_SET_DUMPINFO:
			DBG_ERR("[KDRV_VDOENC][%d] vdoenc set not support id(%d)\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (int)(parm_id));
			break;
		case VDOENC_SET_RESET:
			if (h26x_getBusyStatus())
				h26x_reset();
			break;
		case VDOENC_SET_AQ:
			{
				KDRV_VDOENC_AQ *pinfo = (KDRV_VDOENC_AQ *)param;
				H26XEncAqCfg sCfg = {0};
				INT16 AqTbl[30] = {-120,-112,-104,-96,-88,-80,-72,-64,-56,-48,-40,-32,-24,-16,-8,7,15,23,31,39,47,55,63,71,79,87,95,103,111,119};

				sCfg.bEnable = pinfo->enable;
				sCfg.ucIC2 = 0;
				sCfg.ucIStr = pinfo->i_str;
				sCfg.ucPStr = pinfo->p_str;
				sCfg.cMaxDQp = pinfo->max_delta_qp;
				sCfg.cMinDQp = pinfo->min_delta_qp;
				sCfg.ucAslog2 = 36;
				sCfg.ucDepth = 2;		// HEVC only //
				sCfg.ucPlaneX = 2;
				sCfg.ucPlaneY = 2;
				memcpy(sCfg.sTh, AqTbl, sizeof(INT16)*30);

				#if 0
				if (h26x_getChipId() == 98528) {
					if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
						sCfg.bAqMode = pinfo->mode;
						sCfg.ucIStr1 = pinfo->i_str1;
						sCfg.ucPStr1 = pinfo->p_str1;
						sCfg.ucIStr2 = pinfo->i_str2;
						sCfg.ucPStr2 = pinfo->p_str2;
					}
				}
				#endif

				ret = h26XEnc_setAqCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set AQ = (%u, %u, %u, %d, %d), ret = %d\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (unsigned int)(pinfo->enable), (unsigned int)(pinfo->i_str), (unsigned int)(pinfo->p_str), (int)(pinfo->max_delta_qp), (int)(pinfo->min_delta_qp), (int)(ret));
			}
			break;
		case VDOENC_SET_COE:
			//coe_set_config((PK_COE_CONFIG) param);
			break;
		case VDOENC_SET_CLOSE:
			g_enc_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = FALSE;
			DBG_IND("[KDRV_VDOENC][%d] set Close\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)));
			#if H26X_MEM_USAGE
			{
				H26XMemUsage *p_mem_usage = (H26XMemUsage *)h26xEnc_getMemUsage(g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType, KDRV_DEV_ID_CHANNEL(id));
				p_mem_usage->cxt_size = 0;
			}
			#endif
			break;
		case VDOENC_SET_OSG_RGB:
			{
				KDRV_VDOENC_OSG_RGB *pinfo = (KDRV_VDOENC_OSG_RGB *)param;
				H26XEncOsgRgbCfg stRgbCfg = {0};

				UINT8 i, j;

				for (i = 0; i < 3; i++) {
					for (j = 0; j < 3; j++) {
						stRgbCfg.ucRgb2Yuv[i][j] = pinfo->rgb2yuv[i][j];
					}
				}

				ret = h26XEnc_setOsgRgbCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stRgbCfg);
			}
			break;
		case VDOENC_SET_OSG_PAL:
			{
				KDRV_VDOENC_OSG_PAL *pinfo = (KDRV_VDOENC_OSG_PAL *)param;
				H26XEncOsgPalCfg stPalCfg = {0};

				stPalCfg.ucAlpha = pinfo->alpha;
				stPalCfg.ucRed = pinfo->red;
				stPalCfg.ucGreen = pinfo->green;
				stPalCfg.ucBlue = pinfo->blue;

				ret = h26XEnc_setOsgPalCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, pinfo->idx, &stPalCfg);
			}
			break;
		case VDOENC_SET_OSG_WIN:
			{
				KDRV_VDOENC_OSG_WIN *pinfo = (KDRV_VDOENC_OSG_WIN *)param;
				H26XEncOsgWinCfg stWinCfg = {0};

				stWinCfg.bEnable = pinfo->enable;

				stWinCfg.stGrap.ucType = pinfo->st_grap.type;
				stWinCfg.stGrap.usWidth = pinfo->st_grap.width;
				stWinCfg.stGrap.usHeight = pinfo->st_grap.height;
				stWinCfg.stGrap.usLofs = pinfo->st_grap.line_offset;
				stWinCfg.stGrap.uiAddr = pinfo->st_grap.addr;

				stWinCfg.stDisp.ucMode = pinfo->st_disp.mode;
				stWinCfg.stDisp.usXStr = pinfo->st_disp.str_x;
				stWinCfg.stDisp.usYStr = pinfo->st_disp.str_y;
				stWinCfg.stDisp.ucBgAlpha = pinfo->st_disp.bg_alpha;
				stWinCfg.stDisp.ucFgAlpha = pinfo->st_disp.fg_alpha;
				stWinCfg.stDisp.ucMaskType = pinfo->st_disp.mask_type;
				stWinCfg.stDisp.ucMaskBdSize = pinfo->st_disp.mask_bd_size;
				stWinCfg.stDisp.ucMaskY[0] = pinfo->st_disp.mask_y[0];
				stWinCfg.stDisp.ucMaskY[1] = pinfo->st_disp.mask_y[1];
				stWinCfg.stDisp.ucMaskCb = pinfo->st_disp.mask_cb;
				stWinCfg.stDisp.ucMaskCr = pinfo->st_disp.mask_cr;

				stWinCfg.stGcac.bEnable = pinfo->st_gcac.enable;
				stWinCfg.stGcac.ucBlkWidth = pinfo->st_gcac.blk_width;
				stWinCfg.stGcac.ucBlkHeight = pinfo->st_gcac.blk_height;
				stWinCfg.stGcac.ucBlkHNum = pinfo->st_gcac.blk_num;
				stWinCfg.stGcac.ucOrgColorLv = pinfo->st_gcac.org_color_level;
				stWinCfg.stGcac.ucInvColorLv = pinfo->st_gcac.inv_color_level;
				stWinCfg.stGcac.ucNorDiffTh = pinfo->st_gcac.nor_diff_th;
				stWinCfg.stGcac.ucInvDiffTh = pinfo->st_gcac.inv_diff_th;
				stWinCfg.stGcac.ucStaOnlyMode = pinfo->st_gcac.sta_only_mode;
				stWinCfg.stGcac.ucFillEvalMode = pinfo->st_gcac.full_eval_mode;
				stWinCfg.stGcac.ucEvalLumTarg = pinfo->st_gcac.eval_lum_targ;

				stWinCfg.stQpmap.ucLpmMode = pinfo->st_qp_map.lpm_mode;
				stWinCfg.stQpmap.ucTnrMode = pinfo->st_qp_map.tnr_mode;
				stWinCfg.stQpmap.ucFroMode = pinfo->st_qp_map.fro_mode;
				stWinCfg.stQpmap.ucQpMode = pinfo->st_qp_map.qp_mode;
				stWinCfg.stQpmap.cQpVal = pinfo->st_qp_map.qp;

				stWinCfg.stKey.bEnable = pinfo->st_key.enable;
				stWinCfg.stKey.bAlphaEn = pinfo->st_key.alpha_en;
				stWinCfg.stKey.ucAlpha = pinfo->st_key.alpha;
				stWinCfg.stKey.ucRed = pinfo->st_key.red;
				stWinCfg.stKey.ucGreen = pinfo->st_key.green;
				stWinCfg.stKey.ucBlue = pinfo->st_key.blue;

				ret = h26XEnc_setOsgWinCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, ((pinfo->layer_idx<<4) + pinfo->win_idx), &stWinCfg);
			}
			break;
                case VDOENC_SET_ISPCB:
			{
				KDRV_VDOENC_ISPCB *pinfo = (KDRV_VDOENC_ISPCB *)param;
				H26XEncIspCbCfg stCfg = {0};

                                stCfg.id = pinfo->id;
                                stCfg.vdoenc_isp_cb = pinfo->vdoenc_isp_cb;

				ret = h26XEnc_setIspCbCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
			}
                        break;
                case VDOENC_SET_RDO:
			{
				KDRV_VDOENC_RDO *pinfo = (KDRV_VDOENC_RDO *)param;

				if (pinfo->rdo_codec == VDOENC_RDO_CODEC_264) {
					H264EncRdoCfg stCfg = {0};

					stCfg.bEnable = 1;
					stCfg.ucI_Y4_CB_P = pinfo->rdo_param.rdo_264.avc_intra_4x4_cost_bias;
					stCfg.ucI_Y8_CB_P = pinfo->rdo_param.rdo_264.avc_intra_8x8_cost_bias;
					stCfg.ucI_Y16_CB_P = pinfo->rdo_param.rdo_264.avc_intra_16x16_cost_bias;
					stCfg.ucP_Y4_CB = pinfo->rdo_param.rdo_264.avc_inter_tu4_cost_bias;
					stCfg.ucP_Y8_CB = pinfo->rdo_param.rdo_264.avc_inter_tu8_cost_bias;
					stCfg.ucIP_CB_SKIP = pinfo->rdo_param.rdo_264.avc_inter_skip_cost_bias;
					stCfg.ucP_Y_COEFF_COST_TH = pinfo->rdo_param.rdo_264.avc_inter_luma_coeff_cost_th;
					stCfg.ucP_C_COEFF_COST_TH = pinfo->rdo_param.rdo_264.avc_inter_chroma_coeff_cost_th;

					ret = h264Enc_setRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
					DBG_IND("[KDRV_VDOENC][%d] set H264 rdo, intra[4x4(%u) 8x8(%u) 16x16(%u)], inter[tu4(%u) tu8(%u) skip(%u)]\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), stCfg.ucI_Y4_CB_P, stCfg.ucI_Y8_CB_P, stCfg.ucI_Y16_CB_P, stCfg.ucP_Y4_CB, stCfg.ucP_Y8_CB, stCfg.ucIP_CB_SKIP);
				} else if (pinfo->rdo_codec == VDOENC_RDO_CODEC_265) {
					H265EncRdoCfg stCfg = {0};

					stCfg.global_motion_penalty_I32 = pinfo->rdo_param.rdo_265.hevc_intra_32x32_cost_bias;
					stCfg.global_motion_penalty_I16 = pinfo->rdo_param.rdo_265.hevc_intra_16x16_cost_bias;
					stCfg.global_motion_penalty_I08 = pinfo->rdo_param.rdo_265.hevc_intra_8x8_cost_bias;
					stCfg.cost_bias_skip = pinfo->rdo_param.rdo_265.hevc_inter_skip_cost_bias;
					stCfg.cost_bias_merge = pinfo->rdo_param.rdo_265.hevc_inter_merge_cost_bias;
					stCfg.ime_scale_64c = pinfo->rdo_param.rdo_265.hevc_inter_64x64_cost_bias;
					stCfg.ime_scale_64p = pinfo->rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias;
					stCfg.ime_scale_32c = pinfo->rdo_param.rdo_265.hevc_inter_32x32_cost_bias;
					stCfg.ime_scale_32p = pinfo->rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias;
					stCfg.ime_scale_16c = pinfo->rdo_param.rdo_265.hevc_inter_16x16_cost_bias;

					ret = h265Enc_setRdoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
					DBG_IND("[KDRV_VDOENC][%d] set H265 rdo, intra[32x32(%d) 16x16(%d) 8x8(%d)], inter[skip(%d) merge(%d) 64x64(%u) 64x32_32x64(%u) 32x32(%u) 32x16_16x32(%u) 16x16(%u)]\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), stCfg.global_motion_penalty_I32, stCfg.global_motion_penalty_I16, stCfg.global_motion_penalty_I08, stCfg.cost_bias_skip, stCfg.cost_bias_merge, stCfg.ime_scale_64c, stCfg.ime_scale_64p, stCfg.ime_scale_32c, stCfg.ime_scale_32p, stCfg.ime_scale_16c);
				} else {
	                                DBG_ERR("[KDRV_VDOENC][%d] rdo codec not support\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)));
				}
			}
                        break;
		case VDOENC_SET_JND:
			{
				KDRV_VDOENC_JND *pinfo = (KDRV_VDOENC_JND *)param;
				H26XEncJndCfg sCfg = {0};

				sCfg.bEnable = pinfo->enable;
				sCfg.ucStr = pinfo->str;
				sCfg.ucLevel = pinfo->level;
				sCfg.ucTh = pinfo->threshold;

				#if 0
				if (h26x_getChipId() == 98528) {
					sCfg.ucCStr = pinfo->ucCStr;
					sCfg.ucR5Flag = pinfo->ucR5Flag;
					sCfg.ucLsigmaTh = pinfo->ucLsigmaTh;
					sCfg.ucLsigma = pinfo->ucLsigma;
				}
				#endif

				ret = h26XEnc_setJndCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &sCfg);
				DBG_IND("[KDRV_VDOENC][%d] set JND = (%u, %u, %u, %u), ret = %d\r\n", (int)(KDRV_DEV_ID_CHANNEL(id)), (unsigned int)(pinfo->enable), (unsigned int)(pinfo->str), (unsigned int)(pinfo->level), (int)(pinfo->threshold), (int)(ret));
			}
			break;
		case VDOENC_SET_LL_MEM:
			{
#ifdef VDOCDC_LL
				KDRV_VDOENC_LL_MEM *pinfo = (KDRV_VDOENC_LL_MEM *)param;

				kdrv_vdocdc_set_llc_mem(pinfo->addr, pinfo->size);
#endif
			}
			break;
		case VDOENC_SET_GOPNUM:
			{
				if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)
					h264Enc_setGopNum(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, *(UINT32 *)param);
				else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265)
					h265Enc_setGopNum(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, *(UINT32 *)param);
			}
			break;

		case VDOENC_SET_OSDMASKCB:
			{
#ifdef VDOCDC_LL
				KDRV_VDOENC_OSDMASKCB *pOsdMaskCB = (KDRV_VDOENC_OSDMASKCB *)param;
				kdrv_vdocdc_set_osdmask_cb(pOsdMaskCB);
#endif
			}
			break;
		case VDOENC_SET_LSC:
			h265Enc_setLongStartCode(*(UINT32 *)param);
			break;
		case VDOENC_SET_MAQDIFF:
			{
				KDRV_VDOENC_MAQDIFF *pInfo = (KDRV_VDOENC_MAQDIFF *)param;
				H26XEncMAQDiffInfoCfg stCfg = {0};

				stCfg.enable = pInfo->enable;
				stCfg.str = pInfo->str;
				stCfg.start_idx = pInfo->start_idx;
				stCfg.end_idx = pInfo->end_idx;

				h26XEnc_setMAQDiffInfoCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
			}
			break;
		case VDOENC_SET_SEI_BS_CHKSUM:
			{
				if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264)
					h264Enc_setSEIBsChksumen(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, *(UINT32 *)param);
				else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265)
					h265Enc_setSEIBsChksumen(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, *(UINT32 *)param);
			}
			break;
		case VDOENC_SET_LPM:
			{
				KDRV_VDOENC_LPM *pInfo = (KDRV_VDOENC_LPM *)param;
				H26XEncLpmCfg stCfg = {0};

				stCfg.bEnable = pInfo->enable;

				if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H264) {
					stCfg.ucRmdSadEn = 3;
					stCfg.ucIMEStopEn = 2;
					stCfg.ucIMEStopTh = 66;
					stCfg.ucRdoStopEn = 2;
					stCfg.ucRdoStopTh = 80;
					stCfg.ucChrmDmEn = 0;
				} else if (g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var.eCodecType == VCODEC_H265) {
					stCfg.ucRmdSadEn = 3;
					stCfg.ucIMEStopEn = 2;
					stCfg.ucIMEStopTh = 60;
					stCfg.ucRdoStopEn = 2;
					stCfg.ucRdoStopTh = 100;
					stCfg.ucChrmDmEn = 3;
				} else {
					DBG_ERR("err codec type\r\n");
					ret = H26XENC_FAIL;
				}

				h26XEnc_setLpmCfg(&g_enc_info[KDRV_DEV_ID_CHANNEL(id)].enc_var, &stCfg);
			}
			break;
		default:
			{
				DBG_ERR("vdoenc set parm_id(%d) not support\r\n", (int)(parm_id));
				ret = H26XENC_FAIL;
			}
			break;
	};

	if (ret != H26XENC_SUCCESS) {
		DBG_ERR("vdoenc set parm_id(%d) failed\r\n", (int)(parm_id));
	}

	return (ret != H26XENC_SUCCESS);
}
