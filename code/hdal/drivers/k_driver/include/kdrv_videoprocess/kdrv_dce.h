/**
    Public header file for kdrv_dce

    This file is the header file that define the API and data type for kdrv_dce.

    @file       kdrv_dce.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.    All rights reserved.
*/
#ifndef _KDRV_DCE_H_
#define _KDRV_DCE_H_

//#include "dce_lib.h"

#define IMG_MAX_STP_NUM         16
#define DCE_PARTITION_NUM       16
#define GDC_TABLE_NUM           65
#define CAC_TABLE_NUM           65
#define KDRV_CFA_FREQ_NUM       16
#define KDRV_CFA_LUMA_NUM       17
#define KDRV_CFA_FCS_NUM        16
#define LUT2D_MAX_TABLE_SIZE    (65*65)
#define WDR_SUBIMG_FILT_NUM     3
#define WDR_COEF_NUM            4
#define WDR_INPUT_BLD_NUM       17
#define WDR_NEQ_TABLE_IDX_NUM   32
#define WDR_NEQ_TABLE_VAL_NUM   65
#define HISTOGRAM_STCS_NUM      128

/**
    DCE KDRV interrupt enable enum.
*/
typedef enum _KDRV_DCE_INTE {
	KDRV_DCE_INTE_CLR       = 0x00000000,
	KDRV_DCE_INTE_FST       = 0x00000001,   // enable interrupt: frame start
	KDRV_DCE_INTE_FMD       = 0x00000002,   // enable interrupt: frame end
	KDRV_DCE_INTE_STPERR    = 0x00000008,   // enable interrupt: DCE stripe error
	KDRV_DCE_INTE_LBOVF     = 0x00000010,   // enable interrupt: DCE line buffer overflow error
	KDRV_DCE_INTE_STPOB     = 0x00000040,   // enable interrupt: DCE stripe boundary overflow error
	KDRV_DCE_INTE_YBACK     = 0x00000080,   // enable interrupt: DCE Y coordinate backward skip error
	KDRV_DCE_INTE_LL_END    = 0x00000100,   // enable interrupt: Linked List done
	KDRV_DCE_INTE_LL_ERR    = 0x00000200,   // enable interrupt: Linked List error command
	KDRV_DCE_INTE_LL_ERR2   = 0x00000400,   // enable interrupt: Linked List error in direct mdoe
	KDRV_DCE_INTE_LL_JOBEND = 0x00000800,   // enable interrupt: Linked List job end
	KDRV_DCE_INTE_FRAMEERR  = 0x00001000,   // enable interrupt: frame start error in direct mode
	KDRV_DCE_INTE_ALL       = 0x00001FDF,   // enable interrupt: all
} KDRV_DCE_INTE;

/**
    type for KDRV_DCE_SET_ISRCB
*/
typedef enum {
	KDRV_DCE_INT_FST        = 0x00000001,
	KDRV_DCE_INT_FMD        = 0x00000002,
	KDRV_DCE_INT_STPERR     = 0x00000008,
	KDRV_DCE_INT_LBOVF      = 0x00000010,
	KDRV_DCE_INT_STPOB      = 0x00000040,
	KDRV_DCE_INT_YBACK      = 0x00000080,
	KDRV_DCE_INT_LL_END     = 0x00000100,
	KDRV_DCE_INT_LL_ERR     = 0x00000200,
	KDRV_DCE_INT_LL_ERR2    = 0x00000400,
	KDRV_DCE_INT_LL_JOBEND  = 0x00000800,
	KDRV_DCE_INT_FRAMEERR   = 0x00001000,
	KDRV_DCE_INT_ALL        = 0x00001FDF,
	ENUM_DUMMY4WORD(KDRV_DCE_ISR_STATUS)
} KDRV_DCE_ISR_STATUS;

typedef enum _KDRV_DCE_IN_SRC {
	KDRV_DCE_IN_SRC_DIRECT = 0,     //input from IPP engine (RHE->IFE->DCE->IME)
	KDRV_DCE_IN_SRC_DCE_IME,        //input from dram and direct to IME (DCE->IPE->IME)
	KDRV_DCE_IN_SRC_ALL_DIRECT,     //input from SIE (SIE->RHE->IFE->DCE->IME), only single stripe
	KDRV_DCE_IN_SRC_DRAM,           //input from dram
	ENUM_DUMMY4WORD(KDRV_DCE_IN_SRC)
} KDRV_DCE_IN_SRC;

typedef enum _KDRV_DCE_OP_MODE {
	KDRV_DCE_OP_MODE_NORMAL = 0,    //input from IPP engine (RHE->IFE->DCE->IME)
	KDRV_DCE_OP_MODE_CAL_STRIP,     //Do not use! It is used by flow automatically
	ENUM_DUMMY4WORD(KDRV_DCE_OP_MODE)
} KDRV_DCE_OP_MODE;

typedef enum {
	KDRV_DCE_DRAM_OUT_NORMAL            = 0,    ///<
	KDRV_DCE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(KDRV_DCE_DRAM_OUTPUT_MODE)
} KDRV_DCE_DRAM_OUTPUT_MODE;

