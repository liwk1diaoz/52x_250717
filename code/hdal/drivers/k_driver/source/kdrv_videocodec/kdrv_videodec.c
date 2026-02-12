/**
 * @file kdrv_videoenc.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2018
 */
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/util.h"
#include "kwrap/verinfo.h"

#include "kdrv_type.h"

#include "kdrv_videodec/kdrv_videodec.h"
#include "kdrv_videodec/kdrv_videodec_lmt.h"

#include "kdrv_vdocdc_dbg.h"
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
#include "h26x_def.h"
#include "h264dec_api.h"
#include "h265dec_api.h"
#include "comm/timer.h"

#include "../include/jpeg.h"

typedef struct _vdodec_info_{
	H26XDEC_VAR dec_var;
	BOOL   b_enable;
	UINT64 setup_time;
	UINT64 start_time;
	UINT64 end_time;
} vdodec_info;

vdodec_info g_dec_info[KDRV_VDOENC_ID_MAX];

// timer
static TIMER_ID								gH26XDecTimerID = 0;
static UINT32								gH26XDecTimeout = 0;

// wait for H26X decoding ready by interrupt
static void _kdrv_videodec_timerhdl(UINT32 uiEvent)
{
	// timeout, call H26X API to stop waiting for INT
	h26x_exit_isr();
	DBG_ERR("[H264DEC] decode timeout\r\n");
	gH26XDecTimeout = 1;
}

