/**
    Public header file for kdal_ife

    This file is the header file that define the API and data type for dal_ife.

    @file       kdrv_ife.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IFE_H_
#define _KDRV_IFE_H_

#define NAME_ERR         1

#define RDE_Q_TAB_NUM      8
#define RDE_DEGAMMA_NUM    33

#define NRS_ORD_W_NUM       8
#define NRS_B_OFS_NUM       6
#define NRS_B_W_NUM         6
#define NRS_B_RANGE_NUM     5
#define NRS_B_TH_NUM        5
#define NRS_GB_OFS_NUM      6
#define NRS_GB_W_NUM        6
#define NRS_GB_RANGE_NUM    5
#define NRS_GB_TH_NUM       5


#define FCURVE_Y_W_NUM      17
#define FCURVE_IDX_NUM      32
#define FCURVE_SPLIT_NUM    32
#define FCURVE_VAL_NUM      65

#define FUSION_MC_DIFF_W_NUM         16
#define FUSION_SHORT_COMP_KNEE_NUM   3
#define FUSION_SHORT_COMP_SUB_NUM    4
#define FUSION_SHORT_COMP_SHIFT_NUM  4


#define FUSION_BLD_CUR_KNEE_NUM  1


#define FUSION_DARK_SAT_NUM  2

#define FCGAIN_CH_NUM     5

#define OUTL_BRI_TH_NUM   5
#define OUTL_DARK_TH_NUM  5
#define OUTL_CNT_NUM      2
#define OUTL_ORD_W_NUM    8

#define SPATIAL_W_LEN     6

#define RANGE_A_TH_NUM    6
#define RANGE_B_TH_NUM    6
#define RANGE_A_LUT_SIZE  17
#define RANGE_B_LUT_SIZE  17

#define CGAIN_LUT_SIZE    5
#define COFS_LUT_SIZE     5

#define VIG_CH0_LUT_SIZE  17
#define VIG_CH1_LUT_SIZE  17
#define VIG_CH2_LUT_SIZE  17
#define VIG_CH3_LUT_SIZE  17

#define GBAL_OFS_NUM    17

#define RBFILL_LUMA_NUM    17
#define RBFILL_RATIO_NUM   32


/**
    IFE KDRV interrupt enable enum.
*/
typedef enum _KDRV_IFE_INTE {
	KDRV_IFE_INTE_FMD             = 0x00000001,  ///< enable interrupt: frame end
	KDRV_IFE_INTE_DEC1_ERR        = 0x00000002,  ///< enable interrupt: rde decode error interrupt
	KDRV_IFE_INTE_DEC2_ERR        = 0x00000004,  ///< enable interrupt: rde decode error interrupt
	KDRV_IFE_INTE_LLEND           = 0x00000008,  ///< enable interrupt: LinkedList end interrupt
	KDRV_IFE_INTE_LLERR           = 0x00000010,  ///< enable interrupt: LinkedList error  interrupt
	KDRV_IFE_INTE_LLERR2          = 0x00000020,  ///< enable interrupt: LinkedList error2 interrupt
	KDRV_IFE_INTE_LLJOBEND        = 0x00000040,  ///< enable interrupt: LinkedList job end interrupt
	KDRV_IFE_INTE_BUFOVFL         = 0x00000080,  ///< enable interrupt: buffer overflow interrupt
	KDRV_IFE_INTE_RING_BUF_ERR    = 0x00000100,  ///< enable interrupt: ring buffer error interrupt
	KDRV_IFE_INTE_FRAME_ERR       = 0x00000200,  ///< enable interrupt: frame error interrupt (for direct mode)
	KDRV_IFE_INTE_SIE_FRAME_START = 0x00001000,  ///< enable interrupt: SIE frame start interrupt (for direct mode)
	KDRV_IFE_INTE_ALL             = 0x000013ff,   ///< all interrupt enable mask
	ENUM_DUMMY4WORD(KDRV_IFE_INTE)
} KDRV_IFE_INTE;

/**
    type for KDRV_IFE_SET_ISRCB
*/
typedef enum {
	KDRV_IFE_INT_FMD             = 0x00000001,  ///< interrupt: frame end
	KDRV_IFE_INT_DEC1_ERR        = 0x00000002,  ///< interrupt: rde decode error interrupt
	KDRV_IFE_INT_DEC2_ERR        = 0x00000004,  ///< interrupt: rde decode error interrupt
	KDRV_IFE_INT_LLEND           = 0x00000008,  ///< interrupt: LinkedList end interrupt
	KDRV_IFE_INT_LLERR           = 0x00000010,  ///< interrupt: LinkedList error  interrupt
	KDRV_IFE_INT_LLERR2          = 0x00000020,  ///< interrupt: LinkedList error2 interrupt
	KDRV_IFE_INT_LLJOBEND        = 0x00000040,  ///< interrupt: LinkedList job end interrupt
	KDRV_IFE_INT_BUFOVFL         = 0x00000080,  ///< interrupt: buffer overflow interrupt
	KDRV_IFE_INT_RING_BUF_ERR    = 0x00000100,  ///< interrupt: ring buffer error interrupt
	KDRV_IFE_INT_FRAME_ERR       = 0x00000200,  ///< interrupt: frame error interrupt (for direct mode)
	KDRV_IFE_INT_SIE_FRAME_START = 0x00001000,  ///< interrupt: SIE frame start interrupt (for direct mode)
	KDRV_IFE_INT_ALL             = 0x000013ff,   ///< interrupt: all
	ENUM_DUMMY4WORD(KDRV_IFE_ISR_STATUS)
} KDRV_IFE_ISR_STATUS;

typedef void (*KDRV_IFE_ISRCB)(KDRV_IFE_ISR_STATUS, void *);

