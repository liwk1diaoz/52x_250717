/**
    This file is the public hearder file of DCE engine, \n
    including APIs' declaration and data type's definition

    @file              dce_lib.h

    @ingroup       mIDrvIPP_DCE

    @notes           None

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _DCE_LIB_H_
#define _DCE_LIB_H_


#define _EMULATION_ 0
#define NEW_STRIPE_RULE TRUE

#ifdef __KERNEL__
#include    "kwrap/type.h"
#else
// NOTE: XXXXX
#include <kwrap/nvt_type.h>
//#include "Type.h"
//#include "ErrorNo.h"
#endif
//*********************************************************************************************************
/**
    @addtogroup mIDrvIPP_DCE
*/
//@{

/**
    @name Define DCE function.
*/
//@{
#define DCE_FUNC_CFA                   0x00000010  ///< CFA interpolation function
#define DCE_FUNC_DC                    0x00000020  ///< Distortion correction function
#define DCE_FUNC_CAC                   0x00000040  ///< Chromatic aberration correction function
#define DCE_FUNC_YUV2RGB               0x00000080  ///< YUV to RGB function in D2D mode
#define DCE_FUNC_TCURVE                0x00000100  ///< Tone curve function
#define DCE_FUNC_WDR                   0x00000200  ///< WDR function
#define DCE_FUNC_WDR_SUBOUT            0x00000400  ///< WDR subout function
#define DCE_FUNC_CFA_SUBOUT            0x00000800  ///< CFA subout function
#define DCE_FUNC_CFA_SUBOUT_FLIP       0x00001000  ///< CFA subout flip function
#define DCE_FUNC_CFA_PINKR             0x00002000  ///< Pink reduction function
#define DCE_FUNC_HISTOGRAM             0x00004000  ///< Histogram statistics function
#define DCE_FUNC_CROP                  0x00040000  ///< Output crop function
#define DCE_FUNC_D2D_RAND              0x00200000  ///< D2D random LSB function
#define DCE_FUNC_D2D_UVFILT            0x08000000  ///< D2D UV filter function
#define DCE_FUNC_D2D_IOSTOP            0x10000000  ///< D2D UV filter function
#define DCE_FUNC_ALL                   0x18247ff0  ///< All function mask

//remove in 520
//#define DCE_FUNC_CFA_GC                0x00010000  ///< G channel correction function
//#define DCE_FUNC_CFA_RBC               0x00010000  ///< RB channel correction function
//#define DCE_FUNC_CFA_CL                0x00010000  ///< Distortion correction function
//#define DCE_FUNC_CFA_HF                0x00010000  ///< Distortion correction function
//#define DCE_FUNC_CFA_4x4RB_DIR         0x00010000  ///< Distortion correction function
//@}

/**
    @name Define DCE interrput.
*/
//@{
#define DCE_INT_FRMST              0x00000001  ///< Interrupt for DCE frame start
#define DCE_INT_FRMEND             0x00000002  ///< Interrupt for DCE frame end
#define DCE_INT_STEND              0x00000004  ///< Interrupt for DCE stripe end
#define DCE_INT_STPERR             0x00000008  ///< Interrupt for DCE stripe error
#define DCE_INT_LBOVF              0x00000010  ///< Interrupt for DCE line buffer overflow error
#define DCE_INT_STPOB              0x00000040  ///< Interrupt for DCE stripe boundary overflow error
#define DCE_INT_YBACK              0x00000080  ///< Interrupt for DCE Y coordinate backward skip error
#define DCE_INT_LLEND              0x00000100  ///< Interrupt for Linked List done
#define DCE_INT_LLERR              0x00000200  ///< Interrupt for Linked List error command
#define DCE_INT_LLERR2             0x00000400  ///< Interrupt for Linked List error in direct mdoe
#define DCE_INT_LLJOBEND           0x00000800  ///< Interrupt for Linked List job end
#define DCE_INT_FRAMEERR           0x00001000  ///< Interrupt for frame start error in direct mode
#define DCE_INT_ALL                0x00001FDF  ///< All DCE interrupt mask
//@}

/**
    @name Define constants
*/
//@{
#define IMG_MAX_STP_NUM             16  ///< maximal stripe number of DCE output image
#define PARTITION_NUM 				16  ///< stripe parameter number
#define HISTOGRAM_BIN_NUM 			128 ///< WDR histogram bin number
//@}

/**
    @name DCE enum - Driver control
*/
//@{

/**
    DCE clear flag selection
*/
typedef enum {
	DCE_FLAG_NO_CLEAR = 0,    ///< No clear flag
	DCE_FLAG_CLEAR    = 1,    ///< Clear flag
	ENUM_DUMMY4WORD(DCE_FLAG_CLEAR_SEL)
} DCE_FLAG_CLEAR_SEL;

/**
    DCE setting selection
*/
typedef enum {
	DCE_ENABLE_SEL    = 0,    ///< Enable input 1's
	DCE_DISABLE_SEL   = 1,    ///< Disable input 1's
	DCE_SET_SEL       = 2,    ///< Enable input 1's, disable input 0's
	ENUM_DUMMY4WORD(DCE_SETTING_SEL)
} DCE_SETTING_SEL;

//@}

/**
    @name DCE enum - Engine control
*/
//@{

/**
    DCE operation selection

    Select operation mode
\n Used for dce_set_op_mode().
*/
typedef enum {
	DCE_OPMODE_IFE_IME         = 0,    ///< Flow 0(RHE->IFE->DCE->IPE->IME)
	DCE_OPMODE_DCE_IME         = 1,    ///< Flow 1(DCE->IPE->IME)
	DCE_OPMODE_SIE_IME         = 2,    ///< Flow 2(SIE->RHE->IFE->DCE->IPE->IME)
	DCE_OPMODE_DCE_D2D         = 3,    ///< DCE D2D
	ENUM_DUMMY4WORD(DCE_OPMODE)
} DCE_OPMODE;

