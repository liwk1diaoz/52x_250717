#ifndef _H26X_DEF_H_
#define _H26X_DEF_H_

#include "kwrap/type.h"

/*
0.2.03(2020/04/23): support motion ratio & jnd
0.3.01(2020/05/12): update tile settings. remove unused condition.
			correct search range setting for h264
			correct h/w limitation description for 520 and add h/w limitation description for 528
			update register map to 528 format
			support osg chroma alpha for 528
0.3.02(2020/05/19): update h/w limitation description
			support 528 feature
0.3.03(2020/05/27): 1. set rrc ndqp and rrc stop = 0.05, 2. update vui aspect ratio
0.3.04(2020/06/01): rrc by frame level
0.3.05(2020/06/08): support dynamic change gop
0.3.06(2020/12/11): disable tnr when first frame
0.3.07(2021/01/05): h265 update ltr frame syntax
0.3.08(2021/06/01): support maq diff api
0.3.09(2021/12/01): add coeff cost thd to vendor API
0.3.10(2022/03/01): support gdr in i frame
0.3.11(2022/03/15): add auto rnd
0.3.12(2022/03/31): fix buf of H265 header error when ltr=1
0.3.13(2022/04/15): optimizate MD scaling function
0.3.14(2022/05/30): disable codec tnr when ltr frame
0.3.15(2022/06/06): disable codec tnr when ltr frame (H265)
0.3.16(2022/07/20): add h265 d2d max width definition
0.3.17(2022/09/14): support h264 chroma qp offset header syntax (IVOT_N12061_CO-398)
0.3.18(2022/09/21): add h264 fro sync proc (NA51102-2483)
0.3.19(2022/11/01): update h264 ira_modebits default value (NA51084-551)
0.3.20(2022/11/29): remove maq diff for cim issue, because maq diff vendor cmd is ready (NA51084-577)
0.3.21(2023/01/19): support MD rotation/crop/scaling process
0.3.22(2023/04/25): support bsout buffer full re-trigger (NA51084-597)
0.3.23(2023/05/04): fix bsout flush size for bsout buffer full re-trigger (NA51084-597)
0.3.24(2023/06/27): fix low lantecy patch issue (IVOT_N12078_CO-118)
0.3.25(2023/07/26): disable cache flush of OSG (NA51055-2441)
0.3.26(2023/08/14): remove unneccsarry code (IVOT_N12078_CO-152)
0.3.27(2023/12/21): support venc skip frame (IVOT_N12070_CO-580)
*/

#define NVT_H26XENC_VERSION     0x00031b00

#ifdef CONFIG_NVT_SMALL_HDAL
	#define H26X_SAVE_KO_SIZE	(1)
#else
	#define H26X_SAVE_KO_SIZE	(0)
#endif


#define H26X_MIN_MB			(16)
//#define H26X_MAX_BSDMA_NUM	(H26X_MAX_HEIGHT/H26X_MIN_MB)
#define H26X_MAX_BSDMA_NUM  (15)//(0x2000)
#define H26X_MAX_ROI_W		(10)
#define H26X_ENC_MODE		(1)	// 0 : link-list , 1 : direct mode
#if H26X_SAVE_KO_SIZE
#define H264_P_REDUCE_16_PLANAR (0)
#else
#define H264_P_REDUCE_16_PLANAR (1)
#endif
#define H26X_USE_DIFF_MAQ       (1)
#define JND_DEFAULT_ENABLE      (0)
#define LPM_PROC_ENABLE			(0)
#define H26X_MEM_USAGE          (0)
#define H26X_SET_PROC_PARAM		(1)
#define H26X_MOTION_BITMAP_NUM  (10)
#define H26X_MAQ_MORPHOLOGY     (0)
#define H26X_MAQ_DIFF_SPECIFIC_FRM         (1)
#define H26X_AUTO_RND			(1)
#define H26X_KEYP_DISABLE_TNR	(1)
#define H264_FRO_IP_SYNC		(1)
#define H26X_SYNC_ROWRC_MAXMIN_QP	(1)
#define H26X_DIS_OSG_CACHE_OPERATION	(1)

#define H26XENC_BS_OUTBUF_MAX_SIZE 0x800000
#define H26X_MAX_W_WITHOUT_TILE_V36 (1536)
#define H26X_MAX_W_WITHOUT_TILE_V20 (2048)
#define H26X_MAX_TILE_NUM	    (5)
#define H26X_FIRST_TILE_MIN_W   (576)
#define H26X_MIDDLE_TILE_MIN_W  (576)
#define H26X_LAST_TILE_MIN_W    (448)