typedef enum _KDRV_IFE_IN_MODE {
	KDRV_IFE_IN_MODE_D2D = 0,     //input from dram
	KDRV_IFE_IN_MODE_IPP,         //IPP flow
	KDRV_IFE_IN_MODE_DIRECT,      //All direct flow
	ENUM_DUMMY4WORD(KDRV_IFE_IN_MODE)
} KDRV_IFE_IN_MODE;

/**
    IFE KDRV - input image data format selection
*/
typedef enum _KDRV_IFE_IN_FMT {
	KDRV_IFE_IN_BAYER     = 0,     ///< RAW data input bayer
	KDRV_IFE_IN_RGBIR     = 1,     ///< RAW data input input
	ENUM_DUMMY4WORD(KDRV_IFE_IN_FMT)
} KDRV_IFE_IN_FMT;

/**
    RAW Image Information
*/
typedef struct _KDRV_IFE_RAW_INFO {
	UINT32           width;        ///< image width         (pixel)
	UINT32           height;       ///< image height        (pixel)
	UINT32           crop_width;   ///< image crop width         (pixel)
	UINT32           crop_height;  ///< image crop height        (pixel)
	UINT32           crop_hpos;   ///< image crop horizontal start     (pixel)
	UINT32           crop_vpos;  ///< image crop virtical start       (pixel)
	//UINT32           stripe_hm;
	//UINT32           stripe_hn;
	//UINT32           stripe_hl;
	//UINT8            h_shift;
	KDRV_IPP_RAW_BIT bit;          ///< bits /pixel
	//UINT8            cfa_pat;
	KDRV_IPP_PIX     st_pix;       ///< image start pixel

} KDRV_IFE_RAW_INFO;

//structure data type
/**
    IFE structure - ife kdrv open object
*/
typedef struct _KDRV_IFE_OPENOBJ {
	UINT32 ife_clock_sel;       ///< Engine clock selection
} KDRV_IFE_OPENCFG, *pKDRV_IFE_OPENCFG;

/**
    IFE KDRV Input Image Address
*/
typedef struct _KDRV_IFE_IN_ADDR {
	UINT32      ife_in_addr_1;        ///< input image address
	UINT32      ife_in_addr_2;        ///< input image address
} KDRV_IFE_IN_ADDR;

/**
    IFE KDRV Input Image Line Offset
*/
typedef struct _KDRV_IFE_IN_OFS {
	UINT32           line_ofs_1;   ///< image line offset 1 (byte)
	UINT32           line_ofs_2;   ///< image line offset 2 (byte)
} KDRV_IFE_IN_OFS;

/**
    IFE KDRV Input Image Information
*/
typedef struct _KDRV_IFE_IN_IMG_INFO {
	KDRV_IFE_IN_MODE     in_src;     ///< input image source
	KDRV_IFE_IN_FMT      type;       ///< image type
	KDRV_IFE_RAW_INFO    img_info;   ///< image information
} KDRV_IFE_IN_IMG_INFO;

/**
    IFE KDRV Output Image Address
*/
typedef struct _KDRV_IFE_OUT_ADDR {
	UINT32      ife_out_addr;       ///< output image address
} KDRV_IFE_OUT_ADDR;

/**
    IFE KDRV Output Image Information
*/
typedef struct _KDRV_IFE_OUT_IMG_INFO {
	UINT32           line_ofs;   ///< image out offset
	KDRV_IPP_RAW_BIT bit;        ///< bits /pixel
} KDRV_IFE_OUT_IMG_INFO;

/**
    IFE ring buf info
*/
typedef struct _KDRV_IFE_RING_BUF_INFO {
	BOOL                    dmaloop_en;             ///< Enable/Disable
	UINT32                  dmaloop_line;           ///< Input image
} KDRV_IFE_RING_BUF_INFO;

/**
    IFE wait sie2 set.
*/
typedef struct _KDRV_IFE_WAIT_SIE2_DISABLE_INFO {
	BOOL    dma_wait_sie2_start_disable;                      ///<  Enable/Disable
} KDRV_IFE_WAIT_SIE2_DISABLE_INFO;


/**
    IFE rde Q table index
*/
/*
typedef struct _KDRV_IFE_Q_TAB {
    UINT8           idx[RDE_Q_TAB_NUM];           ///<  RDE the index for DCT decode Q table
} KDRV_IFE_Q_TAB;
*/
/**
    IFE rde control func
*/
/*
typedef struct _KDRV_IFE_DEGAMMA {
    BOOL            enable;              ///<  RDE Degamma enable
    UINT16          table[RDE_DEGAMMA_NUM];           ///<  RDE degamma table
} KDRV_IFE_DEGAMMA;
*/
/**
    IFE rde rand init func
*/
/*
typedef struct _KDRV_IFE_RDE_DITHER {
    BOOL            enable;                  ///<  RDE Dithering enable
    BOOL            dither_reset;            ///<  RDE Dithering reset initial random seed
    UINT8           Rand1Init1;              ///<  RDE dithering random sequence init setting for rde1
    UINT16          Rand1Init2;              ///<  RDE dithering random sequence init setting for rde1
    UINT8           Rand2Init1;              ///<  RDE dithering random sequence init setting for rde2
    UINT16          Rand2Init2;              ///<  RDE dithering random sequence init setting for rde2
} KDRV_IFE_RDE_DITHER;
*/


/**
    IFE KDRV - rde encode rate
*/
typedef enum _KDRV_IFE_ENCODE_RATE {
	KDRV_IFE_ENCODE_RATE_50     = 0,     ///< RDE encode rate 50
	KDRV_IFE_ENCODE_RATE_58     = 1,     ///< RDE encode rate 58
	KDRV_IFE_ENCODE_RATE_66     = 2,     ///< RDE encode rate 66
	ENUM_DUMMY4WORD(KDRV_IFE_ENCODE_RATE)
} KDRV_IFE_ENCODE_RATE;