static void _kdrv_videodec_timeropen(UINT32 uiTime)
{
	if (timer_open((TIMER_ID *)&gH26XDecTimerID, _kdrv_videodec_timerhdl) != E_OK) {
		DBG_ERR("[H264DEC] timer open failed\r\n");
		return;
	}

	timer_cfg(gH26XDecTimerID, uiTime * 1000/*ms*/, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
}

static void _kdrv_videodec_timerclose(void)
{
	timer_close(gH26XDecTimerID);
}

INT32 kdrv_videodec_open(UINT32 chip, UINT32 engine)
{
	if (engine == KDRV_VIDEOCDC_ENGINE0) {
		memset(&g_dec_info, 0, sizeof(vdodec_info)*KDRV_VDODEC_ID_MAX);
#if defined(__FREERTOS)
		nvt_vdocdc_drv_init();
#endif
	}
    else if (engine == KDRV_VIDEOCDC_ENGINE_JPEG) {
	    jpeg_open();
	}
	else {
		DBG_ERR("kdrv_videodec_open engine id(%d) failed\r\n", (int)(engine));
	}

	return 0;
}

INT32 kdrv_videodec_close(UINT32 chip, UINT32 engine)
{
	if (engine == KDRV_VIDEOCDC_ENGINE0) {
#ifdef VDOCDC_LL
		while(kdrv_vdocdc_get_dec_job_done() == FALSE) {
			vos_util_delay_ms(10);
		}
#endif
		memset(&g_dec_info, 0, sizeof(vdodec_info)*KDRV_VDODEC_ID_MAX);

#if defined (__FREERTOS)
		nvt_vdocdc_drv_remove();
#endif
	}
    else if (engine == KDRV_VIDEOCDC_ENGINE_JPEG) {
	    jpeg_close();
	}
	else {
		DBG_ERR("kdrv_videodec_close engine id(%d) failed\r\n", (int)(engine));
	}

	return 0;
}

INT32 kdrv_videodec_trigger(UINT32 id, KDRV_VDODEC_PARAM *p_dec_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	UINT32 ret = 0;
	if (p_dec_param == NULL) {
		DBG_ERR("p_dec_param is NULL\r\n");
		return -1;
	}

	if (KDRV_DEV_ID_ENGINE(id) == KDRV_VIDEOCDC_ENGINE_JPEG) {

		if (jpeg_add_queue(KDRV_DEV_ID_CHANNEL(id) ,JPEG_CODEC_MODE_DEC, (void *)p_dec_param, 0/*p_cb_func*/, 0/*p_user_data*/) != E_OK) {
			if (p_cb_func != NULL) {
				p_dec_param->errorcode = -1;
				p_cb_func->callback(p_dec_param, p_user_data);
			}
			return -1;
		} else {
			if (p_cb_func != NULL) {
				p_dec_param->errorcode = 0;
				p_cb_func->callback(p_dec_param, p_user_data);
			}
    		return 0;
		}
	}
	else if (KDRV_DEV_ID_ENGINE(id) == KDRV_VIDEOCDC_ENGINE0) {
#ifdef VDOCDC_LL
#else
		UINT32 interrupt = 0;
#endif
		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdoenc trigger channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}

		if (p_dec_param->cur_bs_size != 0)
			h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);
		else
			h26x_setIntEn(H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_TIME_OUT_INT | H26X_FRAME_TIME_OUT_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT | H26X_SWRST_FINISH_INT);

		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H264) {
			H264DEC_INFO sInfo;
			KDRV_VDODEC_REFFRMCB pCb;
			pCb = p_dec_param->vRefFrmCb;
			if (!pCb.VdoDec_RefFrmDo) {
				DBG_ERR("[H264DEC][%d] SET_RefFrmCB no RefFrmDo func!\r\n", (unsigned int)id);
				return -1;
			}

			sInfo.uiSrcYAddr = p_dec_param->y_addr;
			sInfo.uiSrcUVAddr = p_dec_param->c_addr;
			sInfo.uiBSAddr = p_dec_param->bs_addr;
			sInfo.uiBSSize = p_dec_param->bs_size;
			sInfo.uiCurBsSize = p_dec_param->cur_bs_size;

			ret = h264Dec_prepareOnePicture(&sInfo, &g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var);
			if (ret != H264DEC_SUCCESS) {
				if (p_cb_func != NULL) {
					p_dec_param->errorcode = -1;
					p_cb_func->callback(p_dec_param, p_user_data);
				}
				return -1;
			}
#ifdef VDOCDC_LL
			kdrv_vdocdc_add_job(VDOCDC_DEC_MODE, id, g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.uiAPBAddr, p_cb_func, p_user_data, (void *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var, (void *)p_dec_param);
#else
			{
				h26x_lock();
				h26x_enableClk();
				h26x_setEncDirectRegSet(g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.uiAPBAddr);
				h26x_start();
				interrupt = h26x_waitINT();
			}

			if (p_cb_func != NULL) {
				if (interrupt != 0x00000001) {
					DBG_ERR("decode error interrupt:(0x%08x)\r\n", (unsigned int)interrupt);
					//h26x_prtReg();
					p_dec_param->errorcode = -1;
				} else {
					p_dec_param->errorcode = 0;
				}
				p_cb_func->callback(p_dec_param, p_user_data);
			}

			if ((h264Dec_getResYAddr((H26XDEC_VAR *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var) != 0) && interrupt == 0x00000001)
				pCb.VdoDec_RefFrmDo(id, h264Dec_getResYAddr((H26XDEC_VAR *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var), FALSE);
#endif
		}
        else if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H265) {
			H265DEC_INFO sInfo;
			KDRV_VDODEC_REFFRMCB pCb;
			pCb = p_dec_param->vRefFrmCb;
			if (!pCb.VdoDec_RefFrmDo) {
				DBG_ERR("[H265DEC][%d] SET_RefFrmCB no RefFrmDo func!\r\n", (unsigned int)id);
				return -1;
			}

			sInfo.uiSrcYAddr = p_dec_param->y_addr;
			sInfo.uiSrcUVAddr = p_dec_param->c_addr;
			sInfo.uiBSAddr = p_dec_param->bs_addr;
			sInfo.uiBSSize = p_dec_param->bs_size;
			sInfo.uiCurBsSize = p_dec_param->cur_bs_size;

			ret = h265Dec_prepareOnePicture(&sInfo, &g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var);
			if (ret != H264DEC_SUCCESS) {
				if (p_cb_func != NULL) {
					p_dec_param->errorcode = -1;
					p_cb_func->callback(p_dec_param, p_user_data);
				}
				return -1;
			}
#ifdef VDOCDC_LL
			kdrv_vdocdc_add_job(VDOCDC_DEC_MODE, id, g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.uiAPBAddr, p_cb_func, p_user_data, (void *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var, (void *)p_dec_param);
#else
			{
				h26x_lock();
				h26x_enableClk();
				h26x_setEncDirectRegSet(g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.uiAPBAddr);
				h26x_start();
				interrupt = h26x_waitINT();
			}

			if (p_cb_func != NULL) {
				if (interrupt != 0x00000001) {
					DBG_ERR("decode error interrupt:(0x%08x)\r\n", (unsigned int)interrupt);
					//h26x_prtReg();
					p_dec_param->errorcode = -1;
				} else {
					p_dec_param->errorcode = 0;
				}
				p_cb_func->callback(p_dec_param, p_user_data);
			}

			if ((h265Dec_getResYAddr((H26XDEC_VAR *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var) != 0) && interrupt == 0x00000001)
				pCb.VdoDec_RefFrmDo(id, h265Dec_getResYAddr((H26XDEC_VAR *)&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var), FALSE);
#endif
		}

#ifdef VDOCDC_LL
#else
		p_dec_param->interrupt = interrupt;

		if (interrupt != 0x00000001) {
			DBG_ERR("decode error interrupt:(0x%08x)\r\n", (unsigned int)interrupt);
			h26x_resetHW();

			h26x_disableClk();
			h26x_unlock();
			//h26x_prtReg();
			return -1;
		}

		h26x_disableClk();
		h26x_unlock();
#endif
	}
	else
	{
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)(KDRV_DEV_ID_ENGINE(id)));
		return -1;
	}

	return 0;
}