/**
    DCE stripe mode selection

    Select stripe mode, don't change the number
\n Used for dce_set_stp_mode().
*/
typedef enum {
	DCE_STPMODE_MST     = 0,  ///< DCE horizontal multiple stripe mode
	DCE_STPMODE_SST     = 1,  ///< DCE horizontal 1 stripe mode
	DCE_STPMODE_2ST     = 2,  ///< DCE horizontal 2 stripe mode
	DCE_STPMODE_3ST     = 3,  ///< DCE horizontal 3 stripe mode
	DCE_STPMODE_4ST     = 4,  ///< DCE horizontal 4 stripe mode
	DCE_STPMODE_5ST     = 5,  ///< DCE horizontal 5 stripe mode
	DCE_STPMODE_6ST     = 6,  ///< DCE horizontal 6 stripe mode
	DCE_STPMODE_7ST     = 7,  ///< DCE horizontal 7 stripe mode
	DCE_STPMODE_8ST     = 8,  ///< DCE horizontal 8 stripe mode
	DCE_STPMODE_9ST     = 9,  ///< DCE horizontal 9 stripe mode
	ENUM_DUMMY4WORD(DCE_STP_MODE)
} DCE_STP_MODE;

/**
    DCE distortion correction selection

    Select distortion correction mode
\n Used for dce_setDistortCorrSel().
*/
typedef enum {
	DCE_DCSEL_GDC_ONLY              = 0,    ///< GDC/CAC only
	DCE_DCSEL_2DLUT_ONLY            = 1,    ///< 2D lut only
	ENUM_DUMMY4WORD(DCE_DC_SEL)
} DCE_DC_SEL;

/**
    DCE D2D format selection

    Select D2D data format
\n Used for dce_set_d2d_fmt().
*/
typedef enum {
	DCE_D2D_YUV422IN_YUV422OUT    = 0,    ///< In/Out YUV 422 8 bit packed plane
	DCE_D2D_YUV420IN_YUV420OUT    = 1,    ///< In/Out YUV 420 8 bit packed plane
	DCE_D2D_BAYERIN_YUV422OUT     = 2,    ///< In Bayer 8 bit plane/Out YUV 422 8 bit packed plane
	ENUM_DUMMY4WORD(DCE_D2D_FMT)
} DCE_D2D_FMT;

/**
    DCE YUV to RGB format selection

    Select YUV to RGB format
\n Used for dce_set_yuv2rgb_fmt().
*/
typedef enum {
	FULL_YUV        = 0,    ///< Full range YUV input
	BT601_YUV       = 1,    ///< BT601 YUV input
	BT709_YUV       = 2,    ///< BT709 YUV input
	ENUM_DUMMY4WORD(YUV2RGB_FMT)
} YUV2RGB_FMT;

/**
    DCE Histogram mode selection

    Select Histogram mode
\n Used for dce_set_hist_sel().
*/
typedef enum {
	BEFORE_WDR      = 0,    ///< histogram statistics before wdr algorithm
	AFTER_WDR       = 1,    ///< histogram statistics after wdr algorithm
	ENUM_DUMMY4WORD(DCE_HIST_SEL)
} DCE_HIST_SEL;

/**
    DCE dithering mode selection

    Select dithering mode
\n
*/
typedef enum {
	DCE_WDR_ROUNDING        = 0,  ///< rounding
	DCE_WDR_HALFTONING      = 1,  ///< halftone rounding
	DCE_WDR_RANDOMLSB       = 2,  ///< random rounding
	ENUM_DUMMY4WORD(DCE_WDR_DITHER_MODE)
} DCE_WDR_DITHER_MODE;


/**
    DCE engine status

    Engine busy or idle status
\n Used for dce_set_d2d_fmt().
*/
typedef enum {
	DCE_BUSY    = 0,    ///< Engine is busy
	DCE_IDLE    = 1,    ///< Engine is idle
	ENUM_DUMMY4WORD(DCE_STATUS)
} DCE_STATUS;

/**
    DCE H stripe input start selection

    Select input start position control
\n Used for dce_setHstripein_start().
*/
typedef enum {
	DCE_1ST_STP_POS_CALC    = 0,    ///< H,V Input starts from DCE calculated position
	DCE_1ST_STP_POS_ZERO    = 1,    ///< H,V Input starts from 0
	ENUM_DUMMY4WORD(DCE_STP_INST)
} DCE_STP_INST;

/**
    DCE control IME H stripe output overlap selection

    Select output overlap pixel number
\n Used for dce_setImeHStripeout_ovlp().
*/
typedef enum {
	DCE_HSTP_IMEOLAP_16    = 0,    ///< 16 pixels overlap
	DCE_HSTP_IMEOLAP_24    = 1,    ///< 24 pixels overlap
	DCE_HSTP_IMEOLAP_32    = 2,    ///< 32 pixels overlap
	DCE_HSTP_IMEOLAP_USER  = 3,    ///< user-defined value
	ENUM_DUMMY4WORD(DCE_HSTP_IMEOLAP_SEL)
} DCE_HSTP_IMEOLAP_SEL;

/**
    DCE control IPE H stripe output overlap selection

    Select output overlap pixel number
\n Used for dce_setIpeHstripeout_ovlp().
*/
typedef enum {
	DCE_HSTP_IPEOLAP_8     = 0,    ///< 4 pixels overlap
	DCE_HSTP_IPEOLAP_16    = 1,    ///< 8 pixels overlap
	ENUM_DUMMY4WORD(DCE_HSTP_IPEOLAP_SEL)
} DCE_HSTP_IPEOLAP_SEL;

//@}

/**
    @name DCE enum - Engine parameters
*/
//@{

/**
    DCE raw data format

    Select sensor raw data format.
*/
typedef enum {
	DCE_BAYER_2X2 = 0,         ///< Bayer 2x2: R, Gr, Gb, B
	DCE_RGBIR_4X4 = 1,         ///< RGBIR 4x4: R, Gr, Gir1, Ir1, B, Gb, Gir2, Ir2
	ENUM_DUMMY4WORD(DCE_RAW_FMT)
} DCE_RAW_FMT;