/**
    DCE KDRV - input image data format selection
*/
typedef enum _KDRV_DCE_D2D_FMT_TYPE {
	KDRV_DCE_D2D_FMT_Y_PACK_UV422 = 0,  //YCC422 separate planar format for input
	KDRV_DCE_D2D_FMT_Y_PACK_UV420 = 1,  //YCC420 separate planar format for input
	KDRV_DCE_D2D_TEST_MODE = 2,         //reversed
	ENUM_DUMMY4WORD(KDRV_DCE_D2D_FMT_TYPE)
} KDRV_DCE_D2D_FMT_TYPE;

/**
    DCE KDRV - input yuv data range format selection
*/
typedef enum _KDRV_DCE_D2D_YUVRGB_FMT {
	KDRV_DCE_YUV2RGB_FMT_FULL = 0,      //YUV full range format
	KDRV_DCE_YUV2RGB_FMT_BT601 = 1,     //YUV BT601 range format
	KDRV_DCE_YUV2RGB_FMT_BT709 = 2,     //YUV BT709 range format
	ENUM_DUMMY4WORD(KDRV_DCE_D2D_YUVRGB_FMT)
} KDRV_DCE_D2D_YUVRGB_FMT;


/**
    YUV channel index
*/
//@{
typedef enum _KDRV_DCE_YUV_CH {
	KDRV_DCE_YUV_Y = 0,                 //process channel Y
	KDRV_DCE_YUV_UV,                    //process channel U
	KDRV_DCE_YUV_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_DCE_YUV_CH)
} KDRV_DCE_YUV_CH;

/**
    DCE KDRV Stripe Type
*/
typedef enum _KDRV_DCE_STRIPE_TYPE {
	KDRV_DCE_STRIPE_AUTO = 0,           //Auto calculation
	KDRV_DCE_STRIPE_MANUAL_MULTI,       //Force mult-stripe
	KDRV_DCE_STRIPE_MANUAL_2STRIPE,     //Force 2 stripe
	KDRV_DCE_STRIPE_MANUAL_3STRIPE,     //Force 3 stripe
	KDRV_DCE_STRIPE_MANUAL_4STRIPE,     //Force 4 stripe
	KDRV_DCE_STRIPE_MANUAL_5STRIPE,     //Force 5 stripe
	KDRV_DCE_STRIPE_MANUAL_6STRIPE,     //Force 6 stripe
	KDRV_DCE_STRIPE_MANUAL_7STRIPE,     //Force 7 stripe
	KDRV_DCE_STRIPE_MANUAL_8STRIPE,     //Force 8 stripe
	KDRV_DCE_STRIPE_MANUAL_9STRIPE,     //Force 9 stripe
	KDRV_DCE_STRIPE_CUSTOMER,           //config stripe setting by user
	ENUM_DUMMY4WORD(KDRV_DCE_STRIPE_TYPE)
} KDRV_DCE_STRIPE_TYPE;

typedef enum _KDRV_DCE_FOV_BND_SEL {
	KDRV_DCE_FOV_BND_DUPLICATE = 0,     ///< Replace out of boundary pixels with duplicate boundary pixels
	KDRV_DCE_FOV_BND_REG_RGB = 1,       ///< Replace out of boundary pixels with GEO_BOUNDR, GEO_BOUNDG, GEO_BOUNDB
	ENUM_DUMMY4WORD(KDRV_DCE_FOV_BND_SEL)
} KDRV_DCE_FOV_BND_SEL;

typedef enum _KDRV_DCE_2DLUT_NUM {
	KDRV_DCE_2DLUT_65_65_GRID = 0,      ///< 65x65 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_33_33_GRID = 1,      ///< 33x33 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_17_17_GRID = 2,      ///< 17x17 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_9_9_GRID = 3,        ///< 9x9 grid 2D lut distortion correction
	ENUM_DUMMY4WORD(KDRV_DCE_2DLUT_NUM)
} KDRV_DCE_2DLUT_NUM;

typedef enum {
	KDRV_DCE_CGAIN_2_8 = 0,     ///< gain range integer/fraction 2.8
	KDRV_DCE_CGAIN_3_7 = 1,     ///< gain range integer/fraction 3.7
	ENUM_DUMMY4WORD(KDRV_DCE_CGAIN_RANGE)
} KDRV_DCE_CGAIN_RANGE;

typedef enum {
	KDRV_DCE_PINKR_MOD_G  = 0,   ///< modify G channel
	KDRV_DCE_PINKR_MOD_RB = 1,   ///< modify RB channel
	ENUM_DUMMY4WORD(KDRV_DCE_PINKR_MODE)
} KDRV_DCE_PINKR_MODE;

typedef enum {
	KDRV_XY_BOTH = 0,           ///< processing both X and Y
	KDRV_X_ONLY  = 1,           ///< processing X only
	ENUM_DUMMY4WORD(KDRV_DCE_GDC_LUT_MODE)
} KDRV_DCE_GDC_LUT_MODE;

typedef enum {
	KDRV_DCE_SRAM_DCE = 0,       ///< Use DCE SRAM only
	KDRV_DCE_SRAM_CNN = 1,       ///< Use DCE and CNN SRAM
	ENUM_DUMMY4WORD(KDRV_DCE_SRAM_SEL)
} KDRV_DCE_SRAM_SEL;

