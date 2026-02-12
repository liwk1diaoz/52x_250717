#ifndef _KDRV_DCE_INT_H_
#define _KDRV_DCE_INT_H_

#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "dce_lib.h"


typedef ER(*KDRV_DCE_SET_FP)(UINT32, void *);
typedef ER(*KDRV_DCE_GET_FP)(UINT32, void *);

/**
    max dce kdrv handle number, current max is 8 due to flag config
*/
#define KDRV_DCE_HANDLE_MAX_NUM (KDRV_IPP_CBUF_MAX_NUM)

typedef struct _KDRV_DCE_PRAM {
	//IPL struct
	KDRV_DCE_TRIG_TYPE_INFO     trig_cfg;
	KDRV_DCE_IN_INFO            in_info;          ///< DCE input image info
	KDRV_DCE_IN_IMG_INFO        in_img_info;
	KDRV_DCE_OUT_IMG_INFO       out_img_info;
	KDRV_DCE_IMG_IN_DMA_INFO    in_dma_info;      ///< pixel YUV address
	KDRV_DCE_IMG_OUT_DMA_INFO   out_dma_info;
	KDRV_DCE_SUBIMG_IN_ADDR     wdr_subin_addr;
	KDRV_DCE_SUBIMG_OUT_ADDR    wdr_subout_addr;
	KDRV_DCE_DRAM_OUTPUT_MODE   dram_out_mode;
	KDRV_DCE_DRAM_SINGLE_OUT_EN single_out_ch;
	KDRV_DCE_GDC_CENTER_INFO    gdc_center;
	KDRV_DCE_DC_SRAM_SEL        dc_sram_sel;
	KDRV_DCE_STRIPE_INFO        ipl_stripe;
	KDRV_DCE_EXTEND_INFO        extend_info;
	KDRV_DCE_STRP_RULE_INFO     stripe_rule_info;
	//PQ struct
	KDRV_DCE_STRIPE_PARAM       stripe_param;
	KDRV_DCE_CFA_PARAM          cfa_param;
	KDRV_DCE_DC_CAC_PARAM       dc_cac_param;
	KDRV_DCE_2DLUT_PARAM        lut_2d_param;
	KDRV_DCE_FOV_PARAM          fov_param;
	KDRV_DCE_WDR_SUBIMG_PARAM   wdr_subimg_param;
	KDRV_DCE_WDR_PARAM          wdr_param;
	KDRV_DCE_HIST_PARAM         hist_param;
	KDRV_DCE_CFA_SUBOUT_INFO    cfa_subout_info;

	KDRV_DCE_INTE               inte_en;            ///< DCE interrupt enable
} KDRV_DCE_PRAM, *pKDRV_DCE_PRAM;


/**
    DCE KDRV handle structure
*/
#define KDRV_DCE_HANDLE_LOCK    0x00000001

typedef struct _KDRV_DCE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_DCE_HANDLE;

#endif //_KDRV_DCE_INT_H_
