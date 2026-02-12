#define __MODULE__          H26XENC
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/type.h"

#include "nvt_vdocdc_dbg.h"

#include "h26x_def.h"
//#include "h26x_common.h"
#include "h26x_bitstream.h"

#include "h26xenc_api.h"
#include "h26xenc_wrap.h"
#include "h26xenc_int.h"
#include "h26xenc_rc.h"
#include "h265enc_int.h"
#if H26X_MEM_USAGE
#include "kdrv_videoenc/kdrv_videoenc.h"
#endif
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"

#if JND_DEFAULT_ENABLE
    H26XEncJndCfg gEncJNDCfg = {0,  3,  0,  20};
#endif
#if H26X_USE_DIFF_MAQ
    int gDiffMAQStr = 0;//-6;
    int gDiffMAQNum = H26X_MOTION_BITMAP_NUM;
    int gDiffMAQMorphology = 0; // 0: none, 1: dilation
#endif
int gRRCNDQPStep = 1;
int gRRCNDQPRange = 1;
#if H26X_AUTO_RND
	int gAutoRndCondition = 10;
	int gAutoRndLevel = 2;
#endif
#if H26X_DIS_OSG_CACHE_OPERATION
	int gOsgCacheFlush = 0;
#endif

extern rc_cb g_rc_cb;

#if H26X_MEM_USAGE
H26XMemUsage g_mem_usage[2][KDRV_VDOENC_ID_MAX]; //[codec][stream]
#endif

#if H26X_SYNC_ROWRC_MAXMIN_QP
extern unsigned int g_RRCSyncRCQPCond;
#endif


//#define iCeil(a,b)              ((((a)+(b)-1)/(b))*(b))
//#define BINARY_LINE_OFFSET(a)   (iCeil(a, BIT_LINE_OFFSET)/8)

struct stream_t
{
    unsigned char *buf;
    unsigned int bit_pos;
    unsigned char *cur_byte;
    unsigned int length;
};

static void stream_enc_init(struct stream_t *stream, unsigned char *buf, int length)
{
    memset(buf, 0, sizeof(unsigned char)*length);
    stream->buf = buf;
    stream->length = length;
    stream->cur_byte = stream->buf;
    stream->bit_pos = 0;
}

static void stream_put_bit(struct stream_t *stream, unsigned char bit)
{
    *stream->cur_byte |= ((bit&0x01)<<stream->bit_pos);
    stream->bit_pos++;
    if (8 == stream->bit_pos) {
        stream->cur_byte++;
        stream->bit_pos = 0;
    }
}

static void stream_set_byte_pos(struct stream_t *stream, unsigned int offset)
{
    stream->cur_byte = stream->buf + offset;
    stream->bit_pos = 0;
}

#define BASE_RATIO_BIT	12
static int BinaryScaleRotate(unsigned char *dst_buf, unsigned char *src_buf, unsigned int src_w, unsigned int src_h, UINT32 src_lft,
								unsigned int roi_x, unsigned int roi_y, unsigned int roi_w, unsigned int roi_h,
                                unsigned int dst_w, unsigned int dst_h, UINT32 dst_lft, UINT32 rot_type)
{
    struct stream_t stream;
    unsigned char *src_ptr;
    unsigned char bit_value;
    unsigned int w, h;
    unsigned int pos_x, pos_y;
    //unsigned int src_lft, dst_lft;
    unsigned int h_r, w_r;
	unsigned int base_x, roi_en = 0;

	DBG_IND("src: w:%u,h:%u,oft:%u(roi: w:%u,h:%u,x:%u,y:%u). dst:w:%u,h:%u,oft:%u,rot:%u\r\n",
		(int)src_w,(int)src_h,(int)src_lft,roi_w,roi_h,roi_x,roi_y,(int)dst_w,(int)dst_h,(int)dst_lft,(int)rot_type);

	if (roi_w > 0 && roi_h > 0) {
		src_w = roi_w;
		src_h = roi_h;
		roi_en = 1;
	}

    if (1 == rot_type) {	// CCW90
        stream_enc_init(&stream, dst_buf, dst_lft*dst_w);
		h_r = (src_w<<BASE_RATIO_BIT)/dst_w;
		w_r = (src_h<<BASE_RATIO_BIT)/dst_h;
        for (w = 0; w < dst_w; w++) {
            stream_set_byte_pos(&stream, w*dst_lft);
            pos_x = (((dst_w-1-w)*h_r)>>BASE_RATIO_BIT)+roi_x;
			base_x = 0;
            for (h = 0; h < dst_h; h++) {
                //pos_y = (h*w_r)>>BASE_RATIO_BIT;
                pos_y = (base_x>>BASE_RATIO_BIT)+roi_y;
                src_ptr = src_buf + pos_y*src_lft;
                bit_value = (src_ptr[pos_x>>3]>>(pos_x&0x07))&0x01;
                stream_put_bit(&stream, bit_value);
				base_x += w_r;
            }
        }
        h26x_cache_clean((UINT32)dst_buf, dst_lft*dst_w);
    }
    else if (2 == rot_type) {	// CCW270
        stream_enc_init(&stream, dst_buf, dst_lft*dst_w);
		h_r = (src_w<<BASE_RATIO_BIT)/dst_w;
		w_r = (src_h<<BASE_RATIO_BIT)/dst_h;
        for (w = 0; w < dst_w; w++) {
            stream_set_byte_pos(&stream, w*dst_lft);
            pos_x = ((w*h_r)>>BASE_RATIO_BIT)+roi_x;
			base_x = (dst_h-1)*w_r;
            for (h = 0; h < dst_h; h++) {
                //pos_y = (((dst_h-1)-h)*w_r)>>BASE_RATIO_BIT;
                pos_y = (base_x>>BASE_RATIO_BIT)+roi_y;
                src_ptr = src_buf + pos_y*src_lft;
                bit_value = (src_ptr[pos_x>>3]>>(pos_x&0x07))&0x01;
                stream_put_bit(&stream, bit_value);
				base_x -= w_r;
            }
        }
        h26x_cache_clean((UINT32)dst_buf, dst_lft*dst_w);
    }
    else if (3 == rot_type) {
        stream_enc_init(&stream, dst_buf, dst_lft*dst_h);
		h_r = (src_h<<BASE_RATIO_BIT)/dst_h;
		w_r = (src_w<<BASE_RATIO_BIT)/dst_w;
        for (h = 0; h < dst_h; h++) {
            stream_set_byte_pos(&stream, h*dst_lft);
            pos_y = (((dst_h-1-h)*h_r)>>BASE_RATIO_BIT)+roi_y;
            src_ptr = src_buf + pos_y*src_lft;
			base_x = (dst_w-1)*w_r;
            for (w = 0; w < dst_w; w++) {
                //pos_x = (((dst_w-1)-w)*w_r)>>BASE_RATIO_BIT;
                pos_x = (base_x>>BASE_RATIO_BIT)+roi_x;
                bit_value = (src_ptr[pos_x>>3]>>(pos_x&0x07))&0x01;
                stream_put_bit(&stream, bit_value);
				base_x -= w_r;
            }
        }
        h26x_cache_clean((UINT32)dst_buf, dst_lft*dst_h);
    }
    else {  // rotation none
        if (src_w == dst_w && src_h == dst_h && 0 == roi_en) {
            memcpy(dst_buf, src_buf, dst_lft*dst_h*sizeof(char));
        } else {
	        stream_enc_init(&stream, dst_buf, dst_lft*dst_h);
			h_r = (src_h<<BASE_RATIO_BIT)/dst_h;
			w_r = (src_w<<BASE_RATIO_BIT)/dst_w;
	        for (h = 0; h < dst_h; h++) {
	            stream_set_byte_pos(&stream, h*dst_lft);
	            //pos_y = h*src_h/dst_h;
	            pos_y = ((h*h_r)>>BASE_RATIO_BIT)+roi_y;
	            src_ptr = src_buf + pos_y*src_lft;
				base_x = 0;
	            for (w = 0; w < dst_w; w++) {
	                //pos_x = w*src_w/dst_w;
	                pos_x = (base_x>>BASE_RATIO_BIT)+roi_x;
	                bit_value = (src_ptr[pos_x>>3]>>(pos_x&0x07))&0x01;
	                stream_put_bit(&stream, bit_value);
					base_x += w_r;
	            }
	        }
		}
        h26x_cache_clean((UINT32)dst_buf, dst_lft*dst_h);
    }
    return 0;
}