typedef enum {
	KDRV_INPUT_BLD_3x3_Y  = 0,   ///< use Y from bayer 3x3 as the input blending source selection
	KDRV_INPUT_BLD_GMEAN  = 1,   ///< use Gmean from bayer 3x3 as the input blending source selection
	KDRV_INPUT_BLD_BAYER  = 2,   ///< use Bayer as the input blending source selection
	ENUM_DUMMY4WORD(KDRV_DCE_WDR_INPUT_BLDSEL)
} KDRV_DCE_WDR_INPUT_BLDSEL;

typedef enum {
	KDRV_BEFORE_WDR      = 0,    ///< histogram statistics before wdr algorithm
	KDRV_AFTER_WDR       = 1,    ///< histogram statistics after wdr algorithm
	ENUM_DUMMY4WORD(KDRV_DCE_HIST_SEL)
} KDRV_DCE_HIST_SEL;

typedef enum {
	KDRV_DCE_CFA_SUBOUT_CH0 = 0,         ///< Select channel 0 as output channel
	KDRV_DCE_CFA_SUBOUT_CH1 = 1,         ///< Select channel 1 as output channel
	KDRV_DCE_CFA_SUBOUT_CH2 = 2,         ///< Select channel 2 as output channel
	KDRV_DCE_CFA_SUBOUT_CH3 = 3,         ///< Select channel 3 as output channel
	ENUM_DUMMY4WORD(KDRV_DCE_CFA_SUBOUT_CH_SEL)
} KDRV_DCE_CFA_SUBOUT_CH_SEL;

typedef enum {
	KDRV_DCE_CFA_SUBOUT_1BYTE = 0,       ///< Output 1 byte per pixel for CFA subout
	KDRV_DCE_CFA_SUBOUT_2BYTE = 1,       ///< Output 2 byte per pixel for CFA subout
	ENUM_DUMMY4WORD(KDRV_DCE_CFA_SUBOUT_BYTE)
} KDRV_DCE_CFA_SUBOUT_BYTE;

typedef enum {
	KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR = 0,   ///< stripe configuration for low latency best performance
	KDRV_DCE_STRP_GDC_HIGH_PRIOR = 1,       ///< stripe configuration for GDC + latency both running
	KDRV_DCE_STRP_2DLUT_HIGH_PRIOR = 2,     ///< stripe configuration for 2DLUT best performance
	KDRV_DCE_STRP_GDC_BEST = 3,     		///< stripe configuration for GDC best performance
	ENUM_DUMMY4WORD(KDRV_DCE_STRP_RULE_SEL)
} KDRV_DCE_STRP_RULE_SEL;


/**
    channel information
*/

//structure data type
typedef struct _KDRV_DCE_TRIGGER_PARAM {
	UINT32 RESERVED;
} KDRV_DCE_TRIGGER_PARAM;

/**
    DCE structure - dce kdrv open object
*/
typedef struct _KDRV_DCE_OPENCFG {
	UINT32 dce_clock_sel;       //Engine clock selection
} KDRV_DCE_OPENCFG, *pKDRV_DCE_OPENCFG;

//----------------------------------------------------------------------

/**
    DCE KDRV trigger type
*/
typedef struct _KDRV_DCE_TRIG_TYPE_INFO {
	KDRV_DCE_OP_MODE opmode;    //DCE operating mode
} KDRV_DCE_TRIG_TYPE_INFO;

//----------------------------------------------------------------------

/**
    DCE structure - Dram single output channel enable
*/
typedef struct {
	BOOL single_out_wdr_en;
	BOOL single_out_cfa_en;
} KDRV_DCE_DRAM_SINGLE_OUT_EN;

//----------------------------------------------------------------------

/**
    DCE KDRV Input Image Information
*/
typedef struct _KDRV_DCE_IN_INFO {
	KDRV_DCE_IN_SRC in_src;                 //input image source
	KDRV_DCE_D2D_FMT_TYPE type;             //image type
	KDRV_DCE_D2D_YUVRGB_FMT yuvfmt;         //d2d input yuv format
} KDRV_DCE_IN_INFO;

//----------------------------------------------------------------------

typedef struct _KDRV_DCE_IN_IMG {
	KDRV_IPP_PIX cfa_pat;                   //CFA pattern
	UINT32 img_size_h;                      //image width
	UINT32 img_size_v;                      //image height
	UINT32 lineofst[KDRV_DCE_YUV_MAX_CH];   // DRAM line offset of all image input channel
} KDRV_DCE_IN_IMG_INFO;

//----------------------------------------------------------------------

typedef struct _KDRV_DCE_OUT_IMG {
	UINT32 crop_size_h;                     //output image width
	UINT32 crop_size_v;                     //output image height
	UINT32 crop_start_h;                    //crop start x
	UINT32 crop_start_v;                    //crop start y
	UINT32 lineofst[KDRV_DCE_YUV_MAX_CH];   //DRAM line offset of all image output channel
} KDRV_DCE_OUT_IMG_INFO;

//----------------------------------------------------------------------

/**
    DCE KDRV Address info
*/
typedef struct _KDRV_DCE_IMG_DMA_INFO {
	UINT32 addr_y;                          //DMA address of Y channel
	UINT32 addr_cbcr;                       //DMA address of Cb channel
} KDRV_DCE_IMG_IN_DMA_INFO, KDRV_DCE_IMG_OUT_DMA_INFO;