/**
    IFE rde func
*/
typedef struct _KDRV_IFE_RDE_INFO {
	BOOL                  enable;        ///<  Enable
	KDRV_IFE_ENCODE_RATE  encode_rate;   ///<  RDE encode rate of bitstream per segment
} KDRV_IFE_RDE_INFO;

/************************************************************************************************
**                                  IFE KDRV IQ parameter                                       **
************************************************************************************************/
/**
    range of color gain coefficients
*/
typedef enum _KDRV_IFE_GAIN_FIELD {
	KDRV_IFE__2_8    = 0,    ///< 2bits decimal, 8bits fraction
	KDRV_IFE__3_7    = 1,    ///< 3bits decimal, 7bits fraction
	ENUM_DUMMY4WORD(KDRV_IFE_GAIN_FIELD)
} KDRV_IFE_GAIN_FIELD;



/**
    IFE nrs Order func
*/
typedef struct _KDRV_IFE_NRS_ORDER_PARAM {
	BOOL            enable;                     ///<                NRS Order enable
	UINT8           rng_bri;                    ///<  Range 0~7   . NRS Order bright range.
	UINT8           rng_dark;                   ///<  Range 0~7   . NRS Order dark range.
	UINT16          diff_th;                    ///<  Range 0~1023. NRS Order difference threshold.
	UINT8           bri_w[NRS_ORD_W_NUM];       ///<  Range 0~8   . NRS Order bright weight.
	UINT8           dark_w[NRS_ORD_W_NUM];      ///<  Range 0~8   . NRS Order dark weight.
} KDRV_IFE_ORDER_PARAM;

/**
    IFE nrs Bilat func
*/
typedef struct _KDRV_IFE_NRS_BILAT_PARAM {
	BOOL            b_enable;                   ///<                NRS Bilateral  enable
	BOOL            gb_enable;                  ///<                NRS G channel Bilateral enable
	UINT8           b_str;                      ///<  Range 0~8   . NRS Bilateral strength.
	UINT8           gb_str;                     ///<  Range 0~8   . NRS G channel Bilateral strength.

	UINT8           gb_blend_w;                 ///<  Range 0~15  . NRS G weight with B.

	UINT8           b_ofs[NRS_B_OFS_NUM];       ///<  Range 0~255 . NRS Bilateral offset.
	UINT8           b_weight[NRS_B_W_NUM];      ///<  Range 0~31  . NRS Bilateral weight.
	UINT8           b_th[NRS_B_TH_NUM];         ///<  Range 0~3   . NRS Bilateral threshold.
	UINT16          b_rng[NRS_B_RANGE_NUM];     ///<  Range 0~4095. NRS Bilateral range.

	UINT8           gb_ofs[NRS_GB_OFS_NUM];     ///<  Range 0~255 . NRS G channel Bilateral offset.
	UINT8           gb_weight[NRS_GB_W_NUM];    ///<  Range 0~31  . NRS G channel Bilateral weight.
	UINT8           gb_th[NRS_GB_TH_NUM];       ///<  Range 0~3   . NRS G channel Bilateral threshold.
	UINT16          gb_rng[NRS_GB_RANGE_NUM];   ///<  Range 0~4095. NRS G channel Bilateral range.
} KDRV_IFE_BILAT_PARAM;


/**
    IFE nrs func
*/
typedef struct _KDRV_IFE_NRS_PARAM {
	BOOL                   enable;///<  Enable/Disable
	KDRV_IFE_ORDER_PARAM   order;
	KDRV_IFE_BILAT_PARAM   bilat;
} KDRV_IFE_NRS_PARAM;

typedef enum {
	KDRV_IFE_F_CURVE_Y_8G4R4B = 0,
	KDRV_IFE_F_CURVE_Y_4G = 1,
	KDRV_IFE_F_CURVE_Y_ORI = 2,
	ENUM_DUMMY4WORD(KDRV_IFE_F_CURVE_Y_SEL)
} KDRV_IFE_F_CURVE_Y_SEL;


typedef struct _KDRV_IFE_FCURVE_CTRL {
	KDRV_IFE_F_CURVE_Y_SEL  y_mean_sel;///< Rnage 0~2. Fcurve Y Mean Method selection.
	UINT8     yv_w;                    ///< Range 0~8. Fcurve Y mean and V weight.
} KDRV_IFE_FCURVE_CTRL;

typedef struct _KDRV_IFE_Y_W {
	UINT8            y_w_lut[FCURVE_Y_W_NUM];     ///<  Range 0~255 . Fcurve Y weight.
} KDRV_IFE_Y_W;

typedef struct _KDRV_IFE_IDX_LUT {
	UINT8            idx_lut[FCURVE_IDX_NUM];     ///<  Range 0~63  . Fcurve Index.
} KDRV_IFE_IDX_LUT;

typedef struct _KDRV_IFE_SPLIT_LUT {
	UINT8            split_lut[FCURVE_SPLIT_NUM]; ///<  Range 0~3   . Fcurve Split.
} KDRV_IFE_SPLIT_LUT;

typedef struct _KDRV_IFE_VAL_LUT {
	UINT16           val_lut[FCURVE_VAL_NUM];     ///<  Range 0~4095. Fcurve Value.
} KDRV_IFE_VAL_LUT;


/**
    IFE fcurve func
*/
typedef struct _KDRV_IFE_FCURVE_PARAM {
	BOOL                 enable;    ///<  Enable/Disable of f curve in IFE.
	KDRV_IFE_FCURVE_CTRL fcur_ctrl; ///<  Fcurve control Y Mean Method and V weight.
	KDRV_IFE_Y_W         y_weight;  ///<  Fcurve Y weight
	KDRV_IFE_IDX_LUT     index;     ///<  Fcurve Index
	KDRV_IFE_SPLIT_LUT   split;     ///<  Fcurve Split
	KDRV_IFE_VAL_LUT     value;     ///<  Fcurve Value
} KDRV_IFE_FCURVE_PARAM;