#if H26X_USE_DIFF_MAQ
int h26xEnc_checkDiffMAQEnable(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	//if (0 != gDiffMAQStr && gDiffMAQNum > 1)
	if (pFuncCtx->stMAq.stMAQDiffInfoCfg.enable == TRUE)
		return 1;
	return 0;
}
#if H26X_MAQ_MORPHOLOGY
static int BinaryDilation(unsigned int out_buf, unsigned int src_buf, int width, int height, UINT32 line_oft)
{
    unsigned int *top_ptr, *cur_ptr, *bot_ptr;
    unsigned int *out_ptr;
    int w, h;
    int b_left, b_right, b_top, b_bottom;

    for (h = 0; h < height; h++) {
        top_ptr = (unsigned int *)(src_buf + (h-1)*line_oft);
        cur_ptr = (unsigned int *)(src_buf + h*line_oft);
        bot_ptr = (unsigned int *)(src_buf + (h+1)*line_oft);
        out_ptr = (unsigned int *)(out_buf + h*line_oft);
        if (0 == h)
            b_top = 0;
        else
            b_top = 1;
        if (height-1 == h)
            b_bottom = 0;
        else
            b_bottom = 1;

        for (w = 0; w < line_oft/4; w++) {
            if (0 == w)
                b_left = 0;
            else
                b_left = 1;
            if (line_oft/4-1 == w)
                b_right = 0;
            else
                b_right = 1;
            *out_ptr = cur_ptr[w];
            if (b_top) {
                if (b_left)
                    *out_ptr |= ((top_ptr[w-1]>>31)&0x01) | ((top_ptr[w]<<1)&0xFFFFFFFE);
                else
                    *out_ptr |= ((top_ptr[w]<<1)&0xFFFFFFFE);
                *out_ptr |= top_ptr[w];
                if (b_right)
                    *out_ptr |= ((top_ptr[w+1]&0x01)<<31) | ((top_ptr[w]>>1)&0x7FFFFFFF);
                else
                    *out_ptr |= ((top_ptr[w]>>1)&0x7FFFFFFF);
            }

            if (b_left)
                *out_ptr |= ((cur_ptr[w-1]>>31)&0x01) | ((cur_ptr[w]<<1)&0xFFFFFFFE);
            else
                *out_ptr |= ((cur_ptr[w]<<1)&0xFFFFFFFE);
            if (b_right)
                *out_ptr |= ((cur_ptr[w+1]&0x01)<<31) | ((cur_ptr[w]>>1)&0x7FFFFFFF);
            else
                *out_ptr |= ((cur_ptr[w]>>1)&0x7FFFFFFF);

            if (b_bottom) {
                if (b_left)
                    *out_ptr |= ((bot_ptr[w-1]>>31)&0x01) | ((bot_ptr[w]<<1)&0xFFFFFFFE);
                else
                    *out_ptr |= ((bot_ptr[w]<<1)&0xFFFFFFFE);
                *out_ptr |= bot_ptr[w];
                if (b_right)
                    *out_ptr |= ((bot_ptr[w+1]&0x01)<<31) | ((bot_ptr[w]>>1)&0x7FFFFFFF);
                else
                    *out_ptr |= ((bot_ptr[w]>>1)&0x7FFFFFFF);
            }
            out_ptr++;
        }
    }
    return 0;
}
#endif