/**
    CFA data start selection

    Select intput RGBIR raw data start channel - R, Gr, Gb, B
    0:R  1:Gr
    2:Gb 3:B
    Select intput RGBIR raw data start channel - R, Gr, Gir1, Ir1, B, Gb, Gir2, Ir2
    0:R    1:Gr   4:B    5:Gb
    2:Gir1 3:Ir1  6:Gir2 7:Ir2
\n Used for dce_setCFA().
*/
typedef enum {
	DCE_CFA_PATR        = 0,        ///< Start channel is R for input Bayer/RGBIR raw data
	DCE_CFA_PATGR       = 1,        ///< Start channel is Gr for input Bayer/RGBIR raw data
	DCE_CFA_PATGB       = 2,        ///< Start channel is Gir1 for input Bayer raw data
	DCE_CFA_PAT_IR_GIR1 = 2,        ///< Start channel is Gir1 for input RGBIR raw data
	DCE_CFA_PATB        = 3,        ///< Start channel is Ir1 for input Bayer raw data
	DCE_CFA_PAT_IR_IR1  = 3,        ///< Start channel is Ir1 for input RGBIR raw data
	DCE_CFA_PAT_IR_B    = 4,        ///< Start channel is B for input RGBIR raw data
	DCE_CFA_PAT_IR_GB   = 5,        ///< Start channel is Gb for input RGBIR raw data
	DCE_CFA_PAT_IR_GIR2 = 6,        ///< Start channel is Gir2 for input RGBIR raw data
	DCE_CFA_PAT_IR_IR2  = 7,        ///< Start channel is Ir2 for input RGBIR raw data
	ENUM_DUMMY4WORD(DCE_CFA_PAT)
} DCE_CFA_PAT;

/**
    DCE Cfa subout channel selection

    Select Cfa subout channel.

    cfa pattern :  0 1
                   2 3
*/
typedef enum {
	DCE_CFA_SUBOUT_CH0 = 0,         ///< Select channel 0 as output channel
	DCE_CFA_SUBOUT_CH1 = 1,         ///< Select channel 1 as output channel
	DCE_CFA_SUBOUT_CH2 = 2,         ///< Select channel 2 as output channel
	DCE_CFA_SUBOUT_CH3 = 3,         ///< Select channel 3 as output channel
	ENUM_DUMMY4WORD(DCE_CFA_SUBOUT_CH_SEL)
} DCE_CFA_SUBOUT_CH_SEL;

/**
    DCE Cfa subout format selection

    Select Cfa subout format.
*/
typedef enum {
	DCE_CFA_SUBOUT_1BYTE = 0,       ///< Output 1 byte per pixel for CFA subout
	DCE_CFA_SUBOUT_2BYTE = 1,       ///< Output 2 byte per pixel for CFA subout
	ENUM_DUMMY4WORD(DCE_CFA_SUBOUT_BYTE)
} DCE_CFA_SUBOUT_BYTE;

/**
    DCE CAC method selection

    Select CAC method.
\n Used for dce_set_cac_sel().
*/
typedef enum {
	DCE_CAC_RGB_GAIN = 0,           ///< Correct CA with G LUT and R, G, B LUTGAIN
	DCE_CAC_RGB_LUT  = 1,           ///< Correct CA with R, G, B LUT
	ENUM_DUMMY4WORD(DCE_CAC_SEL)
} DCE_CAC_SEL;

/**
    DCE FOV boundary selection

    Select FOV boundary.
\n Used for dce_set_fov_bound_sel().
*/
typedef enum {
	DCE_FOV_BOUND_DUPLICATE = 0,    ///< Replace out of boundary pixels with duplicate boundary pixels
	DCE_FOV_BOUND_REG_RGB   = 1,    ///< Replace out of boundary pixels with GEO_BOUNDR, GEO_BOUNDG, GEO_BOUNDB
	ENUM_DUMMY4WORD(DCE_FOV_SEL)
} DCE_FOV_SEL;

/**
    DCE 2D lut number selection

    Select 2D lut number.
\n Used for dce_set2DlutNum().
*/
typedef enum {
	DCE_2DLUT_65_65_GRID = 0,         ///< 65x65 grid 2D lut distortion correction
	DCE_2DLUT_33_33_GRID = 1,         ///< 33x33 grid 2D lut distortion correction
	DCE_2DLUT_17_17_GRID = 2,         ///< 17x17 grid 2D lut distortion correction
	DCE_2DLUT_9_9_GRID   = 3,         ///< 9x9 grid 2D lut distortion correction
	ENUM_DUMMY4WORD(DCE_2DLUT_NUM)
} DCE_2DLUT_NUM;

//@}

/**
    @name DCE enum - DMA burst length
*/
//@{
typedef enum {
	DCE_BURST_32W = 0,   ///< 32 word DMA burst
	DCE_BURST_16W = 1,   ///< 16 word DMA burst
	ENUM_DUMMY4WORD(DCE_DMA_BURST)
} DCE_DMA_BURST;
//@}

/**
    @name DCE enum - Color gain range
*/
//@{
typedef enum {
	DCE_CGAIN_2_8 = 0,   ///< gain range integer/fraction 2.8
	DCE_CGAIN_3_7 = 1,   ///< gain range integer/fraction 3.7
	ENUM_DUMMY4WORD(DCE_CGAIN_RANGE)
} DCE_CGAIN_RANGE;
//@}

/**
    @name DCE enum - Pink reduction mode
*/
//@{
typedef enum {
	DCE_PINKR_MOD_G  = 0,   ///< modify G channel
	DCE_PINKR_MOD_RB = 1,   ///< modify RB channel
	ENUM_DUMMY4WORD(DCE_PINKR_MODE)
} DCE_PINKR_MODE;
//@}

/**
    @name DCE enum - GDC mode
*/
//@{
typedef enum {
	XY_BOTH = 0,        ///< processing both X and Y
	X_ONLY  = 1,        ///< processing X only
	ENUM_DUMMY4WORD(DCE_GDC_LUT_MODE)
} DCE_GDC_LUT_MODE;
//@}

/**
    @name DCE enum - SRAM clock source
*/
//@{
typedef enum {
	DCE_SRAM_CLK_DCE = 0,     ///< DCE has SRAM control
	DCE_SRAM_CLK_PCLK = 1,    ///< CPU has SRAM control
	ENUM_DUMMY4WORD(DCE_SRAM_CLK_SEL)
} DCE_SRAM_CLK_SEL;
//@}

/**
    @name DCE enum - SRAM clock source
*/
//@{
typedef enum {
	DCE_SRAM_DCE = 0,     ///< DCE has SRAM control
	DCE_SRAM_CNN = 1,    ///< CPU has SRAM control
	ENUM_DUMMY4WORD(DCE_SRAM_SEL)
} DCE_SRAM_SEL;
//@}