typedef struct _KDRV_DCE_SUBIMG_IN_ADDR {
	UINT32 addr;                            //DMA address of Subimg input channel
} KDRV_DCE_SUBIMG_IN_ADDR;

typedef struct _KDRV_DCE_SUBIMG_OUT_ADDR {
	BOOL enable;                            //Enable Subimg output channel
	UINT32 addr;                            //DMA address of Subimg output channel
} KDRV_DCE_SUBIMG_OUT_ADDR;

//----------------------------------------------------------------------

/**
    DCE KDRV Stripe Information For IME Reference
*/
typedef struct _KDRV_DCE_STRIPE_INFO {
	UINT32 hstp_num;                        //H stripe number
	UINT16 hstp[IMG_MAX_STP_NUM];           //H stripe width of every stripe of DCE output stripe
	UINT16 ime_in_hstp[IMG_MAX_STP_NUM];    //H stripe width of every IME input stripe
	UINT16 ime_out_hstp[IMG_MAX_STP_NUM];   //H stripe width of every IME output stripes
} KDRV_DCE_STRIPE_INFO;

//----------------------------------------------------------------------

typedef struct _KDRV_DCE_GDC_CENTER_INFO {
	UINT32 geo_center_x;                    //Geometric distortion X center
	UINT32 geo_center_y;                    //Geometric distortion Y center
} KDRV_DCE_GDC_CENTER_INFO;


//----------------------------------------------------------------------

typedef struct _KDRV_DCE_DC_SRAM_SEL {
	KDRV_DCE_SRAM_SEL sram_mode;            //sram mode configuration
} KDRV_DCE_DC_SRAM_SEL;

//----------------------------------------------------------------------

/**
    DCE KDRV Calculate Stripe
*/
typedef struct _KDRV_DCE_EXTEND_INFO {
	BOOL ime_enc_enable;            //enable bit of IME encode
	BOOL ime_3dnr_enable;           //enable bit of IME 3DNR
	BOOL ime_dec_enable;            //enable bit of IME decode
	BOOL ime_isd_enable;            //enable bit of IME ISD
	BOOL ime_ref_out_enable;        //enable bit of IME reference out
	BOOL ime_p1_enc_enable;         //enable bit of IME path 1 encode
	BOOL low_latency_enable;        //enable bit of IME/Codec low latency
} KDRV_DCE_EXTEND_INFO;

//----------------------------------------------------------------------

/**
    DCE KDRV Calculate Stripe
*/
typedef struct _KDRV_DCE_STRP_RULE_INFO {
	KDRV_DCE_STRP_RULE_SEL dce_strp_rule;   //dce stripe rule setting for GDC and low latency
} KDRV_DCE_STRP_RULE_INFO;

//----------------------------------------------------------------------

/**
    DCE KDRV Stripe Information For IME Reference
*/
typedef struct _KDRV_DCE_STRIPE_PARAM {
	KDRV_DCE_STRIPE_TYPE stripe_type;   //assign stripe type for driver calculation or user-assigned value
	UINT32 hstp[DCE_PARTITION_NUM];     //stripe width of DCE output partitions
} KDRV_DCE_STRIPE_PARAM;

//----------------------------------------------------------------------

/**
    DCE KDRV CFA direction decision
*/
typedef struct _KDRV_DCE_CFA_DIR {
	UINT32  diff_norm_bit;          //Difference norm bit, range [0~3]
	UINT32  ns_mar_edge;            //Noise margin for edge, range [0~1023]
	UINT32  ns_mar_diff;            //Noise margin for pixel difference, range [0~1023]
} KDRV_DCE_CFA_DIR;

/**
    DCE KDRV CFA correction term
*/
typedef struct _KDRV_DCE_CFA_CORR {
	//BOOL    g_corr_enable;        //Enable for G channel correction
	BOOL    rb_corr_enable;         //Enable for RB channel correction
	//UINT32  g_corr_norm_bit;      //G correction norm bit, range [0~3]
	//UINT32  g_corr_th;            //G correction noise threshold, range [0~1023]
	UINT32  rb_corr_th1;            //RB correction noise threshold1, range [0~1023]
	UINT32  rb_corr_th2;            //RB correction noise threshold2, range [0~1023]
} KDRV_DCE_CFA_CORR;

/**
    DCE KDRV CFA high frequency check term
*/
typedef struct _KDRV_DCE_CFA_HFC {
	BOOL    cl_check_enable;        ///< Enable cross line check
	BOOL    hf_check_enable;        ///< Enable high frequency check
	BOOL    average_mode;           ///< Interpolation method selection when flat region detect
	BOOL    cl_sel;                 ///< operation type when cross line is not found
	UINT32  cl_th;                  ///< Cross line checking threshold, range [0~255]
	UINT32  hfg_th;                 ///< G channel smoothness checking threshold, range [0~7]
	UINT32  hf_diff;                ///< Difference threshold value between Gr and Gb to determing high frequency pattern detected or not, range [0~255]
	UINT32  hfe_th;                 ///< Threshold to determine H or V direction high frequency pattern detected, range [0~255]
	UINT32  ir_g_edge_th;           ///< Noise margin for edge in Green component interpolation [0~1023]
	UINT32  ir_rb_cstrength;        ///< RB channel smooth strength [0~7]

} KDRV_DCE_CFA_HFC;

