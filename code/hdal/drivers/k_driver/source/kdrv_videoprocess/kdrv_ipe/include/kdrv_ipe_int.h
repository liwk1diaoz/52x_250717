#ifndef _KDRV_IPE_INT_H_
#define _KDRV_IPE_INT_H_

#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "ipe_lib.h"


//static var

/**
    max ipe kdrv handle number, current max is 8 due to flag config
*/
#define KDRV_IPE_HANDLE_MAX_NUM (KDRV_IPP_CBUF_MAX_NUM)

#define KDRV_IPE_MAX_INTEEN_NUM 7

// funtion type for kdrv_ipe
typedef ER(*KDRV_IPE_SET_LOAD_FP)(INT32, IPE_IQINFO *);
typedef ER(*KDRV_IPE_SET_FP)(UINT32, void *);
typedef ER(*KDRV_IPE_GET_FP)(UINT32, void *);

INT32 kdrv_ipe_dma_buffer_allocate(void);
INT32 kdrv_ipe_dma_buffer_free(void);

/**
    IPE function set/enable/disable selection

    Select IPE function update mode
*/
typedef enum {
	KDRV_IPE_FUNC_ENABLE = 0,        ///< Enable functions, FuncEn |= FuncSel, 1s in FuncSel are enabled
	KDRV_IPE_FUNC_DISABLE = 1,       ///< Disable functions, FuncEn &= (~FuncSel), 1s in FuncSel are disabled
	KDRV_IPE_FUNC_SET = 2,           ///< Set functions, FuncEn = FuncSel, 1s in FuncSel are enabled while 0s in FuncSel are disabled
	KDRV_IPE_FUNC_NOUPDATE = 3,      ///< Don't update functions, FuncEn = FuncEn, disregard FuncSel
	ENUM_DUMMY4WORD(KDRV_IPE_FUNC_UPDATE_ENUM)
} KDRV_IPE_FUNC_UPDATE_ENUM;

/**
    IPE set load item

    Select IPE set load item
*/
typedef enum {
	DRV_IPE_SETLAOD_IQ_EEXT,
	DRV_IPE_SETLAOD_IQ_EPROC,
	DRV_IPE_SETLAOD_IQ_RGBLPF,
	DRV_IPE_SETLAOD_IQ_PFR,
	DRV_IPE_SETLAOD_IQ_CC,
	DRV_IPE_SETLAOD_IQ_CCTRL,
	DRV_IPE_SETLAOD_IQ_CADJ_EE,
	DRV_IPE_SETLAOD_IQ_CADJ_YCCON,
	DRV_IPE_SETLAOD_IQ_CADJ_COFS,
	DRV_IPE_SETLAOD_IQ_CADJ_RAND,
	DRV_IPE_SETLAOD_IQ_CADJ_HUE,
	DRV_IPE_SETLAOD_IQ_CADJ_FIXTH,
	DRV_IPE_SETLAOD_IQ_CADJ_MASK,
	DRV_IPE_SETLAOD_IQ_CST,
	DRV_IPE_SETLAOD_IQ_CSTP,
	DRV_IPE_SETLAOD_IQ_GAMYRAND,
	DRV_IPE_SETLAOD_IQ_GAMMA,
	DRV_IPE_SETLAOD_IQ_YCURVE,
	DRV_IPE_SETLAOD_IQ_3DCC,
	DRV_IPE_SETLAOD_IQ_DEFOG_SUBIMG,
	DRV_IPE_SETLAOD_IQ_DEFOG,
	DRV_IPE_SETLAOD_IQ_LCE,
	DRV_IPE_SETLAOD_IQ_EDGEDBG,
	DRV_IPE_SETLAOD_IQ_ETH,
	DRV_IPE_SETLAOD_IQ_VA,
	DRV_IPE_SETLAOD_IQ_EDGE_REGION_STR,
	DRV_IPE_SETLAOD_IQ_MAX,
	DRV_IPE_SETLAOD_IQ_REV = 0x80000000,
	ENUM_DUMMY4WORD(DRV_IPE_SETLOAD_IQ_ID)
} DRV_IPE_SETLOAD_IQ_ID;