/**
    DCE dram single output mode selection

    Select dram single output mode
*/
typedef enum {
	DCE_DRAM_OUT_NORMAL            = 0,    ///<
	DCE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(DCE_DRAM_OUTPUT_MODE)
} DCE_DRAM_OUTPUT_MODE;


/**
    DCE structure - DMA burst length
*/
//@{
typedef struct {
	DCE_DMA_BURST   in_burst_len;  ///< Input burst length
	DCE_DMA_BURST   out_burst_len; ///< Output burst length
} DCE_BURST_LENGTH;
//@}

/**
    DCE parameter mode
*/
typedef enum {
	DCE_PARAM_AUTO_MODE  = 0,   ///< parameters using auto mode
	DCE_PARAM_USER_MODE  = 1,   ///< parameters using user mode
	ENUM_DUMMY4WORD(DCE_PARAM_MODE)
} DCE_PARAM_MODE;

/**
    DCE input blending source selection
*/
typedef enum {
	INPUT_BLD_3x3_Y  = 0,   ///< use Y from bayer 3x3 as the input blending source selection
	INPUT_BLD_GMEAN  = 1,   ///< use Gmean from bayer 3x3 as the input blending source selection
	INPUT_BLD_BAYER  = 2,   ///< use Bayer as the input blending source selection
	ENUM_DUMMY4WORD(DCE_WDR_INPUT_BLDSEL)
} DCE_WDR_INPUT_BLDSEL;


/**
    DCE structure - Image size
*/
//@{
typedef struct {
	UINT32    h_size;    ///< Horizontal image size, unit : pix
	UINT32    v_size;    ///< Vertical image size, uint : line
} DCE_IMG_SIZE;
//@}

/**
    DCE structure - Image ratio
*/
//@{
typedef struct {
	UINT32    x_dist;    ///< Horizontal distance ratio
	UINT32    y_dist;    ///< Vertical distance ratio
} DCE_IMG_RAT;
//@}

/**
    DCE structure - Optical center
*/
//@{
typedef struct {
	INT32    x_cent;    ///< Horizontal center, unit : pix
	INT32    y_cent;    ///< Vertical center, uint : line
} DCE_IMG_CENT;
//@}

/**
    DCE structure - Distance normalization
*/
//@{
typedef struct {
	UINT32    nrm_fact_sel;///< select different factor bit number
	UINT32    nrm_fact;    ///< Normalization factor
	UINT32    nrm_fact_10b;///< Normalization factor 10 bit
	UINT32    nrm_bit;     ///< Normalization bit
} DCE_DIST_NORM;
//@}

/**
    DCE structure - CAC gain
*/
//@{
typedef struct {
	UINT32    r_lut_gain;    ///< R channel lut gain
	UINT32    g_lut_gain;    ///< G channel lut gain
	UINT32    b_lut_gain;    ///< B channel lut gain
} DCE_CAC_GAIN;
//@}

/**
    DCE structure - FOV bound RGB
*/
//@{
typedef struct {
	UINT32    r_bound;    ///< R boundary value
	UINT32    g_bound;    ///< G boundary value
	UINT32    b_bound;    ///< B boundary value
} DCE_FOV_RGB;
//@}

/**
    DCE structure - H,V factor
*/
typedef struct {
	UINT32 factor_h;    ///< horizontal factor
	UINT32 factor_v;    ///< vertical factor
} DCE_HV_FACTOR;

/**
    DCE structure - WDR non-equal table parameters
*/
typedef struct {
	UINT32 *p_lut_idx;
	UINT32 *p_lut_split;
	UINT32 *p_lut_val;
} WDR_NEQ_TABLE;

/**
    DCE structure - WDR input blending parameters
*/
typedef struct {
	DCE_WDR_INPUT_BLDSEL wdr_bld_sel;
	UINT32 wdr_bld_wt;
	UINT32 *p_inblend_lut;
} WDR_IN_BLD_PARAM;

/**
    DCE structure - WDR strength parameters
*/
typedef struct {
	INT32 *p_wdr_coeff;
	UINT32 strength;
	UINT32 contrast;
} WDR_STRENGTH_PARAM;

/**
    DCE structure - WDR gain control parameters
*/
typedef struct {
	BOOL gainctrl_en;
	UINT32 max_gain;
	UINT32 min_gain;
} WDR_GAINCTRL_PARAM;

/**
    DCE structure - WDR output blending parameters
*/
typedef struct {
	BOOL outbld_en;
	WDR_NEQ_TABLE outbld_lut;
} WDR_OUTBLD_PARAM;

/**
    DCE structure - WDR saturation reduction parameters
*/
typedef struct {
	UINT32 sat_th;
	UINT32 sat_wt_low;
	UINT32 sat_delta;
} WDR_SAT_REDUCT_PARAM;


/**
    DCE structure - CFA false color suppression term
*/
//@{
typedef struct {
	BOOL      fcs_dirsel;       ///< Direction selection for detecting high frequency, 0: detect 2 directions, 1: detect 4 directions
	UINT32    fcs_coring;       ///< Coring threshold for false color suppression
	UINT32    fcs_weight;       ///< Global strength for false color suppression
	UINT32    *p_fcs_strength;  ///< CFA false color suppression strength
} DCE_CFA_FCS;
//@}


/**
    DCE structure - CFA interpolation term
*/
//@{
typedef struct {
	UINT32    edge_dth;     ///< Edge threshold 1 for Bayer CFA interpolation
	UINT32    edge_dth2;    ///< Edge threshold 2 for Bayer CFA interpolation
	UINT32    freq_th;      ///<
	UINT32    *p_freq_lut;  ///< CFA frequency blending weight look-up table
	UINT32    *p_luma_wt;   ///< Luma gain for CFA
} DCE_CFA_INTERP;
//@}

/**
    DCE structure - CFA correction term
*/
//@{
typedef struct {
	BOOL      rb_corr_en;    ///< Enable for RB channel correction
	UINT32    rb_corr_th1;  ///< RB correction noise threshold1
	UINT32    rb_corr_th2;  ///< RB correction noise threshold2
} DCE_CFA_CORR;
//@}