/**
    DCE structure - CFA interpolation term
*/
typedef struct {
	UINT32    edge_dth;                         ///< Edge threshold 1 for Bayer CFA interpolation [0~4095]
	UINT32    edge_dth2;                        ///< Edge threshold 2 for Bayer CFA interpolation [0~4095]
	UINT32    freq_th;                          ///< CFA frequency threshold [0~4095]
	UINT32    freq_lut[KDRV_CFA_FREQ_NUM];      ///< CFA frequency blending weight look-up table [0~15]
	UINT32    luma_wt[KDRV_CFA_LUMA_NUM];       ///< Luma gain for CFA [0~255]
} KDRV_DCE_CFA_INTERP;

/**
    DCE structure - CFA false color suppression term
*/
typedef struct {
	BOOL      fcs_dirsel;                       ///< Direction selection for detecting high frequency, 0: detect 2 directions, 1: detect 4 directions
	UINT32    fcs_coring;                       ///< Coring threshold for false color suppression [0~255]
	UINT32    fcs_weight;                       ///< Global strength for false color suppression [0~255]
	UINT32    fcs_strength[KDRV_CFA_FCS_NUM];   ///< CFA false color suppression strength [0~15]
} KDRV_DCE_CFA_FCS;

/**
    DCE structure - CFA IR subtraction term
*/
typedef struct {
	UINT32    ir_sub_r;                         ///< R channel IR sub [0~4095]
	UINT32    ir_sub_g;                         ///< G channel IR sub [0~4095]
	UINT32    ir_sub_b;                         ///< B channel IR sub [0~4095]
	UINT32    ir_sub_wt_lb;                     ///< Lower bound of IR local weighting [0~255]
	UINT32    ir_sub_th;                        ///< Threshold to decay IR local weighting [0~255]
	UINT32    ir_sub_range;                     ///< Range to decay IR local weighting [0~3]
	UINT32    ir_sat_gain;                      ///< Saturation gain multiplied after IR sub [0~1023]
} KDRV_DCE_CFA_IR;

/**
    DCE structure - CFA color gain term
*/
typedef struct {
	UINT32    r_gain;                           ///< R channel gain [0~1023]
	UINT32    g_gain;                           ///< G channel gain [0~1023]
	UINT32    b_gain;                           ///< B channel gain [0~1023]
	KDRV_DCE_CGAIN_RANGE gain_range;            ///< select fraction type of gain: 2.8 or 3.7
} KDRV_DCE_CFA_CGAIN;

/**
    DCE structure - CFA pink reduction term
*/
typedef struct {
	BOOL pink_rd_en;                            ///< pink reduction enable
	KDRV_DCE_PINKR_MODE pink_rd_mode;           ///< pink reduction mode
	UINT32    pink_rd_th1;                      ///< pink reduction threshold 1 [0~255]
	UINT32    pink_rd_th2;                      ///< pink reduction threshold 2 [0~255]
	UINT32    pink_rd_th3;                      ///< pink reduction threshold 3 [0~255]
	UINT32    pink_rd_th4;                      ///< pink reduction threshold 4 [0~255]
} KDRV_DCE_CFA_PINKR;

/**
    DCE structure - DCE CFA parameters.
*/
//@{
typedef struct {
	BOOL cfa_enable;
	KDRV_DCE_CFA_INTERP cfa_interp;             ///< CFA interpolation term
	KDRV_DCE_CFA_CORR cfa_correction;           ///< CFA correction term
	KDRV_DCE_CFA_FCS cfa_fcs;                   ///< CFA false color suppression term
	KDRV_DCE_CFA_HFC cfa_ir_hfc;                ///< CFA high frequency check term
	KDRV_DCE_CFA_IR cfa_ir_sub;                 ///< CFA IR subtraction term
	KDRV_DCE_CFA_PINKR cfa_pink_reduc;          ///< CFA pink reduction term
	KDRV_DCE_CFA_CGAIN cfa_cgain;               ///< CFA color gain term
} KDRV_DCE_CFA_PARAM;


//----------------------------------------------------------------------

/**
    DCE KDRV Distortion Correction Parameters of FOV boundary replacement
*/
typedef struct _KDRV_DCE_FOV_PARAM {
	BOOL fov_enable;
	UINT32 fov_gain;                    ///< 1024 is 1x, Scale down factor on corrected image (12 bit: 2.10)
	KDRV_DCE_FOV_BND_SEL fov_bnd_sel;   ///< Select out-of-bound pixel source
	UINT32 fov_bnd_r;                   ///< Replace value, range [0~1023]
	UINT32 fov_bnd_g;                   ///< Replace value, range [0~1023]
	UINT32 fov_bnd_b;                   ///< Replace value, range [0~1023]
} KDRV_DCE_FOV_PARAM;

//----------------------------------------------------------------------