/**
    IPE KDRV structure
*/
typedef struct {

	// input info
	KDRV_IPE_DMA_ADDR_INFO      in_pixel_addr;       ///< pixel YUV address
	KDRV_IPE_IN_IMG_INFO        in_img_info;         ///< IPE input image info

	//output info
	KDRV_IPE_DMA_ADDR_INFO      out_pixel_addr;      ///< pixel YUV address
	KDRV_IPE_OUT_IMG_INFO       out_img_info;        ///< IPE output image info

	// dma in/out
	KDRV_IPE_DMA_SUBIN_ADDR     subin_dma_addr;      ///< IPE subimg input address info
	KDRV_IPE_DMA_SUBOUT_ADDR    subout_dma_addr;     ///< IPE subimg output address info

	// dram single output
	KDRV_IPE_DRAM_OUTPUT_MODE   dram_out_mode;
	KDRV_IPE_DRAM_SINGLE_OUT_EN  single_out_ch;

	// interrupt enable
	KDRV_IPE_INTE                   inte_en;         ///< IPE interrupt enable

	// IQ function enable
	//UINT32 func_sel;                               ///< IPE function set/enable/disable selection, combined usage with FuncUpdateSel
	//KDRV_IPE_FUNC_UPDATE_ENUM func_update_sel;     ///< IPE function set/enable/disable selection
	//UINT32 param_update_sel;                       ///< IPE parameters update selection, combined usage with definition IPE_SET_XXX

	// IQ parameter
	KDRV_IPE_EEXT_PARAM eext_param;                  ///< Edge extraction parameters
	KDRV_IPE_EEXT_TONEMAP_PARAM eext_tonemap_param;  ///< Edge extraction tone mapping parameters
	KDRV_IPE_EDGE_OVERSHOOT_PARAM overshoot_param;   ///< Edge extraction parameters
	KDRV_IPE_EPROC_PARAM eproc_param;                ///< Edge process parameters
	KDRV_IPE_RGBLPF_PARAM rgb_lpf_param;             ///< RGB LPF parameters
	KDRV_IPE_PFR_PARAM  pfr_param;                   ///< Purple Fringer Reduction parameters
	KDRV_IPE_CC_EN_PARAM cc_en_param;                ///< Color correction enable parameters
	KDRV_IPE_CC_PARAM cc_param;                      ///< Color correction strength parameters
	KDRV_IPE_CCM_PARAM ccm_param;                    ///< Color correction matix parameters
	KDRV_IPE_CCTRL_EN_PARAM cctrl_en_param;          ///< Color control enable parameters
	KDRV_IPE_CCTRL_PARAM cctrl_param;                ///< Color control parameters
	KDRV_IPE_CCTRL_CT_PARAM cctrl_ct_param;          ///< Color control parameters by color temperature
	KDRV_IPE_CADJ_EE_PARAM cadj_ee_param;            ///< Color componenet adjustment: edge enhancement/inversion parameters
	KDRV_IPE_CADJ_YCCON_PARAM cadj_yccon_param;      ///< Color componenet adjustment: YC contrast parameters
	KDRV_IPE_CADJ_COFS_PARAM cadj_cofs_param;        ///< Color componenet adjustment: color offset parameters
	KDRV_IPE_CADJ_RAND_PARAM cadj_rand_param;        ///< Color componenet adjustment: random noise parameters
	KDRV_IPE_CADJ_HUE_PARAM cadj_hue_param;          ///< Hue adjustment parameters
	KDRV_IPE_CADJ_FIXTH_PARAM cadj_fixth_param;      ///< Color componenet adjustment: YC threshold parameters
	KDRV_IPE_CADJ_MASK_PARAM cadj_mask_param;        ///< Color componenet adjustment: YC mask parameters
	KDRV_IPE_CST_PARAM cst_param;                    ///< Color space transform parameters
	KDRV_IPE_CSTP_PARAM cstp_param;                  ///< Color space transform protection parameters
	KDRV_IPE_GAMYRAND_PARAM gamy_rand_param;         ///< Gamma and Y curve: random output parameters
	KDRV_IPE_GAMMA_PARAM gamma;                      ///< Gamma curve
	KDRV_IPE_YCURVE_PARAM y_curve;                   ///< Y curve
	//KDRV_IPE_3DCC_PARAM _3dcc_param;                 ///< 3DCC parameters
	KDRV_IPE_DEFOG_PARAM dfg_param;                  ///< Defog parameters
	KDRV_IPE_LCE_PARAM lce_param;                    ///< LCE parameters
	KDRV_IPE_SUBIMG_PARAM subimg_param;              ///< IPE defog subimg image info
	KDRV_IPE_EDGEDBG_PARAM edgdbg_param;             ///< Edge debug mode parameters
	KDRV_IPE_ETH_INFO eth;                           ///< Edge thresholding parameters
	KDRV_IPE_DMA_VA_ADDR va_addr;                    ///< va address
	KDRV_IPE_VA_WIN_INFO va_win_info;                ///< va windon size parameters
	KDRV_IPE_VA_PARAM va_param;                      ///< va parameters

	// Extend info
	KDRV_IPE_EXTEND_INFO ext_info;                   ///< IPE Extend info

	KDRV_IPE_EDGE_REGION_PARAM edge_region_param;   ///< Edge region strength control parameters
} KDRV_IPE_PRAM, *pKDRV_IPE_PRAM;



typedef struct _KDRV_IPE_IQ_EN_PARAM_ {
	uint8_t rgb_lpf_param_enable;
	uint8_t cc_en_param_enable;
	uint8_t cst_param_enable;
	uint8_t cctrl_en_param_enable;
	uint8_t cadj_ee_param_enable;
	uint8_t cadj_yccon_param_enable;
	uint8_t cadj_cofs_param_enable;
	uint8_t cadj_rand_param_enable;
	uint8_t cadj_hue_param_enable;
	uint8_t cadj_fixth_param_enable;
	uint8_t cadj_mask_param_enable;
	uint8_t cstp_param_enable;
	uint8_t gamy_rand_param_enable;
	uint8_t gamma_enable;
	uint8_t y_curve_enable;
	//uint8_t _3dcc_param_enable;
	uint8_t pfr_param_enable;
	uint8_t dfg_param_enable;
	uint8_t lce_param_enable;
	uint8_t edgdbg_param_enable;
	uint8_t va_param_enable;
	uint8_t va_param_indep_va_enable;
	uint8_t edge_region_str_enable;
} KDRV_IPE_IQ_EN_PARAM;




/**
    IPE KDRV handle structure
*/
#define KDRV_IPE_HANDLE_LOCK    0x00000001

typedef struct _KDRV_IPE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_IPE_HANDLE;
#endif //_KDRV_IPE_INT_H_