/**
    DCE structure - CFA high frequency check term
*/
//@{
typedef struct {
	BOOL      ir_cl_check_en;           ///< Enable cross line check
	BOOL      ir_hf_check_en;           ///< Enable high frequency check
	BOOL      ir_average_mode;      ///< Interpolation method selection when flat region detect
	BOOL      ir_cl_sel;            ///< operation type when cross line is not found
	UINT32    ir_cl_th;             ///< Cross line checking threshold
	UINT32    ir_hf_gth;            ///< G channel smoothness checking threshold
	UINT32    ir_hf_diff;           ///< Difference threshold value between Gr and Gb to determing high frequency pattern detected or not
	UINT32    ir_hf_eth;            ///< Threshold to determine H or V direction high frequency pattern detected
	UINT32    ir_g_edge_th;         ///< Noise margin for edge in Green component interpolation
	UINT32    ir_rb_cstrength;      ///< RB channel smooth strength
} DCE_CFA_IR_HFC;
//@}

/**
    DCE structure - CFA IR subtraction term
*/
//@{
typedef struct {
	UINT32    ir_sub_r;         ///< R channel IR sub.
	UINT32    ir_sub_g;         ///< G channel IR sub.
	UINT32    ir_sub_b;         ///< B channel IR sub.
	UINT32    ir_sub_wt_lb;     ///< Lower bound of IR local weighting
	UINT32    ir_sub_th;        ///< Threshold to decay IR local weighting
	UINT32    ir_sub_range;     ///< Range to decay IR local weighting
	UINT32    ir_sat_gain;      ///< Saturation gain multiplied after IR sub
} DCE_CFA_IR;
//@}

/**
    DCE structure - CFA color gain term
*/
//@{
typedef struct {
	UINT32    gain_r;          ///< R channel gain
	UINT32    gain_g;          ///< G channel gain
	UINT32    gain_b;          ///< B channel gain
	DCE_CGAIN_RANGE gain_range;  ///< select fraction type of gain: 2.8 or 3.7
} DCE_CFA_CGAIN;
//@}

/**
    DCE structure - CFA pink reduction term
*/
//@{
typedef struct {
	DCE_PINKR_MODE pink_rd_mode;       ///< pink reduction mode
	UINT32    pink_rd_th1;          ///< pink reduction threshold 1
	UINT32    pink_rd_th2;          ///< pink reduction threshold 2
	UINT32    pink_rd_th3;          ///< pink reduction threshold 3
	UINT32    pink_rd_th4;          ///< pink reduction threshold 4
} DCE_CFA_PINKR;
//@}

/**
    DCE structure - H stripe mas increment, decrement
*/
//@{
typedef struct {
	UINT32    max_inc;    ///< Max increment
	UINT32    max_dec;    ///< Max decrement
} DCE_HSTP_INCDEC;
//@}

/**
    DCE structure - Output cropping
*/
//@{
typedef struct {
	UINT32    h_size;    ///< Horizontal image size, unit : pix
	UINT32    v_size;    ///< Vertical image size, uint : line
	UINT32    h_start;    ///< Horizontal image size, unit : pix
	UINT32    v_start;    ///< Vertical image size, uint : line
} DCE_OUT_CROP;
//@}

/**
    DCE structure - DCE operation parameters.
*/
//@{
typedef struct {
	DCE_OPMODE op_mode;          ///< Operation mode selection
	DCE_STP_MODE stp_mode;       ///< Stripe mode selection
} DCE_OP_PARAM;
//@}

/**
    DCE structure - DCE WDR parameters.
*/
//@{
typedef struct {
	BOOL wdr_rand_rst;
	DCE_WDR_DITHER_MODE wdr_rand_sel;
} DCE_WDR_DITHER;
//@}

/**
    DCE structure - DCE WDR parameters.
*/
//@{
typedef struct {
	UINT32                  *p_ftrcoef;             ///< wdr sub-image low pass filter coefficients, 3 entries
	WDR_IN_BLD_PARAM        wdr_input_bld;
	WDR_NEQ_TABLE           wdr_tonecurve;
	WDR_STRENGTH_PARAM      wdr_strength;
	WDR_GAINCTRL_PARAM      wdr_gainctrl;
	WDR_OUTBLD_PARAM        wdr_outbld;
	WDR_SAT_REDUCT_PARAM    wdr_sat_reduct;
} DCE_WDR_PARAM;
//@}

/**
    DCE structure - DCE WDR subout parameters.
*/
//@{
typedef struct {

	DCE_PARAM_MODE      subimg_param_mode;          ///< wdr sub-image scaling parameter mode

	DCE_IMG_SIZE        input_size;                 ///< Input image H V size,
	DCE_IMG_SIZE        wdr_subimg_size;            ///< wdr sub-image size
	DCE_HV_FACTOR       subin_scaling_factor;       ///< wdr sub-image scaling factor
	DCE_IMG_SIZE        subout_blksize;             ///< wdr sub-image output block size
	DCE_HV_FACTOR       subout_blk_centfact;        ///< wdr sub-image output block center factor

} DCE_WDR_SUBOUT_PARAM;
//@}

/**
    DCE structure - DCE Histogram parameters.
*/
//@{
typedef struct {
	DCE_HIST_SEL hist_sel;
	UINT32 hist_step_h;
	UINT32 hist_step_v;
	UINT32 *p_hist_stcs;
} DCE_HIST_PARAM;
//@}

/**
    DCE structure - DCE Histogram results.
*/
//@{
typedef struct {
	BOOL enable;
	DCE_HIST_SEL hist_sel;
	UINT16 hist_stcs[HISTOGRAM_BIN_NUM];
} DCE_HIST_RSLT;
//@}

/**
    DCE structure - DCE CFA parameters.
*/
//@{
typedef struct {
	DCE_RAW_FMT raw_fmt;         ///< CFA start pattern
	DCE_CFA_PAT cfa_pat;         ///< CFA start pattern
	DCE_CFA_INTERP cfa_interp;   ///< CFA interpolation term
	DCE_CFA_CORR cfa_corr;       ///< CFA correction term
	DCE_CFA_FCS cfa_fcs;            ///< CFA false color suppression term
	DCE_CFA_IR_HFC cfa_ir_hfc;         ///< CFA high frequency check term
	DCE_CFA_IR cfa_ir_sub;        ///< CFA IR subtraction term
	DCE_CFA_PINKR cfa_pink_reduc; ///< CFA pink reduction term
	DCE_CFA_CGAIN cfa_cgain;     ///< CFA color gain term
} DCE_CFA_PARAM;
//@}