INT32 kdrv_videodec_get(UINT32 id, KDRV_VDODEC_GET_PARAM_ID parm_id, VOID *param)
{
	if (parm_id != VDODEC_GET_JPEG_FREQ && parm_id != VDODEC_GET_MEM_SIZE && parm_id != VDODEC_GET_LL_MEM_SIZE && parm_id != VDODEC_GET_VUI_INFO && parm_id != VDODEC_GET_JPEG_RECYUV_WH) {
		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdodec get channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}
	}

	if (parm_id == VDODEC_GET_JPEG_FREQ) {
		UINT32 *p_freq = param;
		DBG_IND("VDOENC_GET_JPEG_FREQ\r\n");
		*p_freq = jpeg_get_config(JPEG_CONFIG_ID_FREQ);
	}
	else if (parm_id == VDODEC_GET_MEM_SIZE) {
		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H264) {
				H264DecMemInfo *pinfo = (H264DecMemInfo *)param;

				h264Dec_queryMemSize(pinfo);
		} else if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H265) {
				H265DecMemInfo *pinfo = (H265DecMemInfo *)param;
				h265Dec_queryMemSize(pinfo);
		}
	}
	else if (parm_id == VDODEC_GET_VUI_INFO) {
		KDRV_VDODEC_VUI_INFO *pinfo = (KDRV_VDODEC_VUI_INFO *)param;
		H26XDEC_VUI *p_vui = &g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.stVUI;

		pinfo->present_flag = p_vui->bPresentFlag;
		pinfo->sar_width = p_vui->uiSarWidth;
		pinfo->sar_height = p_vui->uiSarHeight;
		pinfo->matrix_coef = p_vui->ucMatrixCoeff;
		pinfo->transfer_characteristics = p_vui->ucTransChar;
		pinfo->colour_primaries = p_vui->ucColorPrimaries;
		pinfo->video_format = p_vui->ucVideoFmt;
		pinfo->color_range = p_vui->ucColorRange;
		pinfo->timing_present_flag = p_vui->bTimingPresentFlag;
	}
	else if (parm_id == VDODEC_GET_JPEG_RECYUV_WH) {
		((KDRV_VDODEC_RECYUV_WH *)param)->align_w = SIZE_16X(((KDRV_VDODEC_RECYUV_WH *)param)->align_w);
		((KDRV_VDODEC_RECYUV_WH *)param)->align_h = SIZE_16X(((KDRV_VDODEC_RECYUV_WH *)param)->align_h);
	}
	else if (parm_id == VDODEC_GET_H26X_INTERRUPT) {
		UINT32 *pInterrupt = (UINT32 *)param;
		UINT32  uiTimeoutMs;

		uiTimeoutMs = 100;  //max time out: 100 ms
		_kdrv_videodec_timeropen(uiTimeoutMs);
		*pInterrupt = h26x_waitINT();
		_kdrv_videodec_timerclose();

		if (gH26XDecTimeout) {
			DBG_WRN("[H26XDEC][%d] h26x_resetHW\r\n", (unsigned int)id);
			h26x_resetHW();
			gH26XDecTimeout = 0;
			*pInterrupt = H26X_TIME_OUT_INT;
		}

		DBG_IND("call h26x_waitINT() Interrupt 0x%x\r\n", (unsigned int)*pInterrupt);
	}
	else if (parm_id == VDODEC_GET_LL_MEM_SIZE) {
		KDRV_VDODEC_LL_MEM_INFO *pinfo = (KDRV_VDODEC_LL_MEM_INFO *)param;
#ifdef VDOCDC_LL
		pinfo->size = kdrv_vdocdc_get_llc_mem();
#else
		pinfo->size = 0;
#endif
	}
	else if (parm_id == VDODEC_GET_RECYUV_WH) {
		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H264) {
			((KDRV_VDODEC_RECYUV_WH *)param)->align_w = SIZE_64X(((KDRV_VDODEC_RECYUV_WH *)param)->align_w);
			((KDRV_VDODEC_RECYUV_WH *)param)->align_h = SIZE_16X(((KDRV_VDODEC_RECYUV_WH *)param)->align_h);
		} else if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H265) {
			((KDRV_VDODEC_RECYUV_WH *)param)->align_w = SIZE_64X(((KDRV_VDODEC_RECYUV_WH *)param)->align_w);
			((KDRV_VDODEC_RECYUV_WH *)param)->align_h = SIZE_64X(((KDRV_VDODEC_RECYUV_WH *)param)->align_h);
		}
	}
	else {
		DBG_ERR("kdrv_videodec_get parm_id(%d) failed\r\n", (int)(parm_id));
	}

	return 0;
}

