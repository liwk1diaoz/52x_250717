#ifndef _KDRV_IME_INT_H_
#define _KDRV_IME_INT_H_


#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ime.h"

#include "ime_lib.h"


/**
    max ime dal handle number, current max is 8 due to flag config
*/
#define KDRV_IME_HANDLE_MAX_NUM (KDRV_IPP_CBUF_MAX_NUM)

#define KDRV_IME_SCALE_BASE                     (100)   //x100
//#define KDRV_IME_SCALING_ISD_STP_OUT_H_LIMIT    (1024)
#define KDRV_IME_SCALING_MAX_RATIO          (3199)  //x100, ime scale maximum ratio
#define KDRV_IME_SCALING_ISD_MAX_RATIO      (1599)  //x100, ime isd scale down maximum ratio

//LCA Sub Out Limitation
#define KDRV_IME_LCA_SUB_OUT_H_SCALE_LIMIT  (207)   //x100, lca sub out h scale down ratio need over than 2.07

#if 0
#define KDRV_IME_DBCS_WT_LUT_TAB (16)

#define KDRV_IME_TPLNR_DETAL_PENALTY_TAB (6)
#define KDRV_IME_TPLNR_SAD_PENALTY_TAB   (8)
#define KDRV_IME_TPLNR_SNR_RATIO_TAB     (6)
#define KDRV_IME_TPLNR_TNR_RATIO_TAB     (6)
#define KDRV_IME_TPLNR_SQR_STR_GAIN_TAB  (8)
#define KDRV_IME_TPLNR_FREQ_RATIO_TAB    (7)
#define KDRV_IME_TPLNR_SNR_FREQ_TAB         (4)
#define KDRV_IME_TPLNR_TNR_STR_GAIN_TAB  (8)
#define KDRV_IME_TPLNR_MOT_TEN_VAL_TAB   (4)
#define KDRV_IME_TPLNR_MOT_CTRL_TAB         (16)
#define KDRV_IME_TPLNR_2D_WET_TAB       (4)
#define KDRV_IME_TPLNR_3D_WET_TAB       (4)
#define KDRV_IME_TPLNR_MIX_TH_TAB       (4)
#endif

// funtion type for dal_sie
typedef ER(*KDRV_IME_SET_FP)(UINT32, void *);
typedef ER(*KDRV_IME_GET_FP)(UINT32, void *);


typedef enum _KDRV_IME_ID {
	KDRV_IME_ID_0 = 0,                                  ///< ISE id 1
	KDRV_IME_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_IME_ID)
} KDRV_IME_ID;


/**
    IME Output Image Information
*/
typedef struct _KDRV_IME_OUT_PATH_INFO {
	BOOL                        path_en;
	BOOL                        flip_en;
	KDRV_IME_OUT_FMT            type;                   ///< image type
	KDRV_IME_DMA_ADDR_INFO      pixel_addr;             ///< pixel YUV address
	KDRV_IME_YCC_CH_INFO        ch[KDRV_IPP_YUV_MAX_CH]; ///< image channel information
	URECT                       crp_window;
	KDRV_IME_DMA_ADDR_INFO      out_sprt_addr;          ///< path output DMA address for separating output
	KDRV_IME_OUT_SPRT_INFO      out_sprt_info;          ///< IME output separate info, option
	KDRV_IME_ENCODE_INFO        out_encode_info;        ///< IME output encode info, only valid for path1
	KDRV_IME_CLAMP_INFO         out_range_clamp;        ///< IME output range clamp

	KDRV_IME_BUF_FLUSH_SEL      out_dma_flush;          ///< output dam buffer flush
} KDRV_IME_OUT_PATH_INFO;

/**
    KDRV IME TMNR parameters
*/
typedef struct _KDRV_IME_TMNR {
	KDRV_IME_TMNR_PARAM                     nr_iq;
	KDRV_IME_TMNR_REF_IN_IMG_INFO           nr_ref_in_img;
	KDRV_IME_TMNR_MOTION_STATUS_INFO        nr_ms;
	KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO    nr_ms_roi;
	KDRV_IME_TMNR_MOTION_VECTOR_INFO        nr_mv;
	KDRV_IME_TMNR_STATISTIC_DATA_INFO       nr_sta;
	KDRV_IME_TMNR_DBG_PARAM                 nr_dbg;
} KDRV_IME_TMNR;