/**
    DCE structure - DCE CFA Subimg output parameters.
*/
//@{
typedef struct {
	DCE_CFA_SUBOUT_CH_SEL subout_ch_sel;        ///< CFA sub-img output channel selection
	DCE_CFA_SUBOUT_BYTE subout_byte;            ///< CFA sub-img output format
	UINT32 subout_shiftbit;                     ///< CFA sub-img output right shift bit
} DCE_CFA_SUBOUT_PARAM;
//@}

/**
    DCE structure - DCE GDC/CAC digital zoom parameters.
*/
//@{
typedef struct {
	DCE_IMG_RAT img_ratio;                      ///< GDC/CAC image ratio setting
	DCE_DIST_NORM distance_norm;                ///< GDC/CAC distance normalization
	UINT32 *p_g_geo_lut;                        ///< GDC/CAC G geo lut
} DCE_GDC_CAC_DZOOM_PARAM;
//@}

/**
    DCE structure - DCE GDC/CAC optical zoom parameters.
*/
//@{
typedef struct {
	DCE_GDC_LUT_MODE gdc_mode;                  ///< process both X & Y or X only
	DCE_CAC_SEL cac_sel;                        ///< CAC compensation method selection
	DCE_CAC_GAIN cac_gain;                      ///< CAC RGB lut gain
	INT32 *p_r_cac_lut;                         ///< GDC/CAC R geo lut
	INT32 *p_b_cac_lut;                         ///< GDC/CAC B geo lut
} DCE_GDC_CAC_OZOOM_PARAM;
//@}

/**
    DCE structure - DCE FOV parameters.
*/
//@{
typedef struct {
	UINT32 fov_gain;                            ///< FOV lut gain
	DCE_FOV_SEL fov_sel;                        ///< FOV boundary select
	DCE_FOV_RGB fov_rgb_val;                    ///< FOV boundary RGB definition
} DCE_FOV_PARAM;
//@}

/**
    DCE structure - 2D lut offset
*/
//@{
typedef struct {
	UINT32    x_ofs_int;   ///< 2D lut x integer offset
	UINT32    x_ofs_frac;  ///< 2D lut x fraction offset
	UINT32    y_ofs_int;   ///< 2D lut y integer offset
	UINT32    y_ofs_frac;  ///< 2D lut y fraction offset
} DCE_2DLUT_OFS;
//@}

/**
    DCE structure - 2D lut scaling factor
*/
//@{
typedef struct {
	UINT32    h_scale_fact;  ///< 2D lut h scaling factor
	UINT32    v_scale_fact;  ///< 2D lut v scaling factor
} DCE_2DLUT_SCALE;
//@}


/**
    DCE structure - DCE 2D lut parameters.
*/
//@{
typedef struct {
	UINT32 lut2d_addr;                  ///< 2D lut DRAM address
	BOOL ymin_auto_en;
	DCE_2DLUT_OFS lut2d_xy_ofs;         ///< 2D lut x y offset
	DCE_2DLUT_NUM lut2d_num_sel;        ///< 2D lut grid number
	DCE_2DLUT_SCALE lut2d_factor;       ///< 2D lut scaling factor
} DCE_2DLUT_PARAM;
//@}

/**
    DCE structure - DCE H stripe parameters.
*/
//@{
typedef struct {
	UINT32 *p_h_stripe;                         ///< H stripe0~15 setting
	DCE_HSTP_INCDEC h_stp_inc_dec;              ///< H stripe max increment/decrement
	DCE_STP_INST h_stp_in_start;                ///< First H stripe input selection
	DCE_HSTP_IMEOLAP_SEL ime_out_hstp_ovlap;    ///< IME H stripe output overlap selection
	DCE_HSTP_IPEOLAP_SEL ipe_out_hstp_ovlap;    ///< IPE H stripe output overlap selection
	UINT32 ime_ovlap_user_val;                  ///< user-defined overlap value
} DCE_HSTP_PARAM;
//@}

/**
    DCE structure - DCE V stripe parameters.
*/
//@{
typedef struct {
	DCE_STP_INST v_stp_in_start;        ///< First V stripe input selection
} DCE_VSTP_PARAM;
//@}

/**
    DCE structure - DCE H stripe status.
*/
//@{
typedef struct {
	UINT32 h_stp_buf_mode;              ///< Last H stripe buffer mode used
	UINT32 h_stp_st_x;                  ///< Last H stripe starting input X coordinate
	UINT32 h_stp_ed_x;                  ///< Last H stripe ending input X coordinate
	UINT32 h_stp_clm_st_x;              ///< Last H stripe starting input X coordinate of the first column
	UINT32 h_stp_clm_ed_x;              ///< Last H stripe ending input X coordinate of the first column
	UINT32 buf_height_g;                ///< Last H stripe vertical buffer used by G
	UINT32 buf_height_rgb;              ///< Last H stripe vertical buffer used by RGB
	UINT32 buf_height_pix;              ///< Last H stripe vertical buffer used by RGB of single pixel
} DCE_HSTP_STATUS;
//@}

/**
    DCE structure - DCE D2D parameters.
*/
//@{
typedef struct {
	BOOL d2d_rand_en;
	BOOL d2d_rand_rest;
	DCE_D2D_FMT d2d_fmt;                ///< D2D format selection
	UINT32 yuv2rgb_fmt;                 ///< YUV to RGB format selection
	UINT32 y_in_addr;                   ///< D2D Y input address
	UINT32 y_in_lofs;                   ///< D2D Y input line offset
	UINT32 uv_in_addr;                  ///< D2D UV input address
	UINT32 uv_in_lofs;                  ///< D2D UV input line offset
	UINT32 y_out_addr;                  ///< D2D Y output address
	UINT32 y_out_lofs;                  ///< D2D Y output line offset
	UINT32 uv_out_addr;                 ///< D2D UV output address
	UINT32 uv_out_lofs;                 ///< D2D UV output line offset
} DCE_D2D_PARAM;
//@}