static int BinaryBitmapDifference(UINT32 dst_bitmap, UINT32 src_bitmap[H26X_MOTION_BITMAP_NUM+1], UINT32 md_w, UINT32 md_h, UINT32 line_oft, UINT8 cur_idx, UINT8 mor_type, UINT8 idx1, UINT8 idx2)
{
    int x, y;
    int i, idx;
    UINT32 *out_ptr = (UINT32 *)dst_bitmap;
    UINT32 *src_ptr[H26X_MOTION_BITMAP_NUM];
    #if H26X_MAQ_MORPHOLOGY
    UINT32 *tmp_ptr, *cur_ptr;
    #endif
	int base_idx, specific_idx, num;
	if (idx1 > idx2) {
		base_idx = idx2;
		specific_idx = idx1;
	} else {
		base_idx = idx1;
		specific_idx = idx2;
	}
	num = specific_idx - base_idx;

    idx = cur_idx;
    for (i = 0; i < H26X_MOTION_BITMAP_NUM; i++) {
        src_ptr[i] = (UINT32 *)src_bitmap[idx];
        if (0 == idx)
            idx = H26X_MOTION_BITMAP_NUM-1;
        else
            idx--;
        //if (idx < 1)
        //    idx = H26X_MOTION_BITMAP_NUM;
    }

#if H26X_MAQ_MORPHOLOGY
    if (1 == mor_type)
        tmp_ptr = (UINT32 *)src_bitmap[H26X_MOTION_BITMAP_NUM];
    else
        tmp_ptr = (UINT32 *)dst_bitmap;
    cur_ptr = src_ptr[0];

	#if H26X_MAQ_DIFF_SPECIFIC_FRM
	for (y = 0; y < md_h; y++) {
		for (x = 0; x < line_oft; x+=4) {
				*tmp_ptr = 0;
			for (i = 1; i < num+1; i++) {
				*tmp_ptr |= (*src_ptr[base_idx + i]);
			}
			tmp_ptr++;
			for (i = 0; i < num+1; i++) {
				src_ptr[base_idx + i]++;
			}
		}
	}
	#else
    for (y = 0; y < md_h; y++) {
        for (x = 0; x < line_oft; x+=4) {
            *tmp_ptr = 0;
            for (i = 1; i < num; i++) {
                *tmp_ptr |= (*src_ptr[i]);
            }
            tmp_ptr++;
            for (i = 0; i < num; i++) {
                src_ptr[i]++;
            }
        }
    }
	#endif
    if (1 == mor_type) {
        BinaryDilation(dst_bitmap, src_bitmap[H26X_MOTION_BITMAP_NUM], md_w, md_h, line_oft);
    }
    out_ptr = (UINT32 *)dst_bitmap;
    for (y = 0; y < md_h; y++) {
        for (x = 0; x < line_oft; x+=4) {
            *out_ptr = ((*out_ptr)^(*cur_ptr)) & (*out_ptr);
            out_ptr++;
            cur_ptr++;
        }
    }
#else
    #if H26X_MAQ_DIFF_SPECIFIC_FRM
	for (y = 0; y < md_h; y++) {
		for (x = 0; x < line_oft; x+=4) {
			*out_ptr = 0;
			for (i = 1; i < num+1; i++) {
				*out_ptr |= (*src_ptr[base_idx + i]);
			}
			*out_ptr = ((*out_ptr)^(*src_ptr[base_idx])) & (*out_ptr);
			out_ptr++;
			for (i = 0; i < num+1; i++) {
				src_ptr[base_idx + i]++;
			}
        }
    }
#else
    for (y = 0; y < md_h; y++) {
        for (x = 0; x < line_oft; x+=4) {
            *out_ptr = 0;
            for (i = 1; i < num; i++) {
                *out_ptr |= (*src_ptr[i]);
            }
            *out_ptr = ((*out_ptr)^(*src_ptr[0])) & (*out_ptr);
            out_ptr++;
            for (i = 0; i < num; i++) {
                src_ptr[i]++;
            }
        }
    }
#endif
#endif
    h26x_cache_clean((UINT32)dst_bitmap, line_oft*md_h);
    return 0;
}
#endif