/**
    KDRV IME TMNR parameters
*/
typedef struct _KDRV_IME_TMNR_REF_OUT {
	KDRV_IME_TMNR_REF_OUT_IMG_INFO          nr_ref_out_img;
} KDRV_IME_TMNR_REF_OUT;




/**
    KDRV IME STL parameters
*/
typedef struct _KDRV_IME_STL {
	KDRV_IME_STL_INFO           stl_info;
	KDRV_IME_STL_ROI_INFO       stl_roi;
	KDRV_IME_STL_HIST_INFO      stl_hist;
	KDRV_IME_STL_OUT_IMG_INFO   stl_out_img;
} KDRV_IME_STL;

/**
    KDRV IME PM parameters
*/
typedef struct _KDRV_IME_PM {
	KDRV_IME_PM_INFO                pm_info[KDRV_IME_PM_SET_IDX_MAX];   ///< privacy mask parameters
	KDRV_IME_PM_PXL_IMG_INFO        pm_pxl_img_info;                    ///< primacy mask pixel image infor
} KDRV_IME_PM;




typedef struct _KDRV_IME_SUB_MODULES {
	KDRV_IME_CMF_PARAM          chroma_filter;  ///< chroma median filter parameter
	KDRV_IME_LCA_PARAM          lca;        ///< chroma adaption parameters
	KDRV_IME_LCA_CTRL_PARAM     lca_en_ctrl;  ///< chroma adaption parameters
	KDRV_IME_LCA_CA_CENT_INFO   lca_ca_cent;    ///< chroma adaption CA center U/V parameters
	KDRV_IME_LCA_IMG_INFO       lca_img;        ///< chroma adaption Reference image information
	KDRV_IME_LCA_SUBOUT_INFO    lca_subout; ///< chroma adaption subout parameters
	KDRV_IME_DBCS_PARAM         dbcs;          ///< dark and bright region chroma supression parameters
	KDRV_IME_SSR_PARAM          ssr;           ///< single image supre-resolution parameters
	KDRV_IME_RGB2YCC_INFO       rgb2ycc;        ///< RGB to YCbCr
	KDRV_IME_YCC2RGB_INFO       ycc2rgb;        ///< YCbCr to RGB
	KDRV_IME_FGN_PARAM          film_grain;    ///< film grain parameters
	KDRV_IME_OSD_INFO           ds[KDRV_IME_OSD_SET_IDX_MAX];   ///< Data stamp parameters
	KDRV_IME_OSD_CST_INFO       ds_cst;     ///< Data stamp color space transform parameters
	KDRV_IME_OSD_PLT_INFO       ds_plt;     ///< OSD palette table
	KDRV_IME_P2I_INFO           p2i;             ///< progressive to interlaced parameter
	KDRV_IME_STL                stl;            ///< statistical parameters
	KDRV_IME_PM                 pm;             ///< primacy mask parameters
	KDRV_IME_TMNR               tmnr;          ///< tmnr parameters
	KDRV_IME_TMNR_REF_OUT       tmnr_refout;
	KDRV_IME_YCC_CVT_PARAM      yuv_cvt;       ///< YUV converter
} KDRV_IME_SUB_MODULES;

/**
    IME KDRV structure
*/
typedef struct _KDRV_IME_PRAM {
	// input info
	KDRV_IME_DMA_ADDR_INFO          in_pixel_addr;      ///< pixel YUV address
	KDRV_IME_IN_INFO                in_img_info;        ///< IME input image info
	KDRV_IME_EXTEND_INFO            extend_info;        ///< IME extend info

	//output info
	KDRV_IME_OUT_PATH_INFO          out_path_info[KDRV_IME_PATH_ID_MAX];
	KDRV_IME_SCL_METHOD_SEL_INFO    out_scl_method_sel;
	KDRV_IME_SUB_MODULES            sub_modules;

	// other control
	UINT32                          inte_en;
	KDRV_IME_BREAK_POINT_INFO       break_point_ctrl;
	KDRV_IME_SINGLE_OUT_INFO        single_out_ctrl;
	KDRV_IME_LOW_DELAY_INFO         low_delay_ctrl;
} KDRV_IME_PRAM, *PKDRV_IME_PRAM;


/**
    IME KDRV handle structure
*/
#define KDRV_IME_HANDLE_LOCK    0x00000001

typedef struct _KDRV_IME_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_IME_HANDLE;



#endif //_KDRV_IME_INT_H_