typedef enum {
	KDRV_IFE_ONE_IMAGE = 0,
	KDRV_IFE_TWO_IMAGE = 1,
	ENUM_DUMMY4WORD(KDRV_IFE_FUSION_IMAGE_NUM)
} KDRV_IFE_FUSION_IMAGE_NUM;

/**
    IFE fusion info
*/
typedef struct _KDRV_IFE_FUSION_INFO {
	BOOL                         enable;///< Enable/Disable of fusion.
	KDRV_IFE_FUSION_IMAGE_NUM    fnum;///< Input image number. When fusion be enabled(HDR mode), fnum must be 1.  Range 0~1.
} KDRV_IFE_FUSION_INFO;

typedef enum {
	KDRV_IFE_Y_SEL_8G4R4B = 0,
	KDRV_IFE_Y_SEL_4G = 1,
	KDRV_IFE_Y_SEL_RGB_MAX = 2,
	KDRV_IFE_Y_SEL_ORI = 3,
	ENUM_DUMMY4WORD(KDRV_IFE_Y_SEL)
} KDRV_IFE_Y_SEL;

typedef enum {
	KDRV_IFE_MODE_FUSION = 0,
	KDRV_IFE_MODE_SHORT_EXP = 1,
	KDRV_IFE_MODE_LONG_EXP = 2,
	KDRV_IFE_MODE_LONG_REAL = 3,
	ENUM_DUMMY4WORD(KDRV_IFE_MODE_SEL)
} KDRV_IFE_MODE_SEL;

/**
    IFE fusion ctrl func
*/
typedef struct _KDRV_IFE_FUSION_CTRL {
	KDRV_IFE_Y_SEL    y_mean_sel; ///< range 0~3  . Y mean option.
	KDRV_IFE_MODE_SEL mode;       ///< range 0~3  . Fusion mode.  0: Fusion; 1: Short frame; 2: Long frame; 3: Long real;
	UINT8             ev_ratio;   ///< range 0~255. 4EV=16, 3EV = 32, 2EV=64, 1EV=128;
} KDRV_IFE_FUSION_CTRL;


typedef enum {
	KDRV_IFE_FUSION_BY_L_EXP = 0,
	KDRV_IFE_FUSION_BY_S_EXP = 1,
	KDRV_IFE_FUSION_BOTH_EXP = 2,
	ENUM_DUMMY4WORD(KDRV_IFE_FUSION_NOR_SEL)
} KDRV_IFE_FUSION_NOR_SEL;


/**
    IFE fusion blend curve
*/
typedef struct _KDRV_IFE_BLEND_CURVE {
	KDRV_IFE_FUSION_NOR_SEL nor_sel; ///<  Range 0~2. Normal blending curve weighting option.
	KDRV_IFE_FUSION_NOR_SEL dif_sel; ///<  Range 0~2. Normal blending curve weighting option.
	UINT16           l_nor_knee[FUSION_BLD_CUR_KNEE_NUM];///<range 0~4095.  Knee point of normal blending curve for long exposure.
	UINT16           l_nor_range;  ///< range 0~12. Normal blending curve range for long  exposure. Actually range = (1<<l_nor_range)
	BOOL             l_nor_w_edge; ///< range 0~1. Weighting function direction.
	UINT16           s_nor_knee[FUSION_BLD_CUR_KNEE_NUM];///<range 0~4095.  Knee point of normal blending curve for short exposure.
	UINT16           s_nor_range;///< range 0~12. Normal blending curve range for short  exposure.
	BOOL             s_nor_w_edge;///< range 0~1. Weighting function direction.

	UINT16           l_dif_knee[FUSION_BLD_CUR_KNEE_NUM];///<range 0~4095.  Knee point of difference blending curve for long exposure.
	UINT16           l_dif_range;///< range 0~12. Difference blending curve range for long  exposure. Actually range = (1<<l_dif_range)
	BOOL             l_dif_w_edge;///< range 0~1. Weighting function direction.


	UINT16           s_dif_knee[FUSION_BLD_CUR_KNEE_NUM];///<range 0~4095.  Knee point of difference blending curve for long exposure.
	UINT16           s_dif_range;///< range 0~12. Difference blending curve range for short  exposure. Actually range = (1<<l_dif_range)
	BOOL             s_dif_w_edge;///< range 0~1. Weighting function direction.

} KDRV_IFE_BLEND_CURVE;


/**
    IFE fusion mc
*/
typedef struct _KDRV_IFE_MC {
	UINT16           lum_th;    ///< Range 0~4095. Lower than this threshold using normal blending curve.
	UINT8            diff_ratio;///< Range 0~3   . Control ratio.
	UINT8            pos_diff_w[FUSION_MC_DIFF_W_NUM];///< Range 0~16. Difference weighting for difference > 0.
	UINT8            neg_diff_w[FUSION_MC_DIFF_W_NUM];///< Range 0~16. Difference weighting for difference <= 0.
	UINT8            dwd;///< Range 0~16. Using this ratio if difference less than lum_th.
} KDRV_IFE_MC;

/**
    IFE fusion dark sat reduce
*/
typedef struct _KDRV_IFE_DARK_SAT {
	UINT16           th[FUSION_DARK_SAT_NUM];       ///< Range 0~4095. Threshold of DS.
	UINT8            step[FUSION_DARK_SAT_NUM];     ///< Range 0~255 . Weighting step of DS.
	UINT8            low_bound[FUSION_DARK_SAT_NUM];///< range 0~255 . Lower bound of DS.
} KDRV_IFE_DARK_SAT;