INT32 kdrv_videodec_set(UINT32 id, KDRV_VDODEC_SET_PARAM_ID parm_id, VOID *param)
{
	UINT32 ret = H26XENC_SUCCESS;

	if (parm_id != VDODEC_SET_JPEG_FREQ && parm_id != VDODEC_SET_CODEC && parm_id != VDODEC_SET_INIT && parm_id != VDODEC_SET_LL_MEM) {
		if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable != TRUE) {
			DBG_ERR("vdodec set channl(%d) not enable already!\r\n", (int)(id));
			return -1;
		}
	}

	switch (parm_id) {
		case VDODEC_SET_JPEG_FREQ:
			{
				UINT32 freq;
				//ER ret;
				DBG_ERR("kdrv_vdodec_set - VDOENC_SET_JPEG_FREQ\r\n");

				freq = *((UINT32*)param);
	            jpeg_set_config(JPEG_CONFIG_ID_FREQ, freq);
			}
			break;
		case VDODEC_SET_CODEC:
			if (*(KDRV_VDODEC_TYPE *)param == VDODEC_TYPE_JPEG)
				;
			else if (*(KDRV_VDODEC_TYPE *)param == VDODEC_TYPE_H264)
				g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType = VCODEC_H264;
			else if (*(KDRV_VDODEC_TYPE *)param == VDODEC_TYPE_H265)
				g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType = VCODEC_H265;
			else
				DBG_ERR("kdrv_vidoedec_set codec(%d) failed\r\n", (int)(*(KDRV_VDOENC_TYPE *)param));
			break;
		case VDODEC_SET_INIT:
			{
				KDRV_VDODEC_INIT *pinfo = (KDRV_VDODEC_INIT *)param;

				if (h26x_efuse_check(pinfo->width, pinfo->height) != TRUE) {
					ret = H26XENC_FAIL;
				}
				else {
					if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H264) {
						H264DEC_INIT sInit;

						sInit.uiWidth        = pinfo->width;
						sInit.uiHeight       = pinfo->height;
						sInit.uiYLineOffset  = sInit.uiWidth;
						sInit.uiUVLineOffset = sInit.uiWidth;
						sInit.uiDecBufAddr   = pinfo->buf_addr;
						sInit.uiDecBufSize   = pinfo->buf_size;
						sInit.uiH264DescAddr = pinfo->desc_addr;
						sInit.uiH264DescSize = pinfo->desc_size;
						ret = h264Dec_initDecoder(&sInit, &g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var);
						pinfo->width = sInit.uiWidth;
						pinfo->height = sInit.uiHeight;
						pinfo->uiYLineoffset = sInit.uiYLineOffset << 2;
						pinfo->uiUVLineoffset = sInit.uiUVLineOffset << 2;
					}
		            else if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H265) {
						H265DEC_INIT sInit;

						sInit.uiWidth        = pinfo->width;
						sInit.uiHeight       = pinfo->height;
						sInit.uiYLineOffset  = sInit.uiWidth;
						sInit.uiUVLineOffset = sInit.uiWidth;
						sInit.uiDecBufAddr   = pinfo->buf_addr;
						sInit.uiDecBufSize   = pinfo->buf_size;
						sInit.uiH265DescAddr = pinfo->desc_addr;
						sInit.uiH265DescSize = pinfo->desc_size;

						ret = h265Dec_initDecoder(&sInit, &g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var);
						pinfo->width = sInit.uiWidth;
						pinfo->height = sInit.uiHeight;
						pinfo->uiYLineoffset = sInit.uiYLineOffset << 2;
						pinfo->uiUVLineoffset = sInit.uiUVLineOffset << 2;
					}
				}

				if (ret == H26XENC_SUCCESS) {
					g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = TRUE;
				}
				else {
					g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = FALSE;
					DBG_ERR("[%d] vdodec set init channel fail\r\n", (int)id);
				}
			}
			break;
		case VDODEC_SET_CLOSE:
			//dal_dec_close(KDRV_DEV_ID_CHANNEL(id));
			g_dec_info[KDRV_DEV_ID_CHANNEL(id)].b_enable = FALSE;
			break;
		case VDODEC_SET_NXT_BS:
			if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H264) {
				KDRV_VDODEC_NXT_BS_INFO *pinfo = (KDRV_VDODEC_NXT_BS_INFO *)param;
				DBG_IND("VDODEC_SET_NXT_BS H264 bs_addr 0x%x, bs_size 0x%x, total_size 0x%x\r\n", (unsigned int)pinfo->bs_addr, (unsigned int)pinfo->bs_size, (unsigned int)pinfo->total_bs_size);
				h264Dec_setNextBsBuf(&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var, pinfo->bs_addr, pinfo->bs_size, pinfo->total_bs_size);
			}
			else if (g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType == VCODEC_H265) {
				KDRV_VDODEC_NXT_BS_INFO *pinfo = (KDRV_VDODEC_NXT_BS_INFO *)param;
				DBG_IND("VDODEC_SET_NXT_BS H265 bs_addr 0x%x, bs_size 0x%x, total_size 0x%x\r\n", (unsigned int)pinfo->bs_addr, (unsigned int)pinfo->bs_size, (unsigned int)pinfo->total_bs_size);
				h265Dec_setNextBsBuf(&g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var, pinfo->bs_addr, pinfo->bs_size, pinfo->total_bs_size);
			}
			else {
				DBG_ERR("[%d] vdodec codec type(%d) not support set parm_id(%d)\r\n", (unsigned int)id, g_dec_info[KDRV_DEV_ID_CHANNEL(id)].dec_var.eCodecType, parm_id);
			}
			break;
		case VDODEC_SET_LL_MEM:
			{
#ifdef VDOCDC_LL
				KDRV_VDOENC_LL_MEM *pinfo = (KDRV_VDOENC_LL_MEM *)param;

				kdrv_vdocdc_set_llc_mem(pinfo->addr, pinfo->size);
#endif
			}
			break;
		default:
			break;
	};

	return 0;
}