/**
    DCE KDRV Distortion Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_DC {
	UINT32 geo_dist_x;                  ///< X input distance factor
	UINT32 geo_dist_y;                  ///< Y input distance factor
	UINT32 geo_lut_g[GDC_TABLE_NUM];    ///< 65 x 16bit
} KDRV_DCE_DC;

/**
    DCE KDRV Chromatic Aberration Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_CAC {
	BOOL cac_enable;
	UINT32 geo_r_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT32 geo_g_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT32 geo_b_lut_gain;              ///< 4096 is 1x, range [0~8191]
	INT32 geo_lut_r[CAC_TABLE_NUM];     ///< 65 x 16bit
	INT32 geo_lut_b[CAC_TABLE_NUM];     ///< 65 x 16bit
	BOOL cac_sel;                       ///< CAC use 0:G_Lut & RGB Lut Gain, 1: RGB Lut
} KDRV_DCE_CAC;

/**
    DCE KDRV DC and CAC Param
*/
typedef struct _KDRV_DCE_DC_CAC_PARAM {
	BOOL dc_enable;
	KDRV_DCE_GDC_LUT_MODE dc_y_dist_off;
	KDRV_DCE_DC dc;
	KDRV_DCE_CAC cac;
} KDRV_DCE_DC_CAC_PARAM;

//----------------------------------------------------------------------

/**
    DCE KDRV 2D Lut Distortion Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_2DLUT_PARAM {
	BOOL lut_2d_enable;
	KDRV_DCE_GDC_LUT_MODE lut_2d_y_dist_off;
	KDRV_DCE_2DLUT_NUM lut_num_select;
	UINT32 lut_2d_value[LUT2D_MAX_TABLE_SIZE];
} KDRV_DCE_2DLUT_PARAM;


//----------------------------------------------------------------------

/**
    DCE structure - WDR non-equal table parameters
*/
typedef struct {
	UINT32 lut_idx[WDR_NEQ_TABLE_IDX_NUM];      ///< non equatable table index [0~63]
	UINT32 lut_split[WDR_NEQ_TABLE_IDX_NUM];    ///< non equatable table split number [0~3]
	UINT32 lut_val[WDR_NEQ_TABLE_VAL_NUM];      ///< non equatable table value [0~4095]
} KDRV_WDR_NEQ_TABLE;

/**
    DCE structure - WDR input blending parameters
*/
typedef struct {
	KDRV_DCE_WDR_INPUT_BLDSEL bld_sel;
	UINT32 bld_wt;                              ///< blending of Y and localmax [0~8],0: Y, 8: local max
	UINT32 blend_lut[WDR_INPUT_BLD_NUM];        ///< blending of Y and raw [0~255], 0: Y, 255: raw
} KDRV_WDR_IN_BLD_PARAM;

/**
    DCE structure - WDR strength parameters
*/
typedef struct {
	INT32 wdr_coeff[WDR_COEF_NUM];              ///< wdr coefficient [-4096~4095]
	UINT32 strength;                            ///< wdr strength [0~255]
	UINT32 contrast;                            ///< wdr contrast [0~255], 128 = original
} KDRV_WDR_STRENGTH_PARAM;

/**
    DCE structure - WDR gain control parameters
*/
typedef struct {
	BOOL gainctrl_en;                           ///< enable gain control for wdr
	UINT32 max_gain;                            ///< maximum gain of wdr [1~255]
	UINT32 min_gain;                            ///< minimum gain of wdr [1~255], mapping to 1/256, 2/256, 255/256
} KDRV_WDR_GAINCTRL_PARAM;

/**
    DCE structure - WDR output blending parameters
*/
typedef struct {
	BOOL outbld_en;                             ///< enable output blending for wdr
	KDRV_WDR_NEQ_TABLE outbld_lut;              ///< wdr output blending table
} KDRV_WDR_OUTBLD_PARAM;

/**
    DCE structure - WDR saturation reduction parameters
*/
typedef struct {
	UINT32 sat_th;                              ///< threshold of wdr saturation reduction [0~4095]
	UINT32 sat_wt_low;                          ///< lower weight of wdr saturation reduction [0~255]
	UINT32 sat_delta;                           ///< delta of wdr saturation reduction [0~255]
} KDRV_WDR_SAT_REDUCT_PARAM;

/**
    DCE KDRV WDR/TONE CURVE Param and Enable/Disable
*/
typedef struct _KDRV_DCE_WDR_PARAM {
	BOOL wdr_enable;
	BOOL tonecurve_enable;
	UINT32  ftrcoef[WDR_SUBIMG_FILT_NUM];       ///< wdr sub-image low pass filter coefficients, 3 entries
	KDRV_WDR_IN_BLD_PARAM     input_bld;        ///< wdr input blending
	KDRV_WDR_NEQ_TABLE        tonecurve;        ///< wdr tonecurve configuration
	KDRV_WDR_STRENGTH_PARAM   wdr_str;          ///< wdr strength
	KDRV_WDR_GAINCTRL_PARAM   gainctrl;         ///< wdr gain control
	KDRV_WDR_OUTBLD_PARAM     outbld;           ///< wdr output blending settings
	KDRV_WDR_SAT_REDUCT_PARAM sat_reduct;       ///< wdr saturation reduction
} KDRV_DCE_WDR_PARAM;