/**
    DCE structure - DCE Input infomation
*/
//@{
typedef struct {
	BOOL wdr_in_en;
	UINT32 wdr_subin_addr;              ///< Wdr subimg input address
	UINT32 wdr_subin_lofs;              ///< Wdr subimg input line offset
} DCE_INPUT_INFO;
//@}

/**
    DCE structure - DCE Output infomation
*/
//@{
typedef struct {
	BOOL wdr_out_en;
	BOOL cfa_out_en;
	UINT32 cfa_subout_addr;             ///< Cfa subimg output address
	UINT32 cfa_subout_lofs;             ///< Cfa subimg output line offset
	UINT32 wdr_subout_addr;             ///< Wdr subimg output address
	UINT32 wdr_subout_lofs;             ///< Wdr subimg output line offset
} DCE_OUTPUT_INFO;
//@}

/**
    DCE structure - Dram single output channel enable
*/
//@{
typedef struct {
	BOOL single_out_wdr_en;
	BOOL single_out_cfa_en;
} DCE_DRAM_SINGLE_OUT_EN;
//@}


/**
    DCE structure - Engine open object
*/
//@{
typedef struct {
	void (*FP_DCEISR_CB)(UINT32 int_status);   ///< External ISR call-back function
	UINT32 dce_clock_sel;                       ///< Engine clock selection
} DCE_OPENOBJ;
//@}

/**
    DCE structure - DCE mode info.
*/
//@{
typedef struct {
	UINT32 int_en_msk;                          ///< Interrupt enable selection
	UINT32 func_en_msk;                         ///< Function enable selection
	DCE_OP_PARAM op_param;                      ///< Engine operation parameters
	DCE_SRAM_SEL sram_sel;                      //< Engine sram selection
	DCE_IMG_SIZE img_in_size;                   ///< Input image size
	DCE_OUT_CROP img_out_crop;                  ///< Output image cropping
	DCE_INPUT_INFO input_info;
	DCE_OUTPUT_INFO output_info;
	DCE_DRAM_OUTPUT_MODE dram_out_mode;         ///< DCE DRAM output mode selection, 0: normal, 1: single out mode
	DCE_DRAM_SINGLE_OUT_EN single_out_en;       ///< DCE DRAM single output channel enable
	DCE_WDR_DITHER wdr_dither;                  ///<
	DCE_WDR_PARAM wdr_param;
	DCE_WDR_SUBOUT_PARAM wdr_subimg_param;
	DCE_HIST_PARAM hist_param;
	DCE_CFA_PARAM cfa_param;                    ///< CFA interpolation parameters
	DCE_CFA_SUBOUT_PARAM cfa_subout_param;      ///< CFA interpolation parameters
	DCE_DC_SEL dc_sel;                          ///< Distortion correction combination selection
	DCE_IMG_CENT img_cent;                      ///< GDC/CAC center setting
	DCE_GDC_CAC_DZOOM_PARAM gdc_cac_dz_param;   ///< GDC/CAC digital zoom parameters
	DCE_GDC_CAC_OZOOM_PARAM gdc_cac_oz_param;   ///< GDC/CAC optical zoom parameters
	DCE_FOV_PARAM fov_param;                    ///< FOV parameters
	DCE_2DLUT_PARAM lut2d_param;                ///< 2D lut parameters
	DCE_HSTP_PARAM h_stp_param;                 ///< H stripe parameters
	DCE_D2D_PARAM d2d_param;                    ///< D2D parameters
	UINT32 back_rsv_line;                       ///< back reserve line number, assign 0 when running IPP flow
} DCE_MODE_INFO;
//@}

/**
    DCE structure - DCE stripe info.
*/
//@{
typedef struct {
	DCE_STP_MODE stp_mode;           ///< Stripe mode
	UINT32 stp_num;					///< Stripe number defined by user
	DCE_SRAM_SEL sram_sel;           ///< Engine sram selection
	UINT32 in_width;                 ///< Input width
	UINT32 in_height;                ///< Input height
	UINT32 ipe_h_ovlap;              ///< IPE Output H overlap
	UINT32 ime_h_ovlap;              ///< IME Output H overlap
	BOOL crop_en;                   ///< Crop function enable
	DCE_OUT_CROP crop_info;          ///< Crop information
	BOOL ime_isd_en;                 ///< IME ISD function enable
	BOOL ime_enc_en;                 ///< enable bit of IME encode
	BOOL ime_3dnr_en;                ///< enable bit of IME 3DNR
	BOOL ime_dec_en;                 ///< enable bit of IME decode
	BOOL ime_ref_out_en;              ///< enable bit of IME reference image out
	BOOL ime_p1_enc_en;               ///< enable bit of IME path1 encoder out
} DCE_NORMSTP_INFO;
//@}

/**
    DCE structure - DCE stripe result.
*/
//@{
typedef struct {
	UINT32 gdc_h_stp[PARTITION_NUM];                       ///< H stripe settings
	UINT32 max_inc;                             ///< Max increment
	UINT32 max_dec;                             ///< Max decrement
	UINT32 ui_hstp_num;                  ///< H stripe number
	UINT16 ui_dce_out_hstp[IMG_MAX_STP_NUM];     ///< H stripe settings of DCE output image
	UINT16 ui_ime_in_hstp[IMG_MAX_STP_NUM];     ///< H stripe settings of IME input
	UINT16 ui_ime_out_hstp[IMG_MAX_STP_NUM];    ///< H stripe settings of IME output
} DCE_NORMSTP_RSLT;
//@}

typedef struct {
	UINT32 frm_start_cnt;
	UINT32 frm_end_cnt;
	UINT32 strp_end_cnt;
	UINT32 strp_err_cnt;
	UINT32 lbuf_ovfl_cnt;
	UINT32 strp_out_of_bnd_cnt;
	UINT32 y_back_cnt;
	UINT32 dce_ll_end_cnt;
	UINT32 dce_ll_jobend_cnt;
	UINT32 dce_ll_err_cnt;
	UINT32 dce_ll_err2_cnt;
} DCE_INT_STATUS_CNT;

extern DCE_INT_STATUS_CNT dce_int_stat_cnt;
extern BOOL fw_power_saving_en;