/*
//remove to flow (IVOT_N00025-854)
static INT32 check_osd_win(H26XEncOsg *p_osd_config) {
	UINT32 i = 0;
	UINT32 j = 0;
	UINT32 k = 0;
	UINT32 pointx1, pointx2, pointy1, pointy2;
	UINT32 mb_st_x[2], mb_end_x[2], mb_st_y[2], mb_end_y[2];

	//if (p_osd_config->check == 1)
	{
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 16; j++) {
				if (p_osd_config->stWin[j+i*16].bEnable == 1) {
					pointx1 = p_osd_config->stWin[j+i*16].stDisp.usXStr;
					pointx2 = pointx1 + p_osd_config->stWin[j+i*16].stGrap.usWidth - 1;
					pointy1 = p_osd_config->stWin[j+i*16].stDisp.usYStr;
					pointy2 = pointy1 + p_osd_config->stWin[j+i*16].stGrap.usHeight - 1;

					mb_st_x[0] = pointx1 / 16;
					mb_end_x[0] = pointx2 / 16;
					mb_st_y[0] = pointy1 / 16;
					mb_end_y[0] = pointy2 / 16;

					for (k = j+1; k < 16; k++) {
						if (p_osd_config->stWin[k+i*16].bEnable == 1) {
							pointx1 = p_osd_config->stWin[k+i*16].stDisp.usXStr;
							pointx2 = pointx1 + p_osd_config->stWin[k+i*16].stGrap.usWidth - 1;
							pointy1 = p_osd_config->stWin[k+i*16].stDisp.usYStr;
							pointy2 = pointy1 + p_osd_config->stWin[k+i*16].stGrap.usHeight - 1;

							mb_st_x[1] = pointx1 / 16;
							mb_end_x[1] = pointx2 / 16;
							mb_st_y[1] = pointy1 / 16;
							mb_end_y[1] = pointy2 / 16;

							if (((mb_st_x[1] >= mb_st_x[0]) && (mb_st_x[1] <= mb_end_x[0])) || ((mb_end_x[1] >= mb_st_x[0]) && (mb_end_x[1] <= mb_end_x[0]))) {
								if (((mb_st_y[1] >= mb_st_y[0]) && (mb_st_y[1] <= mb_end_y[0])) || ((mb_end_y[1] >= mb_st_y[0]) && (mb_end_y[1] <= mb_end_y[0]))) {
									DBG_ERR("OSD MB violation! layer%d, oldregion=%d, newregion=%d\r\n", (int)i, (int)j, (int)k);
									DBG_ERR("(%d, %d, %d, %d), (%d, %d, %d, %d)\r\n", (int)mb_st_x[0],(int)mb_end_x[0],(int)mb_st_y[0],(int)mb_end_y[0],
															(int)mb_st_x[1],(int)mb_end_x[1],(int)mb_st_y[1],(int)mb_end_y[1]);
									return 1;
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
*/
#if H26X_AUTO_RND
INT32 H26XEnc_setAutoRnd(H26XENC_VAR *pVar, UINT8 ucSliceQP)
{
	H26XEncRndCfg rnd_param;
	if (gAutoRndCondition > 0 && ucSliceQP < gAutoRndCondition) {
		VOS_TICK t0;
		rnd_param.bEnable = 1;
		vos_perf_mark(&t0);
		rnd_param.uiSeed = t0;
		rnd_param.ucRange = gAutoRndLevel;
		h26XEnc_setRndCfg(pVar, &rnd_param);
	}
	else {
		rnd_param.bEnable = 0;
		rnd_param.ucRange = 1;
		rnd_param.uiSeed = 0;
		h26XEnc_setRndCfg(pVar, &rnd_param);
	}
	return 0;
}
#endif

INT32 H26XEnc_setUsrQpCfg(H26XENC_VAR *pVar, H26XEncUsrQpCfg *pUsrQp)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	if ((pUsrQp->bEnable == TRUE) && (pUsrQp->uiQpMapAddr == 0)) {
		DBG_ERR("QpMapAddr cannot set to 0 while enable QP map\r\n");
		return H26XENC_FAIL;
	}

	memcpy(&pFuncCtx->stUsrQp.stCfg, pUsrQp, sizeof(H26XEncUsrQpCfg));

	if (pFuncCtx->stUsrQp.stCfg.bEnable == TRUE) {
		if (pUsrQp->uiQpMapAddr != pFuncCtx->stUsrQp.uiQpMapAddr) {
			if (pFuncCtx->stUsrQp.uiUsrQpMapSize >= pUsrQp->uiQpMapSize) {
				memcpy((UINT32 *)pFuncCtx->stUsrQp.uiQpMapAddr, (UINT32 *)pUsrQp->uiQpMapAddr, pUsrQp->uiQpMapSize);
			}
			else {
				DBG_ERR("QpMapAddr not the same and user_qp_map size(%d) bigger than internal buffer(%d)\r\n", (int)pFuncCtx->stUsrQp.uiUsrQpMapSize, (int)pUsrQp->uiQpMapSize);
				return H26XENC_FAIL;
			}
		}
		h26x_cache_clean(pFuncCtx->stUsrQp.uiQpMapAddr, pFuncCtx->stUsrQp.uiUsrQpMapSize);
	}

	h26xEnc_wrapUsrQpCfg(pRegSet, &pFuncCtx->stUsrQp, pVar->eCodecType);

	return H26XENC_SUCCESS;
}