/**
    DCE KDRV WDR subimg size Information
*/
typedef struct {
	UINT32           subimg_size_h;            ///< wdr sub-image size [4~32]
	UINT32           subimg_size_v;            ///< wdr sub-image size [4~32]
	UINT32           subimg_lofs_in;           ///< wdr subin lineoffset, lofs >= (subimg_size_h * 8)
	UINT32           subimg_lofs_out;          ///< wdr subout lineoffset, lofs >= (subimg_size_h * 8)
} KDRV_DCE_WDR_SUBIMG_PARAM;


/**
    DCE KDRV Histogram Param and Enable/Disable
*/
typedef struct _KDRV_DCE_HIST_PARAM {
	BOOL hist_enable;                           ///< Enable histogram statistics
	KDRV_DCE_HIST_SEL hist_sel;                 ///< selection of statistics input
	UINT32 step_h;                              ///< h step size of histogram [0~31]
	UINT32 step_v;                              ///< v step size of histogram [0~31]
} KDRV_DCE_HIST_PARAM;


/**
    DCE KDRV Histogram Output Rslt
*/
typedef struct _KDRV_DCE_HIST_RSLT {
	BOOL enable;                                ///< Enable histogram statistics
	KDRV_DCE_HIST_SEL hist_sel;                 ///< selection of statistics input
	UINT16 hist_stcs[HISTOGRAM_STCS_NUM];       ///< histogram statistics
} KDRV_DCE_HIST_RSLT ;

//----------------------------------------------------------------------

/**
    DCE KDRV CFA subout Param and Enable/Disable
*/
typedef struct _KDRV_DCE_CFA_SUBOUT_PARAM {
	BOOL cfa_subout_enable;
	BOOL cfa_subout_flip_enable;
	UINT32 cfa_addr;                                 ///< cfa subout addr
	UINT32 cfa_lofs;                                 ///< cfa subout lineoffsett
	KDRV_DCE_CFA_SUBOUT_CH_SEL subout_ch_sel;        ///< CFA sub-img output channel selection
	KDRV_DCE_CFA_SUBOUT_BYTE subout_byte;            ///< CFA sub-img output format
	UINT32 subout_shiftbit;                          ///< CFA sub-img output right shift bit
} KDRV_DCE_CFA_SUBOUT_INFO;

//----------------------------------------------------------------------

/**
    DCE KDRV Histogram Output Rslt
*/
typedef struct _KDRV_DCE_STRIPE_NUM {
	UINT32 num;                         ///< save stripe number calculated by rules
} KDRV_DCE_STRIPE_NUM;

//----------------------------------------------------------------------
/**
    KDRV DCE enum - function selection
*/
typedef enum _KDRV_DCE_IQ_FUNC_ {
	KDRV_DCE_IQ_FUNC_CFA          = 0x00000001,
	KDRV_DCE_IQ_FUNC_DC           = 0x00000002,
	KDRV_DCE_IQ_FUNC_CAC          = 0x00000004,
	KDRV_DCE_IQ_FUNC_2DLUT        = 0x00000008,
	KDRV_DCE_IQ_FUNC_FOV          = 0x00000010,//virtual enable
	KDRV_DCE_IQ_FUNC_WDR          = 0x00000020,
	KDRV_DCE_IQ_FUNC_TONECURVE    = 0x00000040,
	KDRV_DCE_IQ_FUNC_HISTOGRAM    = 0x00000080,
	ENUM_DUMMY4WORD(KDRV_DCE_IQ_FUNC)
} KDRV_DCE_IQ_FUNC;


/**
    KDRV DCE all IQ parameters
*/
typedef struct _KDRV_DCE_PARAM_IQ_ALL_PARAM_ {
	KDRV_DCE_CFA_PARAM          *p_cfa_param;
	KDRV_DCE_DC_CAC_PARAM       *p_dc_cac_param;
	KDRV_DCE_2DLUT_PARAM        *p_2dlut_param;
	KDRV_DCE_FOV_PARAM          *p_fov_param;
	KDRV_DCE_STRIPE_PARAM       *p_stripe_param;
	KDRV_DCE_WDR_PARAM          *p_wdr_param;
	KDRV_DCE_WDR_SUBIMG_PARAM   *p_wdr_subimg_param;
	KDRV_DCE_HIST_PARAM         *p_hist_param;
} KDRV_DCE_PARAM_IQ_ALL_PARAM;