//----------------------------------------------------------------------------------------//
extern ER dce_get_norm_stripe(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result, DCE_MODE_INFO *dce_mode_info);
extern ER dce_cal_simple_stripe_settings(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result);
extern VOID dce_get_output_stp_setting(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result);

extern ER dce_open(DCE_OPENOBJ *p_open_info);
extern ER dce_close(void);
extern ER dce_start_linkedlist(void);
extern ER dce_terminate_linkedlist(void);
extern ER dce_start(void);
extern ER dce_start_fend_load(void);
extern ER dce_pause(void);
extern VOID dce_set_builtin_flow_disable(VOID);
extern ER dce_reset(BOOL sw_rst_en);
extern ER dce_set_mode(DCE_MODE_INFO *p_mode_info);
//extern ER dce_change_all(DCE_MODE_INFO *p_mode_info);
extern ER dce_change_int_en(UINT32 int_sel, DCE_SETTING_SEL setting_sel);
extern ER dce_change_func_en(UINT32 func_sel, DCE_SETTING_SEL setting_sel);
extern ER dce_change_operation(DCE_OP_PARAM *p_in_param);
extern ER dce_change_img_in_size(DCE_IMG_SIZE *p_in_param);
extern ER dce_change_img_out_crop(DCE_OUT_CROP *p_in_param);
extern ER dce_change_output_info(DCE_OUTPUT_INFO *p_in_param);
extern ER dce_change_input_info(DCE_INPUT_INFO *p_in_param);
extern ER dce_change_cfa_interp(DCE_CFA_PARAM *p_in_param);
extern ER dce_change_cfa_subout(DCE_CFA_SUBOUT_PARAM *p_in_param);
extern ER dce_change_wdr(BOOL wdr_en, BOOL tonecurve_en, DCE_WDR_PARAM *p_in_param);
extern ER dce_change_wdr_subout(DCE_WDR_SUBOUT_PARAM *p_in_param);
extern ER dce_change_histogram(DCE_HIST_PARAM *p_in_param);
extern ER dce_change_wdr_dither(DCE_WDR_DITHER *p_in_param);
extern ER dce_change_gdc_mode(DCE_GDC_LUT_MODE gdc_mode);
extern ER dce_change_2dlut(DCE_2DLUT_PARAM *p_in_param, DCE_IMG_SIZE *in_img_sz);
extern ER dce_change_distort_corr_sel(DCE_DC_SEL dcsel);
extern ER dce_change_img_center(DCE_IMG_CENT *p_in_param);
extern ER dce_change_gdc_cac_digi_zoom(DCE_GDC_CAC_DZOOM_PARAM *p_in_param);
extern ER dce_change_gdc_cac_opti_zoom(DCE_GDC_CAC_OZOOM_PARAM *p_in_param);
extern ER dce_change_fov(DCE_FOV_PARAM *p_in_param);
extern ER dce_change_h_stripe(DCE_HSTP_PARAM *p_in_param);
extern ER dce_change_d2d_in_out(DCE_D2D_PARAM *p_in_param, DCE_OPMODE op_mode);
extern ER dce_change_sram_mode(DCE_SRAM_SEL ui_sram_mode);
extern void dce_set_raw_fmt(DCE_RAW_FMT raw_fmt);
extern void dce_set_cfa_pat(DCE_CFA_PAT cfa_pat);
extern void dce_get_hist_rslt(DCE_HIST_RSLT *p_result);
extern ER dce_change_dram_out_mode(DCE_DRAM_OUTPUT_MODE outmode);
extern ER dce_change_dram_single_ch_en(DCE_DRAM_SINGLE_OUT_EN *ch_en);
extern void dce_flush_cache(DCE_MODE_INFO *p_mode_info);

extern UINT32 dce_get_clock_rate(void);
extern UINT32 dce_get_int_en(void);
extern UINT32 dce_get_func_en(void);
extern void dce_get_ir_sub(DCE_CFA_IR *p_ir_sub);
extern void dce_get_img_size(DCE_IMG_SIZE *p_size);
extern DCE_CFA_PAT dce_get_cfa_pat(void);
extern DCE_STATUS dce_get_busy_status(void);
extern UINT32 dce_get_int_status(void);
extern UINT32 dce_get_h_stp_cnt(void);
extern void dce_get_h_stripe(DCE_HSTP_PARAM *p_hstp);
extern void dce_get_h_stp_status(DCE_HSTP_STATUS *p_hstp_status);
extern void dce_get_burst_length(DCE_BURST_LENGTH *p_burst_len);

extern void dce_isr(void);
extern void dce_clear_int(UINT32 intr);
extern void dce_wait_flag_frame_end(DCE_FLAG_CLEAR_SEL flag_clear);
extern void dce_clear_flag_frame_end(void);
extern void dce_wait_flag_frame_start(DCE_FLAG_CLEAR_SEL flag_clear);
extern void dce_clear_flag_frame_start(void);
extern void dce_wait_flag_ll_done(DCE_FLAG_CLEAR_SEL flag_clear);
extern void dce_clear_flag_ll_done(void);
extern void dce_wait_flag_ll_job_end(DCE_FLAG_CLEAR_SEL flag_clear);
extern void dce_clear_flag_ll_job_end(void);

extern UINT32 g_frm_start_cnt;
extern UINT32 g_frm_end_cnt;
extern UINT32 g_strp_end_cnt;
extern UINT32 g_strp_err_cnt;
extern UINT32 g_lbuf_ovfl_cnt;
extern UINT32 g_strp_out_of_bnd_cnt;
extern UINT32 g_y_back_cnt;
extern UINT32 g_dce_ll_end_cnt;
extern UINT32 g_dce_ll_jobend_cnt;
extern UINT32 g_dce_ll_err_cnt;
extern UINT32 g_dce_ll_err2_cnt;
#if defined (_NVT_EMULATION_)
extern BOOL dce_end_time_out_status;
//For FPGA verify only
extern ER dce_start_without_load(void);
#endif
#ifdef __cplusplus
}
#endif

#ifdef __KERNEL__
extern void dce_create_resource(void);
extern void dce_release_resource(void);
extern void dce_set_base_addr(UINT32 uiAddr);
#endif
//@}
#endif