/**
    IFE fusion s compression
*/
typedef struct _KDRV_IFE_S_COMP {
	BOOL             enable;                              ///< Range 0~1   . S Compression switch.
	UINT16           knee[FUSION_SHORT_COMP_KNEE_NUM];    ///< Rnage 0~4095. Knee point of S-compression.
	UINT16           sub_point[FUSION_SHORT_COMP_SUB_NUM];///< Range 0~4095. Start point of S-compression.
	UINT8            shift[FUSION_SHORT_COMP_SHIFT_NUM];  ///< Range 0~7   . Shift bit of   S-compression.
} KDRV_IFE_S_COMP;

/**
    IFE fusion color gain
*/
typedef struct _KDRV_IFE_FCGAIN {
	BOOL                 enable;                ///< Enable/Disable.
	KDRV_IFE_GAIN_FIELD  bit_field;             ///< Range 0~1. select Fusion color gain decimal range.

	UINT32               fcgain_path0_r;        ///< Range 0~1023. SHDR short exposure path R channel color gain.
	UINT32               fcgain_path0_gr;       ///< Range 0~1023. SHDR short exposure path Gr channel color gain.
	UINT32               fcgain_path0_gb;       ///< Range 0~1023. SHDR short exposure path Gb channel color gain.
	UINT32               fcgain_path0_b;        ///< Range 0~1023. SHDR short exposure path B channel color gain.
	UINT32               fcgain_path0_ir;       ///< Range 0~1023. SHDR short exposure path IR channel color gain.

	UINT32               fcofs_path0_r;         ///< Range 0~1023. SHDR short exposure path R channel color offset.
	UINT32               fcofs_path0_gr;        ///< Range 0~1023. SHDR short exposure path Gr channel color offset.
	UINT32               fcofs_path0_gb;        ///< Range 0~1023. SHDR short exposure path Gb channel color offset.
	UINT32               fcofs_path0_b;         ///< Range 0~1023. SHDR short exposure path B channel color offset.
	UINT32               fcofs_path0_ir;        ///< Range 0~1023. SHDR short exposure path IR channel color offset.

	UINT32               fcgain_path1_r;        ///< Range 0~1023. SHDR long exposure path R channel color gain.
	UINT32               fcgain_path1_gr;       ///< Range 0~1023. SHDR long exposure path Gr channel color gain.
	UINT32               fcgain_path1_gb;       ///< Range 0~1023. SHDR long exposure path Gb channel color gain.
	UINT32               fcgain_path1_b;        ///< Range 0~1023. SHDR long exposure path B channel color gain.
	UINT32               fcgain_path1_ir;       ///< Range 0~1023. SHDR long exposure path IR channel color gain.

	UINT32               fcofs_path1_r;         ///< Range 0~1023. SHDR long exposure path R channel color offset.
	UINT32               fcofs_path1_gr;        ///< Range 0~1023. SHDR long exposure path Gr channel color offset.
	UINT32               fcofs_path1_gb;        ///< Range 0~1023. SHDR long exposure path Gb channel color offset.
	UINT32               fcofs_path1_b;         ///< Range 0~1023. SHDR long exposure path B channel color offset.
	UINT32               fcofs_path1_ir;        ///< Range 0~1023. SHDR long exposure path IR channel color offset.

} KDRV_IFE_FCGAIN;

/**
    IFE fusion func
*/
typedef struct _KDRV_IFE_FUSION_PARAM {
	KDRV_IFE_FUSION_CTRL   fu_ctrl;
	KDRV_IFE_BLEND_CURVE   bld_cur;
	KDRV_IFE_MC            mc_para;
	KDRV_IFE_DARK_SAT      dk_sat;
	KDRV_IFE_S_COMP        s_comp;
	KDRV_IFE_FCGAIN        fu_cgain;
} KDRV_IFE_FUSION_PARAM;



/**
    IFE outlier func
*/
typedef struct _KDRV_IFE_OUTL_PARAM {
	BOOL    enable;                      ///< Enable/Disable
	UINT32  bright_th[OUTL_BRI_TH_NUM];  ///< UINT32[5]
	///< 0: avg<32 , (bit range: 8-bit)
	///< 1: avg<64
	///< 2: avg<128
	///< 3: max:255
	///< 4: for interpolation
	UINT32  dark_th [OUTL_DARK_TH_NUM];  ///< range: 0~1   , UINT32[5], same as above
	UINT32  outl_cnt[OUTL_CNT_NUM];      ///< range: 0~16  , UINT32[2], outlier neighbor point counter
	UINT32  outl_weight;                 ///< range: 0~255 , outlier weighting
	UINT32  dark_ofs;                    ///< range: 0~4095, outlier dark th offset
	UINT32  bright_ofs;                  ///< range: 0~4095, outlier bright th offset

	BOOL    avg_mode;                    ///< range: 0~1   , outlier use direction average or not

	UINT8   ord_rng_bri;                 ///< range: 0~7   , control the strength of order method for bright defect pixel.
	UINT8   ord_rng_dark;                ///< range: 0~7   , control the strength of order method for dark defect pixel.
	UINT16  ord_protect_th;              ///< range: 0~1023, order method protect artifact th
#if NAME_ERR
	UINT8   ord_blend_w;                 ///< range: 0~255 , outlier and order result blending weight
#else
	UINT8   ord_bleand_w;                ///< range: 0~255 , outlier and order result blending weight
#endif
	UINT8   ord_bri_w[OUTL_ORD_W_NUM];   ///< range: 0~8   , order bright defect pixel compensate weight
	UINT8   ord_dark_w[OUTL_ORD_W_NUM];  ///< range: 0~8   , order dark defect pixel compensate weight
} KDRV_IFE_OUTL_PARAM;