/**
    KDRV DCE all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_DCE_PARAM_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;   ///<It labels which function will be controlled by KFLOW
	BOOL   enable;          ///
} KDRV_DCE_PARAM_IPL_FUNC_EN;
//----------------------------------------------------------------------

/**
    DCE KDRV PARAM ID, for kdrv_dce_set/kdrv_dce_get
*/
typedef enum _KDRV_DCE_PARAM_ID {
	KDRV_DCE_PARAM_IPL_OPENCFG,      // [Set], set clock info
	KDRV_DCE_PARAM_IPL_IN_INFO,      // [Set/Get], use KDRV_DCE_IN_INFO structure, set dce input source and input image type
	KDRV_DCE_PARAM_IPL_IN_IMG,       // [Set/Get], use KDRV_DCE_IN_IMG_INFO structure, set input image size info, line offset
	KDRV_DCE_PARAM_IPL_OUT_IMG,      // [Set/Get], use KDRV_DCE_OUT_IMG_INFO structure, set output image crop info, line offset
	KDRV_DCE_PARAM_IPL_IMG_DMA_IN,   // [Set/Get], use KDRV_DCE_IMG_IN_DMA_INFO structure, set the dram input address, line offset in DRAM_in_out mode
	KDRV_DCE_PARAM_IPL_IMG_DMA_OUT,  // [Set/Get], use KDRV_DCE_IMG_OUT_DMA_INFO structure, set the dram output address, line offset in DRAM_in_out mode

	KDRV_DCE_PARAM_IPL_DMA_SUBIN,    // [Set/Get], use KDRV_DCE_SUBIMG_IN_ADDR structure, set the subimg input address
	KDRV_DCE_PARAM_IPL_DMA_SUBOUT,   // [Set/Get], use KDRV_DCE_SUBIMG_OUT_ADDR structure, set the subimg output address

	KDRV_DCE_PARAM_IPL_OUTPUT_MODE,  // [Set/Get], use KDRV_DCE_DRAM_OUTPUT_MODE structure, DCE dram output mode,
	KDRV_DCE_PARAM_IPL_SINGLE_OUT,   // [Set/Get], use KDRV_DCE_DRAM_SINGLE_OUT_EN structure, DCE dram single out channel,

	KDRV_DCE_PARAM_IPL_GDC_CENTER,   // [Set/Get], use KDRV_DCE_GDC_CENTER_INFO, set dce 2dlut input address
	KDRV_DCE_PARAM_IPL_DC_SRAM_MODE, // [Set/Get], use KDRV_DCE_DC_SRAM_SEL, set dce DC sram mode selection
	KDRV_DCE_PARAM_IPL_STRIPE_INFO,  // [Get], use KDRV_DCE_STRIPE_INFO structure, get dce calculated stripe info
	KDRV_DCE_PARAM_IPL_ISR_CB,       // [Set/Get], use KDRV_IPP_ISRCB structure, set dce external isr cb
	KDRV_DCE_PARAM_IPL_INTER,        // [Set/Get], use KDRV_DCE_INTE structure, set dce interrupt enable
	KDRV_DCE_PARAM_IPL_LOAD,         // [Set] NULL or KDRV_DCE_SET_ITEM, DCE set load
	KDRV_DCE_PARAM_IPL_EXTEND,       // [Set/Get], use KDRV_DCE_EXTEND_INFO structure, set dce extend(IPE/IME) stripe overlap info
	KDRV_DCE_PARAM_IPL_CFAOUT,       // [Set/Get], use KDRV_DCE_CFA_SUBOUT_INFO
	KDRV_DCE_PARAM_IPL_STRIPE_RULE,  // [Set/Get], use KDRV_DCE_STRP_RULE_SEL structure, set dce stripe rule for GDC and low latency

	KDRV_DCE_PARAM_IQ_CFA,           // [Set/Get], use KDRV_DCE_CFA_PARAM structure, set dce cfa info
	KDRV_DCE_PARAM_IQ_DC_CAC,        // [Set/Get], use KDRV_DCE_DC_CAC_PARAM structure, set dce distortion and chromatic aberration correction info
	KDRV_DCE_PARAM_IQ_2DLUT,         // [Set/Get], use KDRV_DCE_2DLUT_PARAM sturcture, set dce 2D Lut info
	KDRV_DCE_PARAM_IQ_FOV,           // [Set/Get], use KDRV_DCE_FOV_PARAM sturcture, set dce FOV info
	KDRV_DCE_PARAM_IQ_STRIPE_PARAM,  // [Set/Get], use KDRV_DCE_STRIPE_PARAM, set dce stripe calculation parameters

	KDRV_DCE_PARAM_IQ_WDR_SUBIMG,    // [Set/Get], use KDRV_DCE_WDR_SUBIMG_PARAM structure, set wdr subimg size info
	KDRV_DCE_PARAM_IQ_WDR,           // [Set/Get], use KDRV_DCE_WDR_PARAM
	KDRV_DCE_PARAM_IQ_HIST,          // [Set/Get], use KDRV_DCE_HIST_PARAM
	KDRV_DCE_PARAM_IPL_HIST_RSLT,    // [Get], use KDRV_DCE_HIST_RSLT
	KDRV_DCE_PARAM_IPL_STRIPE_NUM,   // [Get], use KDRV_DCE_STRIPE_NUM

	KDRV_DCE_PARAM_IQ_ALL,           // [Set], use KDRV_DCE_PARAM_IQ_ALL_PARAM, set dce all IQ param
	KDRV_DCE_PARAM_IPL_ALL_FUNC_EN,  // [Set], use KDRV_DCE_PARAM_IPL_FUNC_EN, dce func enable/disable
	KDRV_DCE_PARAM_IPL_RESET,        // [Set], use KDRV_DCE_PARAM_IPL_RESET, dce async reset
	KDRV_DCE_PARAM_IPL_BUILTIN_DISABLE, //[Set], NULL, dce disable builtin flow control
	KDRV_DCE_PARAM_MAX,
	KDRV_DCE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_DCE_PARAM_ID)
} KDRV_DCE_PARAM_ID;
STATIC_ASSERT((KDRV_DCE_PARAM_MAX &KDRV_DCE_PARAM_REV) == 0);

#define KDRV_DCE_IGN_CHK KDRV_DCE_PARAM_REV  //only support set/get function

#endif //_KDRV_DCE_H_