INT32 H26XEnc_setPSNRCfg(H26XENC_VAR *pVar, BOOL bEnable)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	pFuncCtx->bPSNREn = bEnable;

	h26xEnc_wrapPSNRCfg(pRegSet, pFuncCtx->bPSNREn);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setSliceSplitCfg(H26XENC_VAR *pVar, H26XEncSliceSplitCfg *pSliceSplit)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	memcpy(&pFuncCtx->stSliceSplit.stExe, pSliceSplit, sizeof(H26XEncSliceSplitCfg));

	if (pSliceSplit->bEnable) {
		//fix cim
		if (pFuncCtx->stSliceSplit.stExe.uiSliceRowNum == 0) {
			DBG_ERR("slice split:%d, wrong slice row number = %d\r\n", (int)pSliceSplit->bEnable, (int)pFuncCtx->stSliceSplit.stExe.uiSliceRowNum);

			return H26XENC_FAIL;
		} else {
			if (pVar->eCodecType == VCODEC_H264) {
				pFuncCtx->stSliceSplit.uiNaluNum = CEILING_DIV(CEILING_DIV(pComnCtx->uiHeight, 16), pFuncCtx->stSliceSplit.stExe.uiSliceRowNum);
			}
			else {
				H265ENC_CTX *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
				pFuncCtx->stSliceSplit.uiNaluNum = CEILING_DIV(CEILING_DIV(pComnCtx->uiHeight, 64), pFuncCtx->stSliceSplit.stExe.uiSliceRowNum);

				if (pVdoCtx->stSeqCfg.bTileEn)
					pFuncCtx->stSliceSplit.uiNaluNum = pFuncCtx->stSliceSplit.uiNaluNum * pVdoCtx->stSeqCfg.ucTileNum;
			}
		}
	}
	else {
		pFuncCtx->stSliceSplit.uiNaluNum = 1;
		pFuncCtx->stSliceSplit.stExe.uiSliceRowNum = 0;

		if (pVar->eCodecType == VCODEC_H265) {
			H265ENC_CTX *pVdoCtx = (H265ENC_CTX *)pVar->pVdoCtx;
			H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;

			if (pVdoCtx->stSeqCfg.bTileEn) {
				pFuncCtx->stSliceSplit.uiNaluNum = pFuncCtx->stSliceSplit.uiNaluNum * pVdoCtx->stSeqCfg.ucTileNum;
				pFuncCtx->stSliceSplit.stExe.uiSliceRowNum = pSeqCfg->usLcuHeight;
			}
		}
	}

	h26xEnc_wrapSliceSplitCfg(pRegSet, &pFuncCtx->stSliceSplit);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setVarCfg(H26XENC_VAR *pVar, H26XEncVarCfg *pVarCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	memcpy(&pFuncCtx->stVar, pVarCfg, sizeof(H26XEncVarCfg));

	h26xEnc_wrapVarCfg(pRegSet, &pFuncCtx->stVar);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	// check parameter //

	memcpy(&pFuncCtx->stGdr.stCfg, pGdrCfg, sizeof(H26XEncGdrCfg));

	if (pFuncCtx->stGdr.stCfg.bGDRIFrm == 1) {
		if (pVar->eCodecType == VCODEC_H265) {
			pFuncCtx->stGdr.usStart = H265E_GDRI_MIN_SLICE_HEIGHT/16;
		} else if (pVar->eCodecType == VCODEC_H264) {
			pFuncCtx->stGdr.usStart = H264E_GDRI_MIN_SLICE_HEIGHT/16;
		} else {
			DBG_ERR("error codec type for gdr\r\n");
		}
	} else {
		pFuncCtx->stGdr.usStart = 0;
	}

	pFuncCtx->stGdr.uiStartPicCnt = pComnCtx->uiPicCnt;

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRoiCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncRoiCfg *pRoiCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	if (pRoiCfg->bEnable == TRUE) {
		pFuncCtx->stRoi.ucNumber += (pFuncCtx->stRoi.stCfg[ucIdx].bEnable == FALSE);
	}
	else {
		pFuncCtx->stRoi.ucNumber -= (pFuncCtx->stRoi.stCfg[ucIdx].bEnable == TRUE);
	}

	memcpy(&pFuncCtx->stRoi.stCfg[ucIdx], pRoiCfg, sizeof(H26XEncRoiCfg));

	h26xEnc_wrapRoiCfg(pRegSet, &pFuncCtx->stRoi, ucIdx);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRowRcCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	//H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stRowRc.stCfg, pRowRcCfg, sizeof(H26XEncRowRcCfg));

	//h26xEnc_wrapRowRcCfg(pRegSet, &pFuncCtx->stRowRc);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stAq.stCfg, pAqCfg, sizeof(H26XEncAqCfg));

	h26xEnc_wrapAqCfg(pRegSet, &pFuncCtx->stAq);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setLpmCfg(H26XENC_VAR *pVar, H26XEncLpmCfg *pLpmCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stLpm.stCfg, pLpmCfg, sizeof(H26XEncLpmCfg));

	h26xEnc_wrapLpmCfg(pRegSet, &pFuncCtx->stLpm);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRndCfg(H26XENC_VAR *pVar, H26XEncRndCfg *pRndCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stRnd.stCfg, pRndCfg, sizeof(H26XEncRndCfg));

	h26xEnc_wrapRndCfg(pRegSet, &pFuncCtx->stRnd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setScdCfg(H26XENC_VAR *pVar, H26XEncScdCfg *pScdCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stScd.stCfg, pScdCfg, sizeof(H26XEncScdCfg));

	h26xEnc_wrapScdCfg(pRegSet, &pFuncCtx->stScd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgRgbCfg(H26XENC_VAR *pVar, H26XEncOsgRgbCfg *pOsgRgbCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stOsg.stRgb, pOsgRgbCfg, sizeof(H26XEncOsgRgbCfg));

	h26xEnc_wrapOsgRgbCfg(pRegSet, &pFuncCtx->stOsg.stRgb);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgPalCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPalCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stOsg.stPal[ucIdx], pOsgPalCfg, sizeof(H26XEncOsgPalCfg));

	h26xEnc_wrapOsgPalCfg(pRegSet, ucIdx, &pFuncCtx->stOsg.stPal[ucIdx]);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setOsgWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWinCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	if (pOsgWinCfg->bEnable == TRUE) {
		if (pOsgWinCfg->stDisp.ucMode == 0) {
			if (pOsgWinCfg->stGrap.uiAddr == 0) {
				DBG_ERR("OSG graphic address cannot set to 0 while enable OSG graphic window\r\n");
				return H26XENC_FAIL;
			}
			else {
			#if H26X_DIS_OSG_CACHE_OPERATION
				if (gOsgCacheFlush)
			#endif
				{
					h26x_cache_clean(pOsgWinCfg->stGrap.uiAddr, pOsgWinCfg->stGrap.usLofs * pOsgWinCfg->stGrap.usHeight);
					DBG_IND("OSG cache flush 0x%x\n", (unsigned int)pOsgWinCfg->stGrap.uiAddr);
				}
			}
		}
		else {
			if (pOsgWinCfg->stDisp.ucMode > 1) {
				DBG_ERR("OSG display mode not support %d \r\n", pOsgWinCfg->stDisp.ucMode);
				return H26XENC_FAIL;
			}
		}
	}

	memcpy(&pFuncCtx->stOsg.stWin[ucIdx], pOsgWinCfg, sizeof(H26XEncOsgWinCfg));

	//remove to flow (IVOT_N00025-854)
	//if (check_osd_win(&pFuncCtx->stOsg) == 1) {
	//	return H26XENC_FAIL;
	//}

	h26xEnc_wrapOsgWinCfg(pRegSet, ucIdx, &pFuncCtx->stOsg.stWin[ucIdx]);

	return H26XENC_SUCCESS;
}
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

#if 0
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

INT32 h26XEnc_setMdInfoCfg(H26XENC_VAR *pVar, H26XEncMdInfoCfg *pMdInfoCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;
	UINT32 dst_w, dst_h;
	UINT32 roi_x, roi_y, roi_w, roi_h;

	if (pMdInfoCfg->uiRot & 0xF00) {
		dst_w = (pMdInfoCfg->uiRot == 0 || pMdInfoCfg->uiRot == 3)? SIZE_16X(pComnCtx->uiWidth)/16: SIZE_16X(pComnCtx->uiHeight)/16;
		dst_h = (pMdInfoCfg->uiRot == 0 || pMdInfoCfg->uiRot == 3)? SIZE_16X(pComnCtx->uiHeight)/16: SIZE_16X(pComnCtx->uiWidth)/16;
		if (0 == pMdInfoCfg->uiRoiWH) {
			roi_x = 0;
			roi_y = 0;
			roi_w = 0;
			roi_h = 0;
		}
		else if (pMdInfoCfg->uiRot & 0x100) {
			roi_x = (pMdInfoCfg->uiRoiXY&0xFFFF)>>4;
			roi_y = ((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4;
			roi_w = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
			roi_h = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
		}
		else {
			if (1 == (pMdInfoCfg->uiRot&0xFF)) {
				roi_w = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
				roi_h = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
				roi_x = pMdInfoCfg->uiMdWidth - 1 - (((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4) - roi_w;
				roi_y = (pMdInfoCfg->uiRoiXY&0xFFFF)>>4;
				DBG_IND("roi x = %d (%d - %d - %d), roi y = %d\n", (int)roi_x, (int)(pMdInfoCfg->uiMdWidth - 1), (int)(((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4), (int)roi_w, (int)roi_y);
			}
			else if (2 == (pMdInfoCfg->uiRot&0xFF)) {
				roi_w = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
				roi_h = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
				roi_x = ((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4;
				roi_y = pMdInfoCfg->uiMdHeight - 1 - ((pMdInfoCfg->uiRoiXY&0xFFFF)>>4) - roi_h;
				DBG_IND("roi x = %d, roi y = %d (%d - %d - %d)\n", (int)roi_x, (int)roi_y, (int)(pMdInfoCfg->uiMdHeight - 1), (int)((pMdInfoCfg->uiRoiXY&0xFFFF)>>4), (int)roi_h);
			}
			else if (3 == (pMdInfoCfg->uiRot&0xFF)) {
				roi_w = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
				roi_h = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
				roi_x = pMdInfoCfg->uiMdWidth - 1 - ((pMdInfoCfg->uiRoiXY&0xFFFF)>>4) - roi_w;
				roi_y = pMdInfoCfg->uiMdHeight - 1 - (((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4) - roi_h;
			}
			else {
				roi_x = (pMdInfoCfg->uiRoiXY&0xFFFF)>>4;
				roi_y = ((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4;
				roi_w = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
				roi_h = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
			}
		}
	}
	else {
		dst_w = SIZE_16X(pComnCtx->uiWidth)/16;
		dst_h = SIZE_16X(pComnCtx->uiHeight)/16;
		roi_x = (pMdInfoCfg->uiRoiXY&0xFFFF)>>4;
		roi_y = ((pMdInfoCfg->uiRoiXY>>16)&0xFFFF)>>4;
		roi_w = (pMdInfoCfg->uiRoiWH&0xFFFF)>>4;
		roi_h = ((pMdInfoCfg->uiRoiWH>>16)&0xFFFF)>>4;
	}

	// motion bitmap process, scale and rotation
	BinaryScaleRotate((UINT8*)pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0], (UINT8*)pMdInfoCfg->uiMdBufAdr,
				pMdInfoCfg->uiMdWidth, pMdInfoCfg->uiMdHeight, pMdInfoCfg->uiMdLofs,
				roi_x, roi_y, roi_w, roi_h,
				dst_w, dst_h, pFuncCtx->stMAq.stAddrCfg.uiMotLineOffset/8, (pMdInfoCfg->uiRot&0xFF));
#if H26X_USE_DIFF_MAQ
    // Get difference binary
    //if (0 != gDiffMAQStr && gDiffMAQNum > 1) {
	if (h26xEnc_checkDiffMAQEnable(pVar)) {
        UINT32 md_w, md_h, idx=0;
        md_w = (pMdInfoCfg->uiRot == 0 || pMdInfoCfg->uiRot == 3)? SIZE_16X(pComnCtx->uiWidth)/16: SIZE_16X(pComnCtx->uiHeight)/16;
        md_h = (pMdInfoCfg->uiRot == 0 || pMdInfoCfg->uiRot == 3)? SIZE_16X(pComnCtx->uiHeight)/16: SIZE_16X(pComnCtx->uiWidth)/16;
		idx = pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx;
		memcpy((void *)pFuncCtx->stMAq.stMAQDiffInfo.uiHistMotAddr[idx], (void *)pFuncCtx->stMAq.stAddrCfg.uiMotAddr[0], pFuncCtx->stMAq.stAddrCfg.uiMotLineOffset/8*md_h);

        if (pFuncCtx->stMAq.stMAQDiffInfo.uiMotFrmCnt >= pFuncCtx->stMAq.stMAQDiffInfoCfg.end_idx) {
            BinaryBitmapDifference(pFuncCtx->stMAq.stAddrCfg.uiMotAddr[1], pFuncCtx->stMAq.stMAQDiffInfo.uiHistMotAddr, md_w, md_h,
                pFuncCtx->stMAq.stAddrCfg.uiMotLineOffset/8, pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx, (UINT8)gDiffMAQMorphology, pFuncCtx->stMAq.stMAQDiffInfoCfg.start_idx, pFuncCtx->stMAq.stMAQDiffInfoCfg.end_idx);
        }
        pFuncCtx->stMAq.stMAQDiffInfo.uiMotFrmCnt++;
        pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx++;
        if (pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx >= H26X_MOTION_BITMAP_NUM)
            pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx = 0;
    }
#endif

	h26xEnc_wrapMotAddrCfg(pRegSet, &pFuncCtx->stMAq.stAddrCfg);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setJndCfg(H26XENC_VAR *pVar, H26XEncJndCfg *pJndCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet   *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stJnd.stCfg, pJndCfg, sizeof(H26XEncJndCfg));

	h26xEnc_wrapJndCfg(pRegSet, &pFuncCtx->stJnd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRcInit(H26XENC_VAR *pVar, H26XEncRCParam *pRcParam)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	pRcParam->uiPicSize = SIZE_16X(pComnCtx->uiWidth) * SIZE_16X(pComnCtx->uiHeight);
	pRcParam->uiLTRInterval = pComnCtx->uiLTRInterval;
	pRcParam->uiSVCLayer = pComnCtx->ucSVCLayer;

	if (g_rc_cb.h26xEnc_RcInit == NULL) {
		pComnCtx->ucRcMode = 0;
		return H26XENC_FAIL;
	}

	if (g_rc_cb.h26xEnc_RcInit(&pComnCtx->stRc, pRcParam) != 0) {
		pComnCtx->ucRcMode = 0;
		return H26XENC_FAIL;
	}

	pComnCtx->ucRcMode = pRcParam->uiRCMode;

	//if (pComnCtx->ucRcMode == H26X_RC_EVBR) {
	if (1) {
	//if (pRcParam->iMotionAQStrength) {
		H26XEncMotAqCfg sMAqCfg = {0};

		sMAqCfg.ucMode = 1;
		sMAqCfg.uc8x8to16x16Th = 2;
		sMAqCfg.ucDqpRoiTh = 3;
        if (pRcParam->iMotionAQStrength) {
    		sMAqCfg.cDqp[0] = pRcParam->iMotionAQStrength;
        }
        else {
            sMAqCfg.cDqp[0] = -16;
        }
        #if H26X_USE_DIFF_MAQ
        //if (0 != gDiffMAQStr && gDiffMAQNum > 1) {
        if (h26xEnc_checkDiffMAQEnable(pVar) == FALSE)
        #endif
        {
    		sMAqCfg.cDqp[1] = -16;//pRcParam->iMotionAQStrength;
        }
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

void h26XEnc_SetIspRatio(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatio)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

    memcpy(&pComnCtx->stIspRatio, pIspRatio, sizeof(H26XEncIspRatioCfg));
}

INT32 h26XEnc_setBgrCfg(H26XENC_VAR *pVar, H26XEncBgrCfg *pBgrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;


	// check parameter //

	memcpy(&pFuncCtx->stBgr.stCfg, pBgrCfg, sizeof(H26XEncBgrCfg));

	h26xEnc_wrapBgrCfg(pRegSet, &pFuncCtx->stBgr);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setRmdCfg(H26XENC_VAR *pVar, H26XEncRmdCfg *pIraCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;


	// check parameter //

	memcpy(&pFuncCtx->stRmd.stCfg, pIraCfg, sizeof(H26XEncRmdCfg));

	h26xEnc_wrapRmdCfg(pRegSet, &pFuncCtx->stRmd);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setTnrCfg(H26XENC_VAR *pVar, H26XEncTnrCfg *pTnrCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //
	memcpy(&pFuncCtx->stTnr.stCfg, pTnrCfg, sizeof(H26XEncTnrCfg));
	if (pComnCtx->uiPicCnt == 0) {
		pFuncCtx->stTnr.stCfg.nr_3d_mode = 0;
	}

	h26xEnc_wrapTnrCfg(pRegSet, &pFuncCtx->stTnr);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setLambdaCfg(H26XENC_VAR *pVar, H26XEncLambdaCfg *pLambdaCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	// check parameter //

	memcpy(&pFuncCtx->stLambda.stCfg, pLambdaCfg, sizeof(H26XEncLambdaCfg));

	h26xEnc_wrapLambdaCfg(pRegSet, &pFuncCtx->stLambda);

	return H26XENC_SUCCESS;
}

INT32 h26XEnc_setIspCbCfg(H26XENC_VAR *pVar, H26XEncIspCbCfg *pIspCbCfg)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	// check parameter //

    memcpy(&pComnCtx->stIspCbCfg, pIspCbCfg, sizeof(H26XEncIspCbCfg));

	return H26XENC_SUCCESS;
}

UINT32 h26XEnc_getUsrQpAddr(H26XENC_VAR *pVar)
{
	H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

	return pFuncCtx->stUsrQp.uiQpMapAddr;
}

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

INT32 h26xEnc_getNaluLenResult(H26XENC_VAR *pVar, H26XEncNaluLenResult *pResult)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = pVar->pComnCtx;

	pResult->uiSliceNum = pFuncCtx->stSliceSplit.uiNaluNum;
	pResult->uiVaAddr = pComnCtx->stVaAddr.uiNaluLen;

	h26x_cache_invalidate(pResult->uiVaAddr, pResult->uiSliceNum*4);

    if (pFuncCtx->stSliceSplit.uiNaluNum > H26XE_SLICE_MAX_NUM) {
        DBG_ERR("NALU number(%d) is over h/w constraint(176).\r\n", (int)pFuncCtx->stSliceSplit.uiNaluNum);
        return H26XENC_FAIL;
    }
    return H26XENC_SUCCESS;
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
		h26x_cache_invalidate(pComnCtx->stVaAddr.uiBsOutAddr, uiBsLen);
	}

	return uiBsLen;
}

void h26xEnc_getIspRatioCfg(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatioCfg)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	memcpy(pIspRatioCfg, &pComnCtx->stIspRatio, sizeof(H26XEncIspRatioCfg));
}

void h26xEnc_getRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRrcCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

	memcpy(pRrcCfg, &pFuncCtx->stRowRc, sizeof(H26XEncRowRcCfg));
}

#if H26X_SET_PROC_PARAM
int h26xEnc_getRCDumpLog(void)
{
    return g_rc_dump_log;
}
int h26xEnc_setRCDumpLog(int value)
{
    if (0 == value || 1 == value)
        g_rc_dump_log = value;
    return 0;
}
#endif  // H26X_SET_PROC_PARAM
#if JND_DEFAULT_ENABLE
int h26xEnc_getJNDParam(H26XEncJndCfg *param)
{
    memcpy(param, &gEncJNDCfg, sizeof(H26XEncJndCfg));
    return 0;
}
int h26xEnc_setJNDParam(UINT8 enable, UINT8 str, UINT8 level, UINT8 th)
{
	gEncJNDCfg.bEnable = enable;
	gEncJNDCfg.ucStr = str;
	gEncJNDCfg.ucLevel = level;
	gEncJNDCfg.ucTh = th;
    return 0;
}
#endif  // JND_DEFAULT_ENABLE

int h26xEnc_getNDQPStep(void)
{
	return gRRCNDQPStep;
}
int h26xEnc_setNDQPStep(int value)
{
	if (value >= 0 && value <= 15)
		gRRCNDQPStep = value;
	return 0;
}
int h26xEnc_getNDQPRange(void)
{
	return gRRCNDQPRange;
}
int h26xEnc_setNDQPRange(int value)
{
	if (value >= 0 && value <= 15)
		gRRCNDQPRange = value;
	return 0;
}
#if H26X_AUTO_RND
int h26xEnc_getAutoRndCond(void)
{
	return gAutoRndCondition;
}
int h26xEnc_setAutoRndCond(int value)
{
	if (value >= 0 && value <= 51)
		gAutoRndCondition = value;
	return 0;
}
#endif

#if H26X_SYNC_ROWRC_MAXMIN_QP
int h26xEnc_getRRCSyncQPCond(void)
{
	return g_RRCSyncRCQPCond;
}
int h26xEnc_setRRCSyncQPCond(int value)
{
	if (0 == value || 1 == value)
		g_RRCSyncRCQPCond = value;
	return 0;
}
#endif

#if H26X_DIS_OSG_CACHE_OPERATION
int h26xEnc_getOsgCacheFlush(void)
{
	return gOsgCacheFlush;
}
int h26xEnc_setOsgCacheFlush(int value)
{
	if (0 == gOsgCacheFlush || 1 == gOsgCacheFlush)
		gOsgCacheFlush = value;
	return 0;
}
#endif

UINT32 h26xEnc_getVersion(void)
{
    return NVT_H26XENC_VERSION;
}

#if H26X_MEM_USAGE
UINT32 h26xEnc_getMemUsage(UINT32 type, UINT32 id)
{
	return (UINT32)&(g_mem_usage[type][id]);
}
#endif

UINT32 h26xEnc_queryRecFrmNum(const H26XEncMeminfo *pInfo)
{
	return (1 + (pInfo->ucSVCLayer == 2) + (pInfo->uiLTRInterval != 0));
}

#if H26X_USE_DIFF_MAQ
void h26XEnc_setMAQDiffInfoCfg(H26XENC_VAR *pVar, H26XEncMAQDiffInfoCfg *pMAQDiffInfoCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XRegSet	 *pRegSet  = (H26XRegSet *)pComnCtx->stVaAddr.uiAPBAddr;

	if (pFuncCtx->stMAq.stMAQDiffInfoCfg.enable == FALSE) {
		pFuncCtx->stMAq.stMAQDiffInfo.ucCurMotIdx = 0;
		pFuncCtx->stMAq.stMAQDiffInfo.uiMotFrmCnt = 0;
	}

	memcpy(&pFuncCtx->stMAq.stMAQDiffInfoCfg, pMAQDiffInfoCfg, sizeof(H26XEncMAQDiffInfoCfg));

	h26xEnc_wrapMAQDiffCfg(pRegSet, pMAQDiffInfoCfg);
}

void h26XEnc_getMAQDiffInfoCfg(H26XENC_VAR *pVar, H26XEncMAQDiffInfoCfg *pMAQDiffInfoCfg)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;

	memcpy(pMAQDiffInfoCfg, &pFuncCtx->stMAq.stMAQDiffInfoCfg, sizeof(H26XEncMAQDiffInfoCfg));
}
#endif