//BOOL            OutlCrsChanEn;  ///< remove in NT 98520
//IFE_OUTL_SEL    OutlOp;         ///< outlier option for neighbour condition
/**
    range filter
*/
typedef struct _KDRV_IFE_CENTER_MODIFY {
	BOOL    enable;     ///<               3x3 center mofify bilateral filter threshold enable
	UINT32  th1;        ///< range:0~1023, 3x3 center mofify bilateral filter threshold 1
	UINT32  th2;        ///< range:0~1023, 3x3 center mofify bilateral filter threshold 2
} KDRV_IFE_CENTER_MODIFY;

typedef struct _KDRV_IFE_RANGE_PARAM_R {
	UINT32  a_th [RANGE_A_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[RANGE_A_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [RANGE_B_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[RANGE_B_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER_R;

typedef struct _KDRV_IFE_RANGE_PARAM_GR {
	UINT32  a_th [RANGE_A_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[RANGE_A_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [RANGE_B_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[RANGE_B_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER_GR;

typedef struct _KDRV_IFE_RANGE_PARAM_GB {
	UINT32  a_th [RANGE_A_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[RANGE_A_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [RANGE_B_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[RANGE_B_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER_GB;

typedef struct _KDRV_IFE_RANGE_PARAM_B {
	UINT32  a_th [RANGE_A_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[RANGE_A_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [RANGE_B_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[RANGE_B_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER_B;

typedef struct _KDRV_IFE_RANGE_PARAM_IR {
	UINT32  a_th [RANGE_A_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[RANGE_A_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [RANGE_B_TH_NUM];     ///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[RANGE_B_LUT_SIZE];   ///< range:0~1023, UINT32[17], range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER_IR;

typedef struct _KDRV_IFE_SPATIAL {
	UINT32  weight[SPATIAL_W_LEN];     ///< range:0~31, UINT32[6], filter spatial weight,
} KDRV_IFE_SPATIAL;


/**
    clamp and weighting setting
*/
typedef struct _KDRV_IFE_CLAMP {
	UINT32  th;         ///< range:0~4095, threshold
	UINT32  mul;        ///< range:0~255 , weighting multiplier
	UINT32  dlt;        ///< range:0~4095, threshold adjustment
} KDRV_IFE_CLAMP;

/**
    IFE structure - IFE RB fill parameter set.
*/
typedef struct _KDRV_IFE_RBFILL_PARAM {
	BOOL    enable;                  ///< range: 0~1 , Enable/Disable
	UINT8   luma [RBFILL_LUMA_NUM];  ///< range: 0~31,
	UINT8   ratio[RBFILL_RATIO_NUM]; ///< range: 0~31,
	UINT8   ratio_mode;              ///< range: 0~2 , n3.2, n2.3, n1.4
} KDRV_IFE_RBFILL_PARAM;

/**
    IFE bilateral filter
*/
typedef struct _KDRV_IFE_FILTER_PARAM {
	BOOL                        enable;         ///< Enable/Disable
	KDRV_IFE_SPATIAL            spatial;        ///< filter spatial weight table
	KDRV_IFE_RANGE_FILTER_R     rng_filt_r;     ///< range filter ch0
	KDRV_IFE_RANGE_FILTER_GR    rng_filt_gr;    ///< range filter ch1
	KDRV_IFE_RANGE_FILTER_GB    rng_filt_gb;    ///< range filter ch1
	KDRV_IFE_RANGE_FILTER_B     rng_filt_b;     ///< range filter ch2
	KDRV_IFE_RANGE_FILTER_IR    rng_filt_ir;    ///< range filter ch3

	KDRV_IFE_CENTER_MODIFY      center_mod;     ///< center modify
	KDRV_IFE_CLAMP              clamp;          ///< clamp and weighting setting
	KDRV_IFE_RBFILL_PARAM       rbfill;         ///< RGBIr fill r/b pixel
	UINT32                      blend_w;        ///< range:0~255, range filter A and B weighting
	UINT32                      rng_th_w;       ///< range:0~15 ,range th weight
	UINT32                      bin;            ///< range:0~7  , ( Denominator: 2<<bin )
} KDRV_IFE_FILTER_PARAM;


typedef enum _KDRV_IFE_VIG_GAIN {
	VIG_TAB_GAIN_1X    = 0, //KDRV_IFE_VIG_GAIN_0 ,
	VIG_TAB_GAIN_2X    = 1, //KDRV_IFE_VIG_GAIN_1 ,
	VIG_TAB_GAIN_4X    = 2, //KDRV_IFE_VIG_GAIN_2 ,
	VIG_TAB_GAIN_8X    = 3, //KDRV_IFE_VIG_GAIN_3 ,
	ENUM_DUMMY4WORD(KDRV_IFE_VIG_GAIN)
} KDRV_IFE_VIG_GAIN;

/**
    IFE color gain
*/
typedef struct _KDRV_IFE_CGAIN_PARAM {
	BOOL                 enable;        ///< range:0~1   , Enable/Disable
	BOOL                 inv;           ///< range:0~1   , color gain invert
	BOOL                 hinv;          ///< range:0~1   , color gain H-invert
	KDRV_IFE_GAIN_FIELD  bit_field;     ///< range:0~1   , color gain bit field
	UINT32               mask;          ///< range:0~4095, color gain mask

	UINT32               cgain_r;       ///< range 0~1023, R  channel color gain.
	UINT32               cgain_gr;      ///< range 0~1023, Gr channel color gain.
	UINT32               cgain_gb;      ///< range 0~1023, Gb channel color gain.
	UINT32               cgain_b;       ///< range 0~1023, B  channel color gain.
	UINT32               cgain_ir;      ///< range 0~1023, IR channel color gain.

	UINT32               cofs_r;        ///< range 0~1023, R  channel color offset.
	UINT32               cofs_gr;       ///< range 0~1023, Gr channel color offset.
	UINT32               cofs_gb;       ///< range 0~1023, Gb channel color offset.
	UINT32               cofs_b;        ///< range 0~1023, B  channel color offset.
	UINT32               cofs_ir;       ///< range 0~1023, IR channel color offset.
} KDRV_IFE_CGAIN_PARAM;

/**
    Image coordinates with negative range.
*/
typedef struct _KDRV_IFE_COORDINATE {
	INT32  x;              ///< x point
	INT32  y;              ///< y point
} KDRV_IFE_COORDINATE;

/**
    Vig center coordinate
*/
typedef struct _KDRV_IFE_VIG_CENTER {
	KDRV_IFE_COORDINATE  ch0;     ///< range: -8192~8191, channel 0 ,VIG center of R  / R
	KDRV_IFE_COORDINATE  ch1;     ///< range: -8192~8191, channel 1 ,VIG center of Gr / G
	KDRV_IFE_COORDINATE  ch2;     ///< range: -8192~8191, channel 2 ,VIG center of Gb / Ir
	KDRV_IFE_COORDINATE  ch3;     ///< range: -8192~8191, channel 3 ,VIG center of B  / B
} KDRV_IFE_VIG_CENTER;

/**
    ife vignette shading compensation
*/
typedef struct _KDRV_IFE_VIG_PARAM {
	BOOL    enable;                       ///< range: 0~1   , Enable/Disable
	UINT32  dist_th;                      ///< range: 0~1023, disabled area distance threshold

	UINT32  ch_r_lut[VIG_CH0_LUT_SIZE];   ///< range: 0~1023, VIG LUT of R  channel
	UINT32  ch_gr_lut[VIG_CH1_LUT_SIZE];  ///< range: 0~1023, VIG LUT of Gr channel
	UINT32  ch_gb_lut[VIG_CH1_LUT_SIZE];  ///< range: 0~1023, VIG LUT of Gb channel
	UINT32  ch_b_lut[VIG_CH3_LUT_SIZE];   ///< range: 0~1023, VIG LUT of B  channel
	UINT32  ch_ir_lut[VIG_CH2_LUT_SIZE];  ///< range: 0~1023, VIG LUT of Ir channel

	BOOL    dither_enable;                ///< range: 0~1   ,dithering enable/disable
	BOOL    dither_rst_enable;            ///< range: 0~1   ,dithering reset enable

	BOOL    vig_fisheye_gain_en;          ///< range: 0~1   , fisheye gain enable/disable
	UINT8   vig_fisheye_slope;            ///< range: 0~15  , fisheye gain decrease slope out of radius
	UINT32  vig_fisheye_radius;           ///< range: 0~1023, fisheye gain radius
} KDRV_IFE_VIG_PARAM;

/**
    IFE structure - IFE gbalance parameter set.
*/
typedef struct _KDRV_IFE_GBAL_PARAM {
	BOOL     enable;                ///< range: 0~1   , enable/disable
	BOOL     protect_enable;        ///< range: 0~1   , enable/disable preserve edge or feature
	UINT16   diff_th_str;           ///< range: 0~1023, gbal strength
	UINT8    diff_w_max;            ///< range: 0~15  , difference weight max
	UINT16   edge_protect_th1;      ///< range: 0~1023, edge judgement th 1
	UINT16   edge_protect_th0;      ///< range: 0~1023, edge judgement th 0
	UINT8    edge_w_max;            ///< range: 0~255 , edge weight max
	UINT8    edge_w_min;            ///< range: 0~255 , edge weight min
	UINT8    gbal_ofs[GBAL_OFS_NUM];///< range: 0~63  , weight transision region
} KDRV_IFE_GBAL_PARAM;

/**
    IFE structure - mirror set.
*/
typedef struct _KDRV_IFE_MIRROR {
	BOOL    enable;                      ///<  Enable/Disable
} KDRV_IFE_MIRROR;


#if 0
#endif

typedef struct _KDRV_IFE_PARAM_IQ_ALL_PARAM {
	KDRV_IFE_NRS_PARAM     *p_nrs;
	KDRV_IFE_FCURVE_PARAM  *p_fcurve;
	KDRV_IFE_FUSION_PARAM  *p_fusion;

	KDRV_IFE_OUTL_PARAM    *p_outl;
	KDRV_IFE_FILTER_PARAM  *p_filt;
	KDRV_IFE_CGAIN_PARAM   *p_cgain;
	KDRV_IFE_VIG_PARAM     *p_vig;
	KDRV_IFE_GBAL_PARAM    *p_gbal;
} KDRV_IFE_PARAM_IQ_ALL_PARAM;

typedef struct _KDRV_IFE_PARAM_IPL_FUNC_EN {
	UINT32 ipl_ctrl_func;   ///<It labels which function will be controlled by KFLOW
	BOOL   enable;          ///
} KDRV_IFE_PARAM_IPL_FUNC_EN;

typedef enum _KDRV_IFE_IQ_FUNC {
	KDRV_IFE_IQ_FUNC_OUTL       = 0x00000001,
	KDRV_IFE_IQ_FUNC_FILTER     = 0x00000002,
	KDRV_IFE_IQ_FUNC_CGAIN      = 0x00000004,
	KDRV_IFE_IQ_FUNC_VIG_PARAM  = 0x00000008,
	KDRV_IFE_IQ_FUNC_GBAL       = 0x00000010,
	KDRV_IFE_IQ_FUNC_NRS        = 0x00000020,
	KDRV_IFE_IQ_FUNC_FCURVE     = 0x00000040,
	KDRV_IFE_IQ_FUNC_ALL        = 0x0000007F,
	ENUM_DUMMY4WORD(KDRV_IFE_IQ_FUNC),
} KDRV_IFE_IQ_FUNC;

/**
    KDRV IFE ITEM, for kdrv_ife_set/kdrv_ife_get
*/
typedef enum _KDRV_IFE_PARAM_ID {
	//KDRV_IFE_PARAM_IPL_TRIG = 0,   //<SET    >  ife trigger                       data_type: NONE
	KDRV_IFE_PARAM_IPL_OPENCFG,      //<SET    >  ife clock                         data_type: KDRV_IFE_OPENCFG
	KDRV_IFE_PARAM_IPL_IN_IMG,       //<SET/GET>  ife input image info              data_type: KDRV_IFE_IN_IMG_INFO
	KDRV_IFE_PARAM_IPL_IN_ADDR_1,    //<SET/GET>  ife input address                 data_type: KDRV_IFE_IN_ADDR
	KDRV_IFE_PARAM_IPL_IN_ADDR_2,    //<SET/GET>  ife input address                 data_type: KDRV_IFE_IN_ADDR
	KDRV_IFE_PARAM_IPL_IN_OFS_1,     //<SET/GET>  ife input address                 data_type: KDRV_IFE_IN_OFS
	KDRV_IFE_PARAM_IPL_IN_OFS_2,     //<SET/GET>  ife input address                 data_type: KDRV_IFE_IN_OFS
	KDRV_IFE_PARAM_IPL_OUT_IMG,      //<SET/GET>  ife output path image info        data_type: KDRV_IFE_OUT_IMG_INFO
	KDRV_IFE_PARAM_IPL_OUT_ADDR,     //<SET/GET>  ife output address                data_type: KDRV_IFE_OUT_ADDR
	KDRV_IFE_PARAM_IPL_ISR_CB,       //<SET    >  ife external isr cb               data_type: KDRV_IFE_ISRCB
	KDRV_IFE_PARAM_IPL_INTER,        //<SET/GET>  ife interruput enable,            data_type: KDRV_IFE_INTE
	KDRV_IFE_PARAM_IPL_MIRROR,       //<SET/GET>  ife mirror enable,                data_type: KDRV_IFE_MIRROR
	KDRV_IFE_PARAM_IPL_RDE_INFO,     //<SET/GET>  ife rde en and rate,              data_type: KDRV_IFE_RDE_INFO
	KDRV_IFE_PARAM_IPL_FUSION_INFO,  //<SET/GET>  ife fusion en and frame num,      data_type: KDRV_IFE_FUSION_INFO
	KDRV_IFE_PARAM_IPL_RINGBUF_INFO, //<SET/GET>  ife dmaloop_line and dmaloop_en,  data_type: KDRV_IFE_RING_BUF_INFO
	KDRV_IFE_PARAM_IPL_WAIT_SIE2_DISABLE,//<SET   > ife hdr wait sie2 disable,        data_type: KDRV_IFE_WAIT_SIE2_DISABLE
	KDRV_IFE_PARAM_IPL_LOAD,         //<SET    >  ife ipl set load,

	KDRV_IFE_PARAM_IQ_NRS,           //<SET/GET>  ife IQ parameter (nrs)        data_type: KDRV_IFE_NRS_PARAM
	KDRV_IFE_PARAM_IQ_FUSION,        //<SET/GET>  ife IQ parameter (fusion)     data_type: KDRV_IFE_FUSION_PARAM
	KDRV_IFE_PARAM_IQ_FCURVE,        //<SET/GET>  ife IQ parameter (fcurve)     data_type: KDRV_IFE_FCURVE_PARAM

	KDRV_IFE_PARAM_IQ_OUTL,          //<SET/GET>  ife IQ parameter (outlier)    data_type: KDRV_IFE_OUTL_PARAM
	KDRV_IFE_PARAM_IQ_FILTER,        //<SET/GET>  ife IQ parameter (filter)     data_type: KDRV_IFE_FILTER_PARAM
	KDRV_IFE_PARAM_IQ_CGAIN,         //<SET/GET>  ife IQ parameter (color gain) data_type: KDRV_IFE_CGAIN_PARAM
	KDRV_IFE_PARAM_IQ_VIG,           //<SET/GET>  ife IQ parameter (vignette)   data_type: KDRV_IFE_VIG_PARAM
	KDRV_IFE_PARAM_IPL_VIG_CENTER,   //<SET/GET>  ife IPL info     (vignette)   data_type: KDRV_IFE_VIG_CENTER
	KDRV_IFE_PARAM_IQ_GBAL,          //<SET/GET>  ife IQ parameter (G balance)  data_type: KDRV_IFE_GBAL_PARAM
	KDRV_IFE_PARAM_IQ_ALL,           //<SET/   >  ife all IQ param set          data_type: KDRV_IFE_PARAM_IQ_ALL_PARAM
	KDRV_IFE_PARAM_IPL_ALL_FUNC_EN,  //<SET/   >  ife func enable/disable       data_type: KDRV_IFE_PARAM_IPL_FUNC_EN
	KDRV_IFE_PARAM_IPL_RESET,        //<SET/   >  ife channel reset             data_type: KDRV_IFE_PARAM_IPL_RESET
	KDRV_IFE_PARAM_IPL_BUILTIN_DISABLE,    //<SET/   >  ife disable builtin flow control     data_type: NULL    
	KDRV_IFE_PARAM_MAX,
	KDRV_IFE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IFE_PARAM_ID)
} KDRV_IFE_PARAM_ID;

typedef struct _KDRV_IFE_TRIGGER_PARAM {
	UINT32 RESERVED;
} KDRV_IFE_TRIGGER_PARAM;

STATIC_ASSERT((KDRV_IFE_PARAM_MAX &KDRV_IFE_PARAM_REV) == 0);
#define KDRV_IFE_IGN_CHK KDRV_IFE_PARAM_REV //only support set/get function

#endif //_KDRV_IFE_H_