#define H26X_FIRST_TILE_MAX_W_SR36   (1024)
#define H26X_MIDDLE_TILE_MAX_W_SR36  (896)
#define H26X_LAST_TILE_MAX_W_SR36    (1024)
#define H26X_FIRST_TILE_MAX_W_SR28   (1408)
#define H26X_MIDDLE_TILE_MAX_W_SR28  (1280)
#define H26X_LAST_TILE_MAX_W_SR28    (1408)
#define H26X_MIN_W_TILE              (H26X_FIRST_TILE_MIN_W+H26X_LAST_TILE_MIN_W) // 1024
#define H26X_SR36_MAX_W              (H26X_FIRST_TILE_MAX_W_SR36+H26X_MIDDLE_TILE_MAX_W_SR36*3+H26X_LAST_TILE_MAX_W_SR36) // 4736
#define H26X_SR28_MAX_W              (H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28*3+H26X_LAST_TILE_MAX_W_SR28) // 6656

#define H26X_TILE_NUM_528		(4)
#define H26X_TILE_WIDTH_V52_528		(1536)

#define H26X_FIRST_TILE_MAX_W_SR52_528	(1408)
#define H26X_MIDDLE_TILE_MAX_W_SR52_528	(1280)
#define H26X_LAST_TILE_MAX_W_SR52_528	(1408)

#define H26X_SR52_MAX_W_528		(H26X_TILE_WIDTH_V52_528*H26X_TILE_NUM_528 - 256*(H26X_TILE_NUM_528-1))

#define H26X_FIRST_TILE_MAX_W_SR36_528	(2048)
#define H26X_MIDDLE_TILE_MAX_W_SR36_528	(1920)
#define H26X_LAST_TILE_MAX_W_SR36_528	(2048)

#define H26X_MAX_W_WITHOUT_TILE_V36_528 (2176)
#define H26X_MAX_W_WITHOUT_TILE_V28_528 (2560)
#define H26X_MAX_W_WITHOUT_TILE_V20_528 (4096)

#define RRC_BY_FRAME_LEVEL		1
#define MAX_RRC_FRAME_LEVEL		6

#define H264E_GDRI_MIN_SLICE_HEIGHT		144
#define H265E_GDRI_MIN_SLICE_HEIGHT		192

#define SIZE_512X(a)			((((a) + 511)>>9)<<9)
#define SIZE_256X(a)			((((a) + 255)>>8)<<8)
#define SIZE_128X(a)			((((a) + 127)>>7)<<7)
#define SIZE_64X(a)				((((a) + 63)>>6)<<6)
#define SIZE_32X(a)				((((a) + 31)>>5)<<5)
#define SIZE_16X(a)				((((a) + 15)>>4)<<4)
#define SIZE_8X(a)				((((a) + 7)>>3)<<3)
#define SIZE_4X(a)				((((a) + 3)>>2)<<2)

#define H26X_SWAP(A,B,TT) {TT TMP;\
                      TMP=A;\
                      A=B;\
                      B=TMP;}

#define IS_ISLICE(X) (X == I_SLICE || X == IDR_SLICE)
#define IS_PSLICE(X) (X == P_SLICE)
#define IS_IDRSLICE(X) (X == IDR_SLICE)

typedef enum{
	H26XENC_FAIL = 0,
	H26XENC_SUCCESS,
}H26XEncStatus;

typedef enum{
	VCODEC_H265 = 0,
	VCODEC_H264 = 1,
	VCODEC_NON_DEFINE
}CODEC_TYPE;

typedef enum{
    P_SLICE = 0,
    B_SLICE = 1,
    I_SLICE = 2,
    IDR_SLICE = 3,
    UNSET_SLICE_TYPE = 0xFF,
}SLICE_TYPE;

typedef enum {
	CU16_QP_DEFAULT = 0,
	CU16_QP_DISABLE_AQVAR = 1,
	CU16_QP_DISABLE_AQ = 2,
	CU16_QP_FIXEDQP = 3,
} CU16_QP_MODE;

typedef struct _H26XENC_VAR_{
	CODEC_TYPE eCodecType;

    UINT32 uiEncId;
	UINT32 uiCtxSize;

    void   *pVdoCtx;
	void   *pFuncCtx;
	void   *pComnCtx;
} H26XENC_VAR;

typedef struct _H26XDEC_VUI_{
	BOOL   bPresentFlag;
	UINT32 uiSarWidth;
	UINT32 uiSarHeight;
	UINT8  ucMatrixCoeff;
	UINT8  ucTransChar;
	UINT8  ucColorPrimaries;
	UINT8  ucVideoFmt;
	UINT8  ucColorRange;
	BOOL   bTimingPresentFlag;
} H26XDEC_VUI;

typedef struct _H26XDEC_VAR_{
	CODEC_TYPE eCodecType;

	UINT32 uiAPBAddr;
	UINT32 uiCtxSize;
	H26XDEC_VUI stVUI;

    void   *pContext;
} H26XDEC_VAR;

#endif

