/**
    Public header file for kdrv_ipe

    This file is the header file that define the API and data type for kdrv_ipe.

    @file       kdrv_ipe.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/
#ifndef _KDRV_IPE_H_
#define _KDRV_IPE_H_


#ifdef __KERNEL__
#include "kwrap/type.h"
#endif

#define KDRV_IPE_EDGE_KER_DIV_LEN  12
#define KDRV_IPE_EDGE_DIR_TAB_LEN   8
#define KDRV_IPE_TONE_MAP_LUT_LEN  65
#define KDRV_IPE_EDGE_MAP_LUT_LEN  16
#define KDRV_IPE_ES_MAP_LUT_LEN    16
#define KDRV_IPE_SUBIMG_FILT_LEN   3
#define KDRV_IPE_COEF_LEN          9  // For CCParam.coef/CSTCoef
#define KDRV_IPE_FTAB_LEN          16  // For fstab/fdtab
#define KDRV_IPE_CCTRL_TAB_LEN     24  // For Hue/Sat/Int/Edg
#define KDRV_IPE_DDS_TAB_LEN       8
#define KDRV_IPE_YCURVE_LEN        129
#define KDRV_IPE_GAMMA_LEN         129
#define KDRV_IPE_3DCC_LEN          900
#define KDRV_IPE_PFR_SET_NUM       4
#define KDRV_IPE_PFR_COLOR_LEN     5
#define KDRV_IPE_PFR_LUMA_LEN      13
#define KDRV_IPE_CCONTAB_LEN       17
#define KDRV_DFG_INPUT_BLD_LEN     9
#define KDRV_DFG_INTERP_DIFF_LEN   17
#define KDRV_DFG_OUTPUT_BLD_LEN    17
#define KDRV_DFG_FOG_MOD_LEN       17
#define KDRV_DFG_TARGET_LEN        9
#define KDRV_DFG_AIRLIGHT_NUM      3
#define KDRV_LCE_LUMA_LEN          9
#define KDRV_IPE_VA_MAX_WINNUM     8
#define KDRV_IPE_VA_INDEP_NUM      5
#define KDRV_IPE_VA_INDEP_WIN_MAX  511

/**
    type for KDRV_IPE_SET_ISRCB
*/
typedef enum {
	KDRV_IPE_INT_FMD             = 0x00000002,
	KDRV_IPE_INT_STP             = 0x00000004,
	KDRV_IPE_INT_FMS             = 0x00000008,
	KDRV_IPE_INT_YCC_OUT_END     = 0x00000010,
	KDRV_IPE_INT_GAMMA_IN_END    = 0x00000020,
	KDRV_IPE_INT_DEFOG_IN_END    = 0x00000040,
	KDRV_IPE_INT_VA_OUT_END      = 0x00000080,
	KDRV_IPE_INT_LL_DONE         = 0x00000100,
	KDRV_IPE_INT_LL_JOBEND       = 0x00000200,
	KDRV_IPE_INT_LL_ERR          = 0x00000400,
	KDRV_IPE_INT_LL_ERR2         = 0x00000800,
	KDRV_IPE_INT_FRAMEERR        = 0x00001000,
	KDRV_IPE_INT_ALL             = 0x00001FFE,
	ENUM_DUMMY4WORD(KDRV_IPE_ISR_STATUS)
} KDRV_IPE_ISR_STATUS;

/**
    IPE KDRV interrupt enable enum.
*/
typedef enum {
	KDRV_IPE_INTE_CLR            = 0x00000000,
	KDRV_IPE_INTE_FMD            = KDRV_IPE_INT_FMD,            ///< enable interrupt: frame done
	KDRV_IPE_INTE_STP            = KDRV_IPE_INT_STP,            ///< enable interrupt: current stripe done
	KDRV_IPE_INTE_FMS            = KDRV_IPE_INT_FMS,            ///< enable interrupt: frame start
	KDRV_IPE_INTE_YCC_OUT_END    = KDRV_IPE_INT_YCC_OUT_END,    ///< enable interrupt: YCC DRAM output done
	KDRV_IPE_INTE_GAMMA_IN_END   = KDRV_IPE_INT_GAMMA_IN_END,   ///< enable interrupt: DRAM input done (gamma)
	KDRV_IPE_INTE_DEFOG_IN_END   = KDRV_IPE_INT_DEFOG_IN_END,   ///< enable interrupt: DRAM input done (defog)
	KDRV_IPE_INTE_VA_OUT_END     = KDRV_IPE_INT_VA_OUT_END,     ///< enable interrupt: DRAM VA output done
	KDRV_IPE_INTE_LL_DONE        = KDRV_IPE_INT_LL_DONE,        ///< enable interrupt: Linked List done
	KDRV_IPE_INTE_LL_JOBEND      = KDRV_IPE_INT_LL_JOBEND,      ///< enable interrupt: Linked List job end
	KDRV_IPE_INTE_LL_ERR         = KDRV_IPE_INT_LL_ERR,         ///< enable interrupt: Linked List error command
	KDRV_IPE_INTE_LL_ERR2        = KDRV_IPE_INT_LL_ERR2,        ///< enable interrupt: Linked List error in direct mode
	KDRV_IPE_INTE_FRAMEERR       = KDRV_IPE_INT_FRAMEERR,       ///< enable interrupt: frame start error in direct mode
	KDRV_IPE_INTE_ALL            = KDRV_IPE_INT_ALL,            ///< enable all interrupt
} KDRV_IPE_INTE;

typedef void (*KDRV_IPE_ISRCB)(KDRV_IPE_ISR_STATUS, void *);

typedef enum {
	KDRV_IPE_IN_SRC_DIRECT,      // input direct from engine (IFE->IME)
	KDRV_IPE_IN_SRC_ALL_DIRECT,  // input direct from sensor (SIE->IME)
	KDRV_IPE_IN_SRC_DRAM,        // YUV input from dram (some functions are not supported)
	KDRV_IPE_IN_SRC_DRAM_RGB,    // RGB input from dram

	ENUM_DUMMY4WORD(KDRV_IPE_IN_SRC)
} KDRV_IPE_IN_SRC;

/**
    YUV channel index
*/
typedef enum {
	KDRV_IPE_KDRV_YUV_Y,                  ///< process channel Y
	KDRV_IPE_KDRV_YUV_UV,                 ///< process channel UV
	KDRV_IPE_KDRV_YUV_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_IPE_YUV_CH)
} KDRV_IPE_YUV_CH;

#if 0
/**
    IPE KDRV trigger type
*/
typedef struct {
	BOOL wait_end;          ///< wait IPE frame end
	UINT32 time_out_ms;     ///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_IPE_TRIG_TYPE;

#endif

/**
    channel information
*/
typedef struct {
	UINT32 h_size;             ///< image width(pixel)
	UINT32 v_size;            ///< image height(pixel)
} KDRV_IPE_SIZE;


/**
    channel information
*/
typedef struct {
	UINT32 y_width;             ///< image width(pixel)
	UINT32 y_height;            ///< image height(pixel)
	UINT32 y_line_ofs;          ///< image y channel line offset(byte)
	UINT32 uv_line_ofs;         ///< image uv channel line offset(byte)
} KDRV_IPE_IN_CH_INFO;

// structure data type
/**
    IPE structure - IPE kdrv open object
*/
typedef struct {
	UINT32 ipe_clock_sel;       ///< Engine clock selection
} KDRV_IPE_OPENCFG, *pKDRV_IPE_OPENCFG;

/**
    IPE structure - IPE kdrv trigger parameter
*/
typedef struct {
	UINT32 reserved;            ///< reserved
} KDRV_IPE_TRIGGER_PARAM;

/**
    IPE KDRV Address info
*/
typedef struct {
	UINT32 addr_y;              ///< DMA address of Y channel, Eth only Y channel
	UINT32 addr_uv;             ///< DMA address of UV channel
} KDRV_IPE_DMA_ADDR_INFO;

/**
    IPE KDRV enum - Input YCC format
*/
typedef enum {
	/* 3 channel */
	KDRV_IPE_IN_FMT_Y_PACK_UV444,            ///< 444 format y planar UV pack UVUVUVUVUV.....
	KDRV_IPE_IN_FMT_Y_PACK_UV422,            ///< 422 format y planar UV pack UVUVUVUVUV.....
	KDRV_IPE_IN_FMT_Y_PACK_UV420,            ///< 420 format y planar UV pack UVUVUVUVUV.....
	/* extend */
	KDRV_IPE_IN_FMT_Y,                       ///< Only Y information
	ENUM_DUMMY4WORD(KDRV_IPE_IN_FMT)
} KDRV_IPE_IN_FMT;

/**
    IPE KDRV enum - Output YCC format
*/
typedef enum {
	/* 3 channel */
	KDRV_IPE_OUT_FMT_Y_PACK_UV444,          ///< 444 format y planar UV pack UVUVUVUVUV.....
	KDRV_IPE_OUT_FMT_Y_PACK_UV422,          ///< 422 format y planar UV pack UVUVUVUVUV.....
	KDRV_IPE_OUT_FMT_Y_PACK_UV420,          ///< 420 format y planar UV pack UVUVUVUVUV.....
	/* extend */
	KDRV_IPE_OUT_FMT_Y,                     ///< Only Y information
	ENUM_DUMMY4WORD(KDRV_IPE_OUT_FMT)
} KDRV_IPE_OUT_FMT;

/**
    IPE KDRV enum - Input YCC range format
*/
typedef enum {
	KDRV_IPE_YUV_FMT_FULL,              ///<
	KDRV_IPE_YUV_FMT_BT601,             ///<
	KDRV_IPE_YUV_FMT_BT709,             ///<
	ENUM_DUMMY4WORD(KDRV_IPE_YVU_FMT)
} KDRV_IPE_YVU_FMT;

/**
    IPE KDRV enum - dram single output mode selection
*/
typedef enum {
	KDRV_IPE_DRAM_OUT_NORMAL            = 0,    ///<
	KDRV_IPE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(KDRV_IPE_DRAM_OUTPUT_MODE)
} KDRV_IPE_DRAM_OUTPUT_MODE;

/**
    IPE KDRV Input Image Information
*/
typedef struct {
	KDRV_IPE_IN_SRC      in_src;            ///< input image source
	KDRV_IPE_IN_FMT      type;              ///< image type. This is valid only when output to DRAM (support KDRV_IPP_YUV444/KDRV_IPP_YUV422/KDRV_IPP_YUV420/KDRV_IPP_Y_ONLY)
	KDRV_IPE_YVU_FMT     yuv_range_fmt;     ///< yuv input range format.
	KDRV_IPE_IN_CH_INFO  ch;                ///< image channel information
} KDRV_IPE_IN_IMG_INFO;

/**
    IPE KDRV Output Image Information
*/

typedef struct {
	KDRV_IPE_OUT_FMT type;          ///< image type. This is valid only when output to DRAM (support KDRV_IPP_YUV444/KDRV_IPP_YUV422/KDRV_IPP_YUV420/KDRV_IPP_Y_ONLY)
	BOOL dram_out;                  ///< output image to DRAM, only support when Eth output to DRAM DISABLE (KDRV_IPE_ETH_PARAM, b_output_eth)
} KDRV_IPE_OUT_IMG_INFO;

/**
    IPE KDRV Defog In/Out address Information
*/


typedef struct {
	UINT32 addr;
} KDRV_IPE_DMA_SUBIN_ADDR;

typedef struct {
	BOOL enable;
	UINT32 addr;
} KDRV_IPE_DMA_SUBOUT_ADDR;

/**
    IPE KDRV Defog In/Out Information
*/

typedef struct {
	KDRV_IPE_SIZE subimg_size;                              ///< defog sub-image size, max (32 x 32)
	UINT32        subimg_lofs_in;                           ///< defog subin lineoffset
	UINT32        subimg_lofs_out;                          ///< defog subout lineoffset
	UINT8         subimg_ftrcoef[KDRV_IPE_SUBIMG_FILT_LEN]; ///< sub-image low pass filter coefficients, 3 entries
} KDRV_IPE_SUBIMG_PARAM;

/**
    IPE Kdrv - Dram single output channel enable
*/
typedef struct {
	BOOL single_out_y_en;
	BOOL single_out_c_en;
	BOOL single_out_va_en;
	BOOL single_out_defog_en;
} KDRV_IPE_DRAM_SINGLE_OUT_EN;


/**
    IPE KDRV IQ parameter Information
*/

/**
    IPE edge extract gamma selection

    Select pre or post gamma for edge extract
*/
typedef enum {
	KDRV_IPE_EEXT_PRE_GAM = 0,      ///< Select pre-gamma edge extraction
	KDRV_IPE_EEXT_POST_GAM = 1,     ///< Select post-gamma edge extraction
	ENUM_DUMMY4WORD(KDRV_IPE_EEXT_GAM_ENUM)
} KDRV_IPE_EEXT_GAM_ENUM;

/**
    IPE edge extract channel selection

    Select G or Y for edge extract
*/
typedef enum {
	KDRV_IPE_EEXT_YUVIN = 0,     ///< Select Y for edge extraction
	KDRV_IPE_EEXT_RGBIN = 1,     ///< Convert to Y for edge extraction
	ENUM_DUMMY4WORD(KDRV_IPE_EEXT_CH_ENUM)
} KDRV_IPE_EEXT_CH_ENUM;

/**
    IPE edge mapping input selection

    Select intput data for edge mapping
*/
typedef enum {
	KDRV_IPE_EIN_ENG = 0,       ///< Select edge energy for edge mapping input
	KDRV_IPE_EIN_EEXT = 1,      ///< Select EEXT for edge mapping input
	KDRV_IPE_EIN_AVG = 2,       ///< Select average of EEXT & edge energy for edge mapping input
	ENUM_DUMMY4WORD(KDRV_IPE_EMAP_IN_ENUM)
} KDRV_IPE_EMAP_IN_ENUM;

/**
    IPE directed edge: y diff range

    Select y diff range

*/
typedef enum {
	KDRV_IPE_YDR_9x9 = 0,      ///< Select 9x9 for y diff calculation
	KDRV_IPE_YDR_7x7 = 1,      ///<  Select 7x7 for y diff calculation
	KDRV_IPE_YDR_5x5 = 2,      ///<  Select 5x5 for y diff calculation
	KDRV_IPE_YDR_3x3 = 3,      ///<  Select 3x3 for y diff calculation
	ENUM_DUMMY4WORD(KDRV_IPE_YDRANGE)
} KDRV_IPE_YDRANGE;

/**
    IPE directed edge: eext kernel selection

    Select directed eext kernel

*/
typedef enum {
	KDRV_IPE_KER_A = 0,      ///< kernel A
	KDRV_IPE_KER_B = 1,      ///< kernel B
	ENUM_DUMMY4WORD(KDRV_IPE_KERSEL)
} KDRV_IPE_KERSEL;

/**
    IPE directed edge: connection score

    Select connection score of directed edge

*/
typedef enum {
	KDRV_IPE_DE_ADD1 = 0,      ///< add 1
	KDRV_IPE_DE_ADD2 = 1,      ///< add 2
	ENUM_DUMMY4WORD(KDRV_IPE_DECONSCR)
} KDRV_IPE_DECONSCR;

/**
    IPE directed edge: index selection in connection

    Select index selection in connection of directed edge

*/
typedef enum {
	KDRV_IPE_DIR_COMPLEX = 0,      ///< as complex area
	KDRV_IPE_DIR_FLAT = 1,         ///< as flat area
	ENUM_DUMMY4WORD(KDRV_IPE_DECON_IDXSEL)
} KDRV_IPE_DECON_IDXSEL;

/**
    IPE edge weighting: kernal B output selection

    Select kernal B input sources

*/
typedef enum {
	KDRV_IPE_AVG_HV = 0,         ///< (ED0+ED1)>>1
	KDRV_IPE_AVG_4D = 1,         ///< (ED0+ED1+ ED2+ED3)>>2
	ENUM_DUMMY4WORD(KDRV_IPE_EW_KBOUTSEL)
} KDRV_IPE_EW_KBOUTSEL;

//@}

/**
    IPE RGB LPF: kernal size selection

    Select kernel size of low-pass filter

*/
typedef enum {
	KDRV_IPE_LPFSIZE_3X3 = 0,    ///< 3x3 filter size
	KDRV_IPE_LPFSIZE_5X5 = 1,    ///< 5x5 filter size
	KDRV_IPE_LPFSIZE_7X7 = 2,    ///< 7x7 filter size
	ENUM_DUMMY4WORD(KDRV_IPE_LPF_SIZE_ENUM)
} KDRV_IPE_LPF_SIZE_ENUM;

/**
    IPE CC range: precision selection

    Select precision 2.9 or 3.8 or 4.7 of color correction

*/
typedef enum {
	KDRV_IPE_CCRANGE_2_9 = 0,    ///< Precision 2.9
	KDRV_IPE_CCRANGE_3_8 = 1,    ///< Precision 3.8
	KDRV_IPE_CCRANGE_4_7 = 2,    ///< Precision 4.7
	ENUM_DUMMY4WORD(KDRV_IPE_CC_RANGE_ENUM)
} KDRV_IPE_CC_RANGE_ENUM;

/**
    IPE CC output: weighting reference

    Select identity or gray for output weighting

*/
typedef enum {
	KDRV_IPE_CC2_IDENTITY = 0,   ///< Identity matrix
	KDRV_IPE_CC2_GRAY = 1,       ///< Gray matrix
	ENUM_DUMMY4WORD(KDRV_IPE_CC2_SEL_ENUM)
} KDRV_IPE_CC2_SEL_ENUM;

/**
    IPE color correction gamma selection

    Select pre or post gamma for color correction
*/
typedef enum {
	KDRV_IPE_CC_PRE_GAM = 0,       ///< Select pre-gamma color correction
	KDRV_IPE_CC_POST_GAM = 1,      ///< Select post-gamma color correction
	ENUM_DUMMY4WORD(KDRV_IPE_CC_GAMSEL)
} KDRV_IPE_CC_GAMSEL;

/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	KDRV_IPE_CC_MAX = 0,             ///< Select max
	KDRV_IPE_CC_MIN = 1,             ///< Select min
	KDRV_IPE_CC_MAX_MINUS_MIN = 2,   ///< Select max-min
	ENUM_DUMMY4WORD(KDRV_IPE_CC_STABSEL)
} KDRV_IPE_CC_STABSEL;

/**
    IPE color correction offset option

    Select mode of color correction offset
*/

typedef enum {
	KDRV_CC_OFS_BYPASS = 0,      ///< Select bypass
	KDRV_CC_OFS_Y_FULL = 1,      ///< Select offset for full YCC to RGB
	KDRV_CC_OFS_Y_BTU  = 2,      ///< Select offset for Bt601/Bt709 YCC to RGB
	ENUM_DUMMY4WORD(KDRV_IPE_CC_OFSSEL)
} KDRV_IPE_CC_OFSSEL;

/**
    IPE Cb Cr contrast look-up tabel option

    Select mode of Cb Cr contrast look-up tabel
*/

typedef enum {
	KDRV_SATURATION = 0,            ///<
	KDRV_ABS_MEAN = 1,              ///<
	ENUM_DUMMY4WORD(KDRV_IPE_CCONTAB_SEL)
} KDRV_IPE_CCONTAB_SEL;


/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	KDRV_IPE_CST_NOOP = 0,             ///< No operation
	KDRV_IPE_CST_MINUS128 = 1,         ///< out = in - 128
	ENUM_DUMMY4WORD(KDRV_IPE_CST_OFFSEL)
} KDRV_IPE_CST_OFFSEL;

/**
    IPE YC thresholding: replacement option

    Select replacement with the original or a value specified by the register
*/
typedef enum {
	KDRV_IPE_YCTH_ORIGINAL_VALUE = 0,
	KDRV_IPE_YCTH_REGISTER_VALUE = 1,
	ENUM_DUMMY4WORD(KDRV_IPE_YCTH_SEL_ENUM)
} KDRV_IPE_YCTH_SEL_ENUM;

/**
    IPE color control source option

    Select edge reference of color control vdet
*/
typedef enum {
	KDRV_IPE_CCTRL_E5b = 0,     ///< 5x5 kernel B output
	KDRV_IPE_CCTRL_E5a = 1,           ///< 5x5 kernel A output
	KDRV_IPE_CCTRL_E7 = 2,           ///< 7x7 kernel output
	ENUM_DUMMY4WORD(KDRV_IPE_CCTRL_SEL_ENUM)
} KDRV_IPE_CCTRL_SEL_ENUM;

/**
    IPE rgb gamma curve option

    KDRV_IPE_GAMMA_RGB_COMBINE:
        RGB curve combin
        only rgb_curve_addr[KDRV_IPP_RGB_R] valid
        R channel: uiGammaAddr[KDRV_IPP_RGB_R]
        G channel: uiGammaAddr[KDRV_IPP_RGB_R]
        B channel: uiGammaAddr[KDRV_IPP_RGB_R]

    KDRV_IPE_GAMMA_RGB_SEPERATE
        RGB curve seperate
        R channel: uiGammaAddr[KDRV_IPP_RGB_R]
        G channel: uiGammaAddr[KDRV_IPP_RGB_G]
        B channel: uiGammaAddr[KDRV_IPP_RGB_B]
*/
typedef enum {
	KDRV_IPE_GAMMA_RGB_COMBINE = 0,  ///< RGB curve combin, reference uiGammaAddr[KDRV_IPP_RGB_R]
	KDRV_IPE_GAMMA_RGB_SEPERATE = 1, ///< RGB curve seperate, only support when PARAM_YCURVE disable
	ENUM_DUMMY4WORD(KDRV_IPE_GAMMA_OPTION)
} KDRV_IPE_GAMMA_OPTION;

/**
    IPE Y curve selection

    Select Y curve domain
*/
typedef enum {
	KDRV_YCURVE_RGB_Y     = 0,       ///<
	KDRV_YCURVE_RGB_V     = 1,       ///<
	KDRV_YCURVE_YUV       = 2,       ///<
	ENUM_DUMMY4WORD(KDRV_IPE_YCURVE_SEL)
} KDRV_IPE_YCURVE_SEL;

/**
    IPE edge debug mode selection
*/
typedef enum {
	KDRV_DBG_EDGE_REGION    = 0,       ///<
	KDRV_DBG_EDGE_WEIGHT    = 1,       ///<
	KDRV_DBG_EDGE_STRENGTH  = 2,       ///<
	ENUM_DUMMY4WORD(KDRV_IPE_EDGEDBG_SEL)
} KDRV_IPE_EDGEDBG_SEL;

/**
    IPE 3DCC rounding

    Select 3DCC rounding type
*/
typedef enum {
	KDRV_IPE_ROUNDING = 0,             ///< rounding
	KDRV_IPE_HALFTONE = 1,             ///< halftone rounding
	KDRV_IPE_RANDOM = 2,               ///< random rounding
	ENUM_DUMMY4WORD(KDRV_IPE_3DCC_ROUNDOPT)
} KDRV_IPE_3DCC_ROUNDOPT;

/**
    IPE ETH ouput downsampling

    Select if ETH is downsampled at output
*/
typedef enum {
	KDRV_ETH_OUT_ORIGINAL = 0,             ///< original size
	KDRV_ETH_OUT_DOWNSAMPLED = 1,          ///< downsampled by 2x2
	KDRV_ETH_OUT_BOTH = 2,                 ///< both origianl size and downsample size
	ENUM_DUMMY4WORD(KDRV_IPE_ETH_OUTSEL)
} KDRV_IPE_ETH_OUTSEL;

/**
    IPE ETH ouput format

    Select ETH output bitdepth
*/
typedef enum {
	KDRV_ETH_OUT_2BITS = 0,             ///< 2 bits/pixel
	KDRV_ETH_OUT_8BITS = 1,             ///< 8 bits/pixel
	ENUM_DUMMY4WORD(KDRV_IPE_ETH_OUTFMT)
} KDRV_IPE_ETH_OUTFMT;

/**
    IPE ETH ouput position

    Select ETH output position when downsampling
*/
typedef enum {
	KDRV_IPE_ETH_POS_EVEN = 0,             ///< even position
	KDRV_IPE_ETH_POS_ODD = 1,              ///< odd position
	ENUM_DUMMY4WORD(KDRV_IPE_ETH_POSSEL)
} KDRV_IPE_ETH_POSSEL;

/**
    IPE defog target method selection
*/
typedef enum {
	KDRV_DEFOG_METHOD_A    = 0,       ///<
	KDRV_DEFOG_METHOD_B    = 1,       ///<
	ENUM_DUMMY4WORD(KDRV_IPE_DEFOG_METHOD_SEL)
} KDRV_IPE_DEFOG_METHOD_SEL;

/**
    IPE defog output blending reference selection
*/
typedef enum {
	KDRV_DEFOG_OUTBLD_REF_BEFOR    = 0,       ///<
	KDRV_DEFOG_OUTBLD_REF_AFTER    = 1,       ///<
	ENUM_DUMMY4WORD(KDRV_IPE_DEFOG_OUTBLD_REF_SEL)
} KDRV_IPE_DEFOG_OUTBLD_REF_SEL;


/**
    IPE parameter mode
*/
typedef enum {
	KDRV_IPE_PARAM_AUTO_MODE  = 0,   ///< parameters using auto mode
	KDRV_IPE_PARAM_USER_MODE  = 1,   ///< parameters using user mode
	ENUM_DUMMY4WORD(KDRV_IPE_PARAM_MODE)
} KDRV_IPE_PARAM_MODE;

/**
    IPE VA filter symmety selection
*/
typedef enum {
	KDRV_IPE_VA_FILTER_SYM_MIRROR = 0,
	KDRV_IPE_VA_FILTER_SYM_INVERSE,
	ENUM_DUMMY4WORD(KDRV_IPE_VA_FILTER_SYM_SEL)
} KDRV_IPE_VA_FILTER_SYM_SEL;

/**
    IPE VA filter size selection
*/
typedef enum {
	KDRV_IPE_VA_FILTER_SIZE_1 = 0,
	KDRV_IPE_VA_FILTER_SIZE_3,
	KDRV_IPE_VA_FILTER_SIZE_5,
	KDRV_IPE_VA_FILTER_SIZE_7,
	ENUM_DUMMY4WORD(KDRV_IPE_VA_FILTER_SIZE_SEL)
} KDRV_IPE_VA_FILTER_SIZE_SEL;

#if 0
/**
    IPE structure - IPE directed edge eext
*/
typedef struct {
	KDRV_IPE_YDRANGE yd_range;              ///< range of calculation y difference
	UINT32      yd_max_th;                  ///< threshold of max y difference
	BOOL        pnd_adj_en;                 ///< enable of adjusting PD/ND edges
	UINT32      pnd_shift;                  ///< PNDEdge = PNDedge - (PNDedge>>(PndShft+1))
	UINT32      edge_shift;                 ///< Edge shift for all directions
	KDRV_IPE_KERSEL  ker_sel;               ///< Select kernel of directed eext
} KDRV_IPE_DIREDG_EEXT;

/**
    IPE structure - IPE directed edge score calculation
*/
//@{
typedef struct {
	INT8        a0;              ///< a coefficient of score 0, range: -8~7
	INT8        b0;              ///< b coefficient of score 0, range: -8~7
	INT8        c0;              ///< c coefficient of score 0, range: -8~7
	UINT8       d0;              ///< d coefficient of score 0, range: 0~15
	INT8        a1;              ///< a coefficient of score 1, range: -8~7
	INT8        b1;              ///< b coefficient of score 1, range: -8~7
	INT8        c1;              ///< c coefficient of score 1, range: -8~7
	UINT8       d1;              ///< d coefficient of score 1, range: 0~15
	INT8        a2;              ///< a coefficient of score 2, range: -8~7
	INT8        b2;              ///< b coefficient of score 2, range: -8~7
	INT8        c2;              ///< c coefficient of score 2, range: -8~7
	UINT8       d2;              ///< d coefficient of score 2, range: 0~15
} KDRV_IPE_DIREDG_SCORECAL;
//@}

/**
    IPE structure - IPE directed edge score calculation
*/
//@{
typedef struct {
	UINT8       score_th0;       ///< score threshold 0
	UINT8       score_th1;       ///< score threshold 1
	UINT8       score_th2;       ///< score threshold 2
	UINT8       score_th3;       ///< score threshold 3
} KDRV_IPE_DIREDG_SCORETH;
//@}

/**
    IPE structure - IPE directed edge connection parameters
*/
//@{
typedef struct {
	KDRV_IPE_DECONSCR score_center;             ///< score of center
	KDRV_IPE_DECONSCR score_edge;               ///< score of edges
	KDRV_IPE_DECONSCR score_detatil;            ///< score of details
	UINT8        cnt_num_th;                    ///< threshold of count number
	UINT8        max1_th;                       ///< threshold of max 1
	UINT8        max_cnt_th;                    ///< threshold of max count
	UINT8        max2_th;                       ///< threshold of max 2
	KDRV_IPE_DECON_IDXSEL idx_sel;              ///< index selection
} KDRV_IPE_DIREDG_CONPARAM;
//@}

/**
    IPE structure - IPE edge weighting paramters of kernel A
*/
//@{
typedef struct {
	KDRV_IPE_KERSEL   ker_sel;          ///< 2D kernel selection of edge weighting
	UINT8        adwt0;                 ///< kernal A's dir weight 0, range: 0~63
	UINT8        adwt1;                 ///< kernal A's dir weight 1, range: 0~63
	UINT8        adwt2;                 ///< kernal A's dir weight 2, range: 0~63
	UINT8        adwt3;                 ///< kernal A's dir weight 3, range: 0~63
	UINT8        adwt4;                 ///< kernal A's dir weight 4, range: 0~63
	UINT8        adwt5;                 ///< kernal A's dir weight 5, range: 0~63
	INT8         ads0;                  ///< kernal A's dir shift 0, range: -4~3
	INT8         ads1;                  ///< kernal A's dir shift 1, range: -4~3
	INT8         ads2;                  ///< kernal A's dir shift 2, range: -4~3
	INT8         ads3;                  ///< kernal A's dir shift 3, range: -4~3
	INT8         ads4;                  ///< kernal A's dir shift 4, range: -4~3
	INT8         ads5;                  ///< kernal A's dir shift 5, range: -4~3
} KDRV_IPE_EWKERA;
//@}

/**
    IPE structure - IPE edge weighting paramters of kernel B
*/
//@{
typedef struct {
	UINT8           bdwt0;              ///< kernal B's dir weight 0, range: 0~7
	UINT8           bdwt1;              ///< kernal B's dir weight 1, range: 0~7
	INT8            bds0;               ///< kernal B's dir shift 0, range: -2~1
	INT8            bds1;               ///< kernal B's dir shift 1, range: -2~1
	KDRV_IPE_EW_KBOUTSEL kel_sel;       ///< Kernal B's output selection
} KDRV_IPE_EWKERB;
//@}

/**
    IPE structure - IPE edge weighting paramters
*/
//@{
typedef struct {
	UINT8           ewd0;               ///< weighting 0 with E7
	UINT8           ewd1;               ///< weighting 1 with E7
	UINT8           ewd2;               ///< weighting 2 with E7
	BOOL            dynamic_map_en;     ///< enable of dynamic e-mapping
} KDRV_IPE_EW_PARAM;
//@}

#endif

/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	UINT32 eext_enh;                ///< Legal range : 0~63, enhance term for kernel
	INT32 eext_div;                 ///< Legal range : -4~3, normalize term for kernel
} KDRV_EEXT_KER_STR;

/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	KDRV_EEXT_KER_STR ker_freq0;    ///< strength of kernal A
	KDRV_EEXT_KER_STR ker_freq1;    ///< strength of kernal C
	KDRV_EEXT_KER_STR ker_freq2;    ///< strength of kernal D
} KDRV_IPE_EEXT_KER_STRENGTH;

/**
    IPE structure - IPE edge eext, strength of edge contrast and edge energy
*/
//@{
typedef struct {
	INT32 eext_div_con;             ///< Legal range : -8~7, normalize term for edge contrast
	INT32 eext_div_eng;             ///< Legal range : -8~7, normalize term for edge engergy
	UINT32 wt_con_eng;              ///< Legal range : 0~8, blending weight between edge engergy and edge contrast, 0: all contrast, 8: all edge energy
} KDRV_IPE_EEXT_ENG_CON;
//@}

/**
    IPE structure - IPE edge kernel thickness

                    ker B_C     ker A_B
                        |           |
        iso_ker_robust  |           | iso_ker_thin
                        |           |
                    ker D        ker C

    input: ker A & ker B -> wt_ker_thin -> output: kerA_B
    input: ker B & ker C -> wt_ker_robust -> output: kerB_C
    input: ker A_B & ker C -> iso_ker_thin -> output: kerAB_C
    input: ker B_C & ker D -> iso_ker_robust -> output: kerBC_D
*/
//@{
typedef struct {
	UINT32 wt_ker_thin;             ///< Legal range : 0~8, Thin kernel weighting between A & B, 8: all thinner kernel A
	UINT32 wt_ker_robust;           ///< Legal range : 0~8, robust kernel weighting between B & C, 8: all thinner kernel B
	UINT32 iso_ker_thin;            ///< Legal range : 0~8, Thin kernel weighting between A_B & C, 8: all thinner kernel A_B
	UINT32 iso_ker_robust;          ///< Legal range : 0~8, robust kernel weighting between B_C & D, 8: all thinner kernel B_C

} KDRV_IPE_KER_THICKNESS;
//@}

/**
    IPE structure - IPE region weight parameters
*/
typedef struct {
	UINT32 wt_low;                  ///< Legal range : 0~16, the kernel weighting of flat region
	UINT32 wt_high;                 ///< Legal range : 0~16, the kernel weighting of edge region
	UINT32 wt_low_hld;              ///< Legal range : 0~16, the kernel weighting of flat region in HLD mode
	UINT32 wt_high_hld;             ///< Legal range : 0~16, the kernel weighting of edge region in HLD mode
} KDRV_IPE_REGION_WEIGHT;
//@

/**
    IPE structure - IPE region threshold parameters
*/
typedef struct {
	UINT32 th_flat;                 ///< Legal range : 0~1023, the threshold for flat region
	UINT32 th_edge;                 ///< Legal range : 0~1023, the threshold for edge region
	UINT32 th_flat_hld;             ///< Legal range : 0~1023, the threshold for flat region in HLD mode
	UINT32 th_edge_hld;             ///< Legal range : 0~1023, the threshold for edge region in HLD mode
	UINT32 th_lum_hld;              ///< Legal range : 0~1023, the luma threshold in HLD mode
} KDRV_IPE_REGION_THRESH;
//@

/**
    IPE structure - IPE region classification parameters
*/
//@{
typedef struct {
	KDRV_IPE_REGION_THRESH reg_th;  ///< the region threshold for region classification
	KDRV_IPE_REGION_WEIGHT reg_wt;  ///< the region weight for finding kernel weighting for blending
} KDRV_IPE_REGION_PARAM;
//@}


/**
    IPE structure - IPE edge map
*/
//@{
typedef struct {
	UINT32 ethr_low;                ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 ethr_high;               ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 etab_low;                ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32 etab_high;               ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
	KDRV_IPE_EMAP_IN_ENUM map_sel;  ///< Edge mapping input selection
} KDRV_IPE_EDGEMAP_PARAM;
//@}

/**
    IPE structure - IPE luminance map
*/
//@{
typedef struct {
	UINT32 ethr_low;                ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 ethr_high;               ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 etab_low;                ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32 etab_high;               ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
} KDRV_IPE_ESMAP_PARAM;
//@}


/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	UINT32 lpf_w;                               ///< LPF weighting, range: 0~15
	UINT32 s_only_w;                            ///< S-Only weighting, range: 0~15
	UINT32 range_th0;                           ///< Range threshold 0, range: 0~1023
	UINT32 range_th1;                           ///< Range threshold 1, range: 0~1023
	KDRV_IPE_LPF_SIZE_ENUM filt_size;           ///< Filter size
} KDRV_IPE_STR_RGBLPF_PARAM;
//@}

typedef struct {
	BOOL enable;
	UINT32 color_u;                             ///< Legal Range: 0~255, reference color u for PFR
	UINT32 color_v;                             ///< Legal Range: 0~255, reference color v for PFR
	UINT32 color_wet_r;                         ///< Legal Range: 0~255, PFR weight for R channel
	UINT32 color_wet_b;                         ///< Legal Range: 0~255, PFR weight for B channel
	UINT32 cdiff_th;                            ///< Legal Range: 0~255, threshold for color difference weighting table
	UINT32 cdiff_step;                          ///< Legal Range: 0~6, step size for color difference weighting table
	UINT32 cdiff_lut[KDRV_IPE_PFR_COLOR_LEN];   ///< Legal Range: 0~255, table for color difference weighting
} KDRV_IPE_COLORWET_PARAM;

/**
    IPE structure - IPE Y threshold 1
*/
//@{
typedef struct {
	UINT32 y_th;                                ///< Y threshold, range: 0~255
	UINT32 edge_th;                             ///< Edge threshold, range: 0~1023
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_hit;        ///< Value selection when hit
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_nonhit;     ///< Value selection when not-hit
	UINT32 value_hit;                           ///< The register value when hit, range: 0~255
	UINT32 nonvalue_hit;                        ///< The register value when not-hit, range: 0~255
} KDRV_IPE_STR_YTH1_PARAM;
//@}

/**
    IPE structure - IPE Y threshold 2
*/
//@{
typedef struct {
	UINT32 y_th;                                ///< Y threshold, range: 0~255
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_hit;        ///< Value selection when hit
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_nonhit;     ///< Value selection when not-hit
	UINT32 value_hit;                           ///< The register value when hit, range: 0~255
	UINT32 nonvalue_hit;                        ///< The register value when not-hit, range: 0~255
} KDRV_IPE_STR_YTH2_PARAM;
//@}

/**
    IPE structure - IPE C threshold
*/
//@{
typedef struct {
	UINT32 edge_th;                             ///< Edge threshold, range: 0~1023
	UINT32 y_th_low;                            ///< Low threshold of Y
	UINT32 y_th_high;                           ///< High threshold of Y
	UINT32 cb_th_low;                           ///< Low threshold of CB
	UINT32 cb_th_high;                          ///< High threshold of CB
	UINT32 cr_th_low;                           ///< Low threshold of CR
	UINT32 cr_th_high;                          ///< High threshold of CR
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_hit;        ///< Value selection when hit
	KDRV_IPE_YCTH_SEL_ENUM ycth_sel_nonhit;     ///< Value selection when not-hit
	UINT32 cb_value_hit;                        ///< The register value of CB when hit, range: 0~255
	UINT32 cb_value_nonhit;                     ///< The register value of CB when not-hit, range: 0~255
	UINT32 cr_value_hit;                        ///< The register value of CR when hit, range: 0~255
	UINT32 cr_value_nonhit;                     ///< The register value of CR when not-hit, range: 0~255
} KDRV_IPE_STR_CTH_PARAM;

/**
    IPE structure - IPE random input of gamma and y curve
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
	BOOL rand_en;                               ///< Enable of random input
	BOOL rst_en;                                ///< Reset of random input
	UINT8 rand_shift;                           ///< Shift of random input, range: 0~1, 0:2bits, 1:3bits
} KDRV_IPE_GAMYRAND_PARAM;
//@}

/**
    IPE structure - IPE 3DCC rounding
*/
//@{
typedef struct {
	KDRV_IPE_3DCC_ROUNDOPT round_opt;           ///< Rounding option
	BOOL rst_en;                                ///< Reset of random input
} KDRV_IPE_3DCCROUND;
//@}

/**
    IPE structure - Parameters of IPE Edge Thresholding
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
	KDRV_IPE_ETH_OUTSEL eth_outsel;             ///< If ETH output is downsampled
	KDRV_IPE_ETH_OUTFMT eth_outfmt;             ///< ETH output format
	UINT32 th_low;                              ///< Low edge threshold value, range: 0~1023
	UINT32 th_mid;                              ///< Middle edge threshold value, range: 0~1023
	UINT32 th_high;                             ///< High edge threshold value, range: 0~1023
	KDRV_IPE_ETH_POSSEL outsel_h;               ///< Horizontal position selection when downsampling
	KDRV_IPE_ETH_POSSEL outsel_v;               ///< Vertical position selection when downsampling
} KDRV_IPE_ETH_INFO;
//@}

/**
    IPE structure - Defog input blending parameters
*/
typedef struct {
	UINT8  in_blend_wt[KDRV_DFG_INPUT_BLD_LEN];         ///< Legal Range: 0~ 255, input blending weight table
} KDRV_IPE_DEFOG_IN_BLD_PARAM;

/**
    IPE structure - Defog scale parameters
*/
typedef struct {
	UINT8 interp_diff_lut[KDRV_DFG_INTERP_DIFF_LEN];    ///< Legal Range: 0~ 63, difference table for edge preserved interpolation
	UINT8 interp_wdist;                                 ///< Legal Range: 0~ 8, weight of distance beween min and average in edge preserved interpolation
	UINT8 interp_wout;                                  ///< Legal Range: 0~ 8, weight of edge preserve result, 0: no edge preserved,  8: edge preserved
	UINT8 interp_wcenter;                               ///< Legal Range: 1~ 63, weight of center in edge preserve interpolation
	UINT8 interp_wsrc;                                  ///< Legal Range: 0~ 255, weight of edge preserve result and src image
} KDRV_IPE_DEFOG_SCAL_PARAM;

/**
    IPE structure - Defog output blending parameters
*/
typedef struct {
	KDRV_IPE_DEFOG_OUTBLD_REF_SEL outbld_ref_sel;       ///< defog output blending reference selection
	BOOL outbld_local_en;                               ///< defog local output blending enable
	UINT8 outbld_lum_wt[KDRV_DFG_OUTPUT_BLD_LEN];       ///< defog luminance output blending table, range: 0~255
	UINT8 outbld_diff_wt[KDRV_DFG_OUTPUT_BLD_LEN];      ///< defog difference output blending table, range: 0~63
} KDRV_IPE_DEFOG_OUTBLD_PARAM;

/**
    IPE structure - Defog output blending parameters
*/
typedef struct {
	BOOL dfg_self_comp_en;                              ///< defog self compare enable
	UINT16 dfg_min_diff;                                ///< defog minimum difference
	UINT16 dfg_airlight[KDRV_DFG_AIRLIGHT_NUM];         ///< defog airlight settings, range: 0~ 1023
	UINT16 fog_mod_lut[KDRV_DFG_FOG_MOD_LEN];           ///< defog fog modify table, range: 0~ 1023
} KDRV_IPE_DEFOG_ENV_ESTIMATION;

/**
    IPE structure - Defog strength parameters
*/
typedef struct {
	KDRV_IPE_DEFOG_METHOD_SEL  str_mode_sel;            ///< defog method selection
	UINT16 target_lut[KDRV_DFG_TARGET_LEN];             ///< defog target min for method A, range: 0~ 1023
	UINT8 fog_ratio;                                    ///< defog fog ratio for method B, range : 0~255
	UINT8 dgain_ratio;                                  ///< defog d gain ratio, range: 0~ 255
	UINT8 gain_th;                                      ///< defog gain threshold, range: 0~255
} KDRV_IPE_DEFOG_STRENGTH_PARAM;

/**
    IPE structure - Defog statistics setting
*/
typedef struct {
	UINT32 airlight_stcs_ratio;                         ///< the ratio of the total pixel count, 4096 = 1x
} KDRV_IPE_DEFOG_STCS_RATIO_PARAM;


/**
    IPE structure - Defog statistics
*/
typedef struct {
	UINT16 dfg_airlight[KDRV_DFG_AIRLIGHT_NUM];         ///< the defog statistics
} KDRV_IPE_DEFOG_STCS;


typedef struct {
	KDRV_IPE_VA_FILTER_SYM_SEL symmetry;        ///< select filter kernel symmetry
	KDRV_IPE_VA_FILTER_SIZE_SEL filter_size;    ///< select filter kernel size
	UINT8 tap_a;                                ///< filter coefficent A, range: 0~31
	INT8 tap_b;                                 ///< filter coefficent B, range: -16~15
	INT8 tap_c;                                 ///< filter coefficent C, range: -8~7
	INT8 tap_d;                                 ///< filter coefficent D, range: -8~7
	UINT8 div;                                  ///< filter normalized term, range: 0~15
	UINT16 th_l;                                ///< lower threshold, range: 0~255
	UINT16 th_u;                                ///< upper threshold, range: 0~255
} KDRV_IPE_VA_FILTER;

typedef struct {
	KDRV_IPE_VA_FILTER h_filt;                  ///< horizontal filter
	KDRV_IPE_VA_FILTER v_filt;                  ///< vertical filter
	BOOL linemax_mode;                          ///< accumulates one max value in one line
	BOOL count_enable;                          ///< count the nonzero pixels
} KDRV_IPE_VA_GROUP_INFO;

typedef struct {
	BOOL enable;                                ///< va independent window enable
	BOOL linemax_g1;                            ///< accumulates one max value in one line group1
	BOOL linemax_g2;                            ///< accumulates one max value in one line group2
} KDRV_IPE_VA_INDEP_OPT;



#if 0
/**
    IPE structure - IPE edge extraction.
*/
//@{
typedef struct {
	KDRV_IPE_EEXT_CH_ENUM edge_ch_sel;                  ///< Edge input channel selection
	INT16 edge_ker[KDRV_IPE_EDGE_KER_DIV_LEN];          ///< Edge extraction kernel setting, table size: 12, range ([0]:0~1023, [1]~[9]:-512~511, [10]:0~15, [11]:0~31)
	UINT32 dir_edge_ker;                                ///< Legal range : 0~3, 0: Normal division for directed edge kernel, 1: -1 division, 2: -2 division, 3: -3 division
	UINT8 dir_edge_wtbl[KDRV_IPE_EDGE_DIR_TAB_LEN];     ///< Legal range : 0~8, wieghting for directed edge, 0 : no direction decision, 8 : sensitive edge decision, table size: 8
	KDRV_IPE_DIREDG_EEXT dir_edge_eext;                 ///< Directed edge extraction setting
	KDRV_IPE_DIREDG_SCORECAL dir_edge_score_calc;       ///< Directed edge score calculation setting
	KDRV_IPE_DIREDG_SCORETH dir_edge_score_th;          ///< Directed edge score thresholds
	BOOL power_save_en;                                 ///< Enable of power-saving mode
	KDRV_IPE_DIREDG_CONPARAM dir_edge_con;              ///< Directed edge connection setting
	KDRV_IPE_EWKERA dir_edge_w_a;                       ///< Directed edge weighting of kernal A
	KDRV_IPE_EWKERB dir_edge_w_b;                       ///< Directed edge weighting of kernal B
	KDRV_IPE_EW_PARAM dir_edge_w;                       ///< Weighting of directed edge and 7x7
} KDRV_IPE_EEXT_PARAM;
//@}
#else
/**
    IPE structure - IPE edge extraction.
*/
//@{
typedef struct {
	INT16 edge_ker[KDRV_IPE_EDGE_KER_DIV_LEN];          ///< Edge extraction user-defined kernel setting, table size: 12, range ([0]:0~1023, [1]~[9]:-512~511, [10]:0~15, [11]:0~31)
	KDRV_IPE_EEXT_KER_STRENGTH eext_kerstrength;        ///< Edge extraction fixed kernel strength setting
	KDRV_IPE_EEXT_ENG_CON eext_engcon;                  ///< Edge extraction edge energy setting
	KDRV_IPE_KER_THICKNESS ker_thickness;               ///< Edge extraction kernel thickness setting
	KDRV_IPE_KER_THICKNESS ker_thickness_hld;           ///< Edge extraction kernel thickness in high light detail region setting
	KDRV_IPE_REGION_PARAM eext_region;                  ///< Edge extraction region settings
} KDRV_IPE_EEXT_PARAM;
//@}
#endif

/**
    IPE structure - IPE edge tone mapping curve.
*/
//@{
typedef struct {
	KDRV_IPE_EEXT_GAM_ENUM gamma_sel;                   ///< Select pre- or post-gamma edge extraction
	UINT32 tone_map_lut[KDRV_IPE_TONE_MAP_LUT_LEN];     ///< Tone remap LUT, table size: 65
} KDRV_IPE_EEXT_TONEMAP_PARAM;
//@}

/**
    IPE structure - IPE overshoot/undershoot parameters
*/
//@{
typedef struct {
	BOOL overshoot_en;              ///< enable function
	UINT32 wt_overshoot;            ///< Legal range : 0~255, 128 = 1x, the weight for overshooting
	UINT32 wt_undershoot;           ///< Legal range : 0~255, 128 = 1x, the weight for undershooting

	UINT32 th_overshoot;            ///< Legal range : 0~255, the threshold for overshooting
	UINT32 th_undershoot;           ///< Legal range : 0~255, the threshold for undershooting
	UINT32 th_undershoot_lum;       ///< Legal range : 0~255, the luma threshold for undershooting enhance
	UINT32 th_undershoot_eng;       ///< Legal range : 0~255, the edge energy threshold for undershootingenhance

	UINT32 slope_overshoot;         ///< Legal range : 0~32767 (256, slope = 1), the slope of the overshooting weight curve,
	UINT32 slope_undershoot;        ///< Legal range : 0~32767 (256, slope = 1), the slope of the undershooting weight curve
	UINT32 slope_undershoot_lum;    ///< Legal range : 0~32767 (256, slope = 1), the slope of the enhanced undershooting luminance weight curve
	UINT32 slope_undershoot_eng;    ///< Legal range : 0~32767 (256, slope = 1), the slope of the enhanced undershooting energe weight curve

	UINT32 clamp_wt_mod_lum;        ///< Legal range : 0~255, clamping weight of luminace undershooting enhance control
	UINT32 clamp_wt_mod_eng;        ///< Legal range : 0~255, clamping weight of edge energy undershooting enhance control
	UINT32 strength_lum_eng;        ///< Legal range : 0~15, Strength of undershooting enhance control
	UINT32 norm_lum_eng;            ///< Legal range : 0~15, Normalized term of undershooting enhance control

} KDRV_IPE_EDGE_OVERSHOOT_PARAM;
//@}

/**
    IPE structure - IPE edge process.
*/
//@{
typedef struct {
	KDRV_IPE_EDGEMAP_PARAM edge_map_th;                 ///< ED map thresholds
	UINT8 edge_map_lut[KDRV_IPE_EDGE_MAP_LUT_LEN];      ///< ED map LUT(16 entries). Legal range: 0~255. 1X = 255, table size: 16
	KDRV_IPE_ESMAP_PARAM es_map_th;                     ///< ES map thresholds
	UINT8 es_map_lut[KDRV_IPE_ES_MAP_LUT_LEN];          ///< ES map LUT(16 entries). Legal range: 0~255. 1X = 64, table size: 16
} KDRV_IPE_EPROC_PARAM;
//@}

/**
    IPE structure - IPE edge stcs.
*/
//@{
typedef struct {
	UINT32 localmax_max;                        ///< staticstics, max value of the local max in a frame
	UINT32 coneng_max;                          ///< staticstics, max value of the edge energy in a frame
	UINT32 coneng_avg;                          ///< staticstics, average value of the edge energy in a frame
} KDRV_IPE_EDGE_STCS;
//@}

/**
    IPE structure - IPE edge threshold suggestion value.
*/
//@{
typedef struct {

	KDRV_IPE_EDGE_STCS stcs;                    ///< input statistics

	UINT32 edge_ratio;                          ///< Legal range: 0~ 255,
	UINT32 flat_ratio;                          ///< Legal range: 0~ 255,
	UINT32 luma_ratio;                          ///< Legal range: 0~ 255,

	KDRV_IPE_REGION_THRESH region_th_auto;      ///< output threshold

} KDRV_IPE_EDGE_THRES_AUTO;
//@}


/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	BOOL enable;                               ///< enable function
	KDRV_IPE_STR_RGBLPF_PARAM    lpf_param_r;  ///< LPF setting of R channel
	KDRV_IPE_STR_RGBLPF_PARAM    lpf_param_g;  ///< LPF setting of G channel
	KDRV_IPE_STR_RGBLPF_PARAM    lpf_param_b;  ///< LPF setting of B channel
} KDRV_IPE_RGBLPF_PARAM;
//@}

/**
    IPE structure - IPE PFR
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
	BOOL uv_filt_en;                            ///< UV filter enable function
	BOOL luma_level_en;                         ///< Luma level threshold enable function
	UINT32 pfr_strength;                        ///< PFR strength, range: 0~255. 0 = no PFR, 255: strong PFR
	UINT32 edge_th;                             ///< PFR edge weighting threshold, range: 0~255
	UINT32 edge_str;                            ///< PFR edge weighting strength, range: 0~255, 128 = 1x
	UINT32 luma_th;                             ///< PFR luminance level threshold, range: 0~1023
	UINT32 luma_lut[KDRV_IPE_PFR_LUMA_LEN];     ///< PFR luminance level control table, range: 0~255
	UINT32 color_wet_g;                         ///< Legal Range: 0~255, PFR weight for G channel
	KDRV_IPE_COLORWET_PARAM color_wet_set[KDRV_IPE_PFR_SET_NUM];
} KDRV_IPE_PFR_PARAM;
//@}

/**
    IPE structure - IPE CC enable.
*/
//@{
typedef struct {
	BOOL enable;                            ///< enable function
} KDRV_IPE_CC_EN_PARAM;
//@}

/**
    IPE structure - IPE CC strength.
*/
//@{
typedef struct {
	KDRV_IPE_CC2_SEL_ENUM cc2_sel;          ///< Select the other data of CC weighting
	KDRV_IPE_CC_STABSEL cc_stab_sel;        ///< Select CC stab input
	UINT8 fstab[KDRV_IPE_FTAB_LEN];         ///< Color correction stab, table size: 16
	UINT8 fdtab[KDRV_IPE_FTAB_LEN];         ///< Color correction dtab, table size: 16
} KDRV_IPE_CC_PARAM;
//@}

/**
    IPE structure - IPE CCM.
*/
//@{
typedef struct {
	KDRV_IPE_CC_RANGE_ENUM cc_range;        ///< Range of matrix coefficients,0: 3.8 format,1: 4.7format
	KDRV_IPE_CC_GAMSEL cc_gamma_sel;        ///< Select pre- or post-gamma CC
	INT16 coef[KDRV_IPE_COEF_LEN];          ///< Color correction matrix [rr rg rb gr gg gb br bg bb], table size: 9
} KDRV_IPE_CCM_PARAM;
//@}

/**
    IPE structure - IPE CST.
*/
//@{
typedef struct {
	BOOL enable;                            ///< enable function
	INT16 cst_coef[KDRV_IPE_COEF_LEN];      ///< Color space transform matrix [yr yg yb ur ug ub vr vg vb], table size: 9
	KDRV_IPE_CST_OFFSEL cst_off_sel;        ///< Operation when CST is off
} KDRV_IPE_CST_PARAM;
//@}

/**
    IPE structure - IPE CCTRL ENABLE
*/
//@{
typedef struct {
	BOOL enable;                            ///< enable function
} KDRV_IPE_CCTRL_EN_PARAM;
//@}

/**
    IPE structure - IPE CCTRL PARAM.
*/
//@{
typedef struct {
	INT16 int_ofs;                          ///< Legal range : -128~127, Y offset, Y' = Y + Intofs
	INT16 sat_ofs;                          ///< Legal range : -128~127, color saturation adjust, 0 : NO color saturation adjust
	BOOL hue_c2g;                           ///< Select G ch for hue calculation, 0: input is G channel, 1: input is C channel
	KDRV_IPE_CCTRL_SEL_ENUM cctrl_sel;      ///< Color control: vdet source selection
	UINT8 vdet_div;                         ///< Vdet weighting. out =  in*vdetdiv>>8
	UINT8 edge_tab[KDRV_IPE_CCTRL_TAB_LEN]; ///< Legal range : 0~255, color control edge adjust table, 128 : NO edge adjust, table size: 24
	UINT8 dds_tab[KDRV_IPE_DDS_TAB_LEN];    ///< Legal range : 0~255, color control weighting table for SatTab, IntTab, and EdgTab, 0 : NO adjust, 32 : 1X adjust weighting, table size: 8
} KDRV_IPE_CCTRL_PARAM;
//@}

/**
    IPE structure - IPE CCTRL PARAM by CT.
*/
//@{
typedef struct {
	BOOL hue_rotate_en;                     ///< Hue adjust rotation option, 0 : NO rotation, 1 : CHUEM[n] LSB 2 bit = 0 : 0 degree rotation, 1 : 90 degree rotation, 2 bit = 2 : 180 degree rotation, 3 : 270 degree rotation
	UINT8 hue_tab[KDRV_IPE_CCTRL_TAB_LEN];  ///< Legal range : 0~255, color control hue adjust table, 128 : NO hue adjust, table size: 24
	INT32 sat_tab[KDRV_IPE_CCTRL_TAB_LEN];  ///< Legal range : -128~127, color control sat adjust table, 0 : NO sat adjust, -128 : Min sat suppress, 127 : Max sat enhance, table size: 24
	INT32 int_tab[KDRV_IPE_CCTRL_TAB_LEN];  ///< Legal range : -128~127, color control int adjust table, 0 : NO int adjust, -128 : Min int suppress, 127 : Max int enhance, table size: 24
} KDRV_IPE_CCTRL_CT_PARAM;
//@}

/**
    IPE structure - IPE CADJ enhance & inverse.
*/
//@{
typedef struct {
	BOOL enable;                ///< enable function
	UINT32 edge_enh_p;          ///< Positive edge enhance weighting. Legal range: 0~1023. 1X = 64.
	UINT32 edge_enh_n;          ///< Negative edge enhance weighting. Legal range: 0~1023. 1X = 64.
	BOOL edge_inv_p_en;         ///< Legal range : 0~1, if EInvP is 1, positive edge(overshoot) is inverted to negative edge(undershoot)
	BOOL edge_inv_n_en;         ///< Legal range : 0~1, if EInvN is 1, negative edge(undershoot) is inverted to positive edge(overshoot)
} KDRV_IPE_CADJ_EE_PARAM;
//@}

/**
    IPE structure - IPE CADJ edge region strength control
*/
//@{
typedef struct {
    BOOL    enable;         ///< Enable region strength control
    UINT8   enh_thin;       ///< Strength for thin kernel, 64 = 1x
    UINT8   enh_robust;     ///< Strength for robust kernel, 64 = 1x

    INT16   slope_flat;     ///< Strength slope of the flat region, 16 bit (s15)
    INT16   slope_edge;     ///< Strength slope of the edge region, 16 bit (s15)  

    UINT8   str_flat;       ///< Strength for flat region, 64 = 1x
    UINT8   str_edge;       ///< Strength for edge region, 64 = 1x
} KDRV_IPE_EDGE_REGION_PARAM;
//@}


/**
    IPE structure - IPE CADJ YC contrast.
*/
//@{
typedef struct {
	BOOL enable;                            ///< enable function
	UINT8 y_con;                            ///< Legal range : 0~255, Y contrast adjust, 128 : NO contrst adjust, Y' = (Y * Ycon) >> 7
	UINT8 c_con;                            ///< Legal range : 0~255, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
	UINT32 cconlut[KDRV_IPE_CCONTAB_LEN];   ///< Legal range : 0~1023, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
	KDRV_IPE_CCONTAB_SEL ccontab_sel;       ///< Legal range : 0~1, saturation mapping selection, 0 : use saturation as x-axis, 1: abs mean of cb, cr

} KDRV_IPE_CADJ_YCCON_PARAM;
//@}

/**
    IPE structure - IPE CADJ CbCr offset.
*/
//@{
typedef struct {
	BOOL enable;                ///< enable function
	UINT8 cb_ofs;               ///< Legal range : 0~255, Cb color offset, 128 : NO color offset
	UINT8 cr_ofs;               ///< Legal range : 0~255, Cr color offset, 128 : NO color offset
} KDRV_IPE_CADJ_COFS_PARAM;
//@}

/**
    IPE structure - IPE noise effect.
*/
//@{
typedef struct {
	BOOL enable;                ///< enable function
	BOOL rand_en_y;             ///< Enable of random noise on Y
	BOOL rand_en_c;             ///< Enable of random noise on C
	UINT32 rand_level_y;        ///< Legal range : 0~7, 0 : NO Y random noise, 7 : MAX Y random noise level
	UINT32 rand_level_c;        ///< Legal range : 0~7, 0 : NO CbCr random noise, 7 : MAX CbCr random noise level
	BOOL rand_reset;            ///< Legal range : 0~1, 0 : No reset for random pattern sequence, 1 : Frame start reset for random pattern seqeunce.
} KDRV_IPE_CADJ_RAND_PARAM;
//@}

/**
    IPE structure - IPE YC fix effect.
*/
//@{
typedef struct {
	BOOL enable;                       ///< enable function
	KDRV_IPE_STR_YTH1_PARAM yth1;      ///< Y threshold setting 1
	KDRV_IPE_STR_YTH2_PARAM yth2;      ///< Y threshold setting 2
	KDRV_IPE_STR_CTH_PARAM cth;        ///< C threshold setting
} KDRV_IPE_CADJ_FIXTH_PARAM;
//@}

/**
    IPE structure - IPE YC mask effect.
*/
//@{
typedef struct {
	BOOL enable;            ///< enable function
	UINT8 y_mask;           ///< Mask of Y channel, range: 0~255
	UINT8 cb_mask;          ///< Mask of CB channel, range: 0~255
	UINT8 cr_mask;          ///< Mask of CR channel, range: 0~255
} KDRV_IPE_CADJ_MASK_PARAM;
//@}

/**
    IPE structure - IPE CSTP effect.
*/
//@{
typedef struct {
	BOOL enable;            ///< enable function
	UINT8 cstp_ratio;       ///< Ratio of color space transform protection
} KDRV_IPE_CSTP_PARAM;
//@}

/**
    IPE structure - IPE 3DCC.
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
	UINT32 cc3d_lut[KDRV_IPE_3DCC_LEN];         ///< 3DCC DRAM input address, table size: 900, UINT32
	KDRV_IPE_3DCCROUND cc3d_rnd;                ///< 3DCC rounding option
} KDRV_IPE_3DCC_PARAM;
//@}

/**
    IPE structure - IPE CADJ Hue adjustment function enable.
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
} KDRV_IPE_CADJ_HUE_PARAM;
//@}

/**
    IPE structure - IPE RGB gamma.
*/
//@{
typedef struct {
	BOOL enable;                                                ///< enable function
	KDRV_IPE_GAMMA_OPTION option;                               ///< select rgb gamma combin or seperate
	UINT32 gamma_lut[KDRV_IPP_RGB_MAX_CH][KDRV_IPE_GAMMA_LEN];  ///< gamma DRAM input address, each channel table size: 129, UINT32
} KDRV_IPE_GAMMA_PARAM;
//@}

/**
    IPE structure - IPE y curve.
*/
//@{
typedef struct {
	BOOL enable;                                        ///< enable function
	KDRV_IPE_YCURVE_SEL ycurve_sel;                     ///< y curve selection
	UINT32 y_curve_lut[KDRV_IPE_YCURVE_LEN];            ///< y curve DRAM input address, table size: 129, UINT32
} KDRV_IPE_YCURVE_PARAM;
//@}

/**
    IPE structure - IPE Defog parameter.
*/
//@{
typedef struct {
	BOOL enable;                                        ///< enable function
	KDRV_IPE_DEFOG_SCAL_PARAM        scalup_param;      ///< defog subin scaling parameters
	KDRV_IPE_DEFOG_IN_BLD_PARAM      input_bld;         ///< defog input blending parameters
	KDRV_IPE_DEFOG_ENV_ESTIMATION    env_estimation;    ///< defog environment estimation settings
	KDRV_IPE_DEFOG_STRENGTH_PARAM    dfg_strength;      ///< defog strength settings
	KDRV_IPE_DEFOG_OUTBLD_PARAM      dfg_outbld;        ///< defog output blending settings
	KDRV_IPE_DEFOG_STCS_RATIO_PARAM  dfg_stcs;          ///< defog statistics
} KDRV_IPE_DEFOG_PARAM;
//@}

/**
    IPE structure - IPE LCE parameter.
*/
//@{
typedef struct {
	BOOL enable;                                ///< enable function
	UINT8 diff_wt_pos;                          ///< Lce positive weight for difference gain, range: 0~ 255, 128 = 1x.
	UINT8 diff_wt_neg;                          ///< Lce negative weight for difference gain, range: 0~ 255, 128 = 1x.
	UINT8 diff_wt_avg;                          ///< Lce center weight for difference gain, range: 0~ 255, 128 = 1x.
	UINT8 lum_wt_lut[KDRV_LCE_LUMA_LEN];        ///< Lce luma weighting table, range: 0~ 255, 64 = 1x
} KDRV_IPE_LCE_PARAM;
//@}

/**
    IPE structure - IPE Edge Debug mode parameter.
*/
//@{
typedef struct {
	BOOL enable;
	KDRV_IPE_EDGEDBG_SEL mode_sel;              ///< Edge debug mode selection, 0: region, 1: kerenl weighting, 2: edge map
} KDRV_IPE_EDGEDBG_PARAM;
//@}

/**
    IPE structure - IPE VA paramater settingss
*/
typedef struct {
	BOOL enable;                                ///< va function enable
	BOOL indep_va_enable;                       ///< independent va function enable

	//filter
	KDRV_IPE_VA_GROUP_INFO group_1;             ///< va group 1 info
	KDRV_IPE_VA_GROUP_INFO group_2;             ///< va group 2 info

	//VA
	BOOL va_out_grp1_2;                         ///< 0 --> output only group 1, 1 --> output group 1 and 2
	UINT32 va_lofs;                             ///< va output lineoffset
	USIZE win_num;                              ///< va window number, 1x1 ~ 8x8

	//independent VA
	KDRV_IPE_VA_INDEP_OPT indep_win[KDRV_IPE_VA_INDEP_NUM]; ///< va independent settings

} KDRV_IPE_VA_PARAM;

/**
    IPE structure - IPE VA IPL window info. settings
*/
typedef struct {
	UINT32 ratio_base;
	USIZE winsz_ratio;                                  ///< va window size of each window, max 511x511
	URECT indep_roi_ratio[KDRV_IPE_VA_INDEP_NUM];       ///< va indep-window size, max 511x511
} KDRV_IPE_VA_WIN_INFO;


/**
    IPE structure - IPE  VA IPL addr settings
*/
typedef struct {
	UINT32 addr_va;                             ///< va output address
} KDRV_IPE_DMA_VA_ADDR;


/**
    IPE structure - IPE  VA result
*/
typedef struct {
	UINT32 buf_g1_h[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g1_v[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g2_h[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g2_v[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g1_h_cnt[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g1_v_cnt[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g2_h_cnt[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
	UINT32 buf_g2_v_cnt[KDRV_IPE_VA_MAX_WINNUM * KDRV_IPE_VA_MAX_WINNUM];
} KDRV_IPE_VA_RST;

/**
    IPE structure - IPE independent VA result
*/
typedef struct {
	UINT32 g1_h[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g1_v[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g2_h[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g2_v[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g1_h_cnt[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g1_v_cnt[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g2_h_cnt[KDRV_IPE_VA_INDEP_NUM];
	UINT32 g2_v_cnt[KDRV_IPE_VA_INDEP_NUM];
} KDRV_IPE_VA_INDEP_RSLT;

/**
    IPE structure - IPE Extend info
*/
//@{
typedef struct {
	UINT32 hstrp_num;   ///< stripe number
	BOOL ime_3dnr_en;   ///< ime 3dnr enable
	BOOL ime_enc_en;    //< ime yuv compress enable
} KDRV_IPE_EXTEND_INFO;
//@}

/**
    KDRV IPE enum - function selection
*/
typedef enum _KDRV_IPE_IQ_FUNC_ {
	KDRV_IPE_IQ_FUNC_RGBLPF             = 0x00000001,
	KDRV_IPE_IQ_FUNC_CC                 = 0x00000002,
	KDRV_IPE_IQ_FUNC_CST                = 0x00000004,
	KDRV_IPE_IQ_FUNC_CCTRL              = 0x00000008,
	KDRV_IPE_IQ_FUNC_CADJ_EE            = 0x00000010,
	KDRV_IPE_IQ_FUNC_CADJ_YCCON         = 0x00000020,
	KDRV_IPE_IQ_FUNC_CADJ_COFS          = 0x00000040,
	KDRV_IPE_IQ_FUNC_CADJ_RAND          = 0x00000080,
	KDRV_IPE_IQ_FUNC_CADJ_HUE           = 0x00000100,
	KDRV_IPE_IQ_FUNC_CADJ_FIXTH         = 0x00000200,
	KDRV_IPE_IQ_FUNC_CADJ_MASK          = 0x00000400,
	KDRV_IPE_IQ_FUNC_CSTP               = 0x00000800,
	KDRV_IPE_IQ_FUNC_GAMMA_RAND         = 0x00001000,
	KDRV_IPE_IQ_FUNC_GAMMA              = 0x00002000,
	KDRV_IPE_IQ_FUNC_YCURVE             = 0x00004000,
	KDRV_IPE_IQ_FUNC_RESERVED           = 0x00008000,
	KDRV_IPE_IQ_FUNC_PFR                = 0x00010000,
	KDRV_IPE_IQ_FUNC_DEFOG              = 0x00020000,
	KDRV_IPE_IQ_FUNC_LCE                = 0x00040000,
	KDRV_IPE_IQ_FUNC_EDGEDBG            = 0x00080000,
	KDRV_IPE_IQ_FUNC_VA                 = 0x00100000,
	KDRV_IPE_IQ_FUNC_VA_INDEP           = 0x00200000,
	KDRV_IPE_IQ_FUNC_EDGE_REGION_STR    = 0x00400000,
	KDRV_IPE_IQ_FUNC_ALL                = 0x007FFFFF,
	ENUM_DUMMY4WORD(KDRV_IPE_IQ_FUNC)
} KDRV_IPE_IQ_FUNC;


/**
    KDRV IPE all IQ parameters
*/
typedef struct _KDRV_IPE_PARAM_IQ_ALL_PARAM_ {
	KDRV_IPE_EEXT_PARAM             *p_eext;
	KDRV_IPE_EEXT_TONEMAP_PARAM     *p_eext_tonemap;
	KDRV_IPE_EDGE_OVERSHOOT_PARAM   *p_edge_overshoot;
	KDRV_IPE_EPROC_PARAM            *p_eproc;
	KDRV_IPE_RGBLPF_PARAM           *p_rgb_lpf;
	KDRV_IPE_PFR_PARAM              *p_pfr;
	KDRV_IPE_CC_EN_PARAM            *p_cc_en;
	KDRV_IPE_CC_PARAM               *p_cc;
	KDRV_IPE_CCM_PARAM              *p_ccm;
	KDRV_IPE_CCTRL_EN_PARAM         *p_cctrl_en;
	KDRV_IPE_CCTRL_PARAM            *p_cctrl;
	KDRV_IPE_CCTRL_CT_PARAM         *p_cctrl_ct;
	KDRV_IPE_CADJ_EE_PARAM          *p_cadj_ee;
	KDRV_IPE_CADJ_YCCON_PARAM       *p_cadj_yccon;
	KDRV_IPE_CADJ_COFS_PARAM        *p_cadj_cofs;
	KDRV_IPE_CADJ_RAND_PARAM        *p_cadj_rand;
	KDRV_IPE_CADJ_HUE_PARAM         *p_cadj_hue;
	KDRV_IPE_CADJ_FIXTH_PARAM       *p_cadj_fixth;
	KDRV_IPE_CADJ_MASK_PARAM        *p_cadj_mask;
	KDRV_IPE_CST_PARAM              *p_cst;
	KDRV_IPE_CSTP_PARAM             *p_cstp;
	KDRV_IPE_GAMYRAND_PARAM         *p_gamy_rand;
	KDRV_IPE_GAMMA_PARAM            *p_gamma;
	KDRV_IPE_YCURVE_PARAM           *p_y_curve;
	KDRV_IPE_DEFOG_PARAM            *p_defog;
	KDRV_IPE_LCE_PARAM              *p_lce;
	KDRV_IPE_SUBIMG_PARAM           *p_subimg;
	KDRV_IPE_EDGEDBG_PARAM          *p_edgedbg;
	KDRV_IPE_VA_PARAM               *p_va;
	KDRV_IPE_EDGE_REGION_PARAM      *p_edge_region_str;
} KDRV_IPE_PARAM_IQ_ALL_PARAM;


/**
    KDRV IPE all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_IPE_PARAM_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;  ///< IPE IQ function enable selection
	BOOL enable;           ///< IPE IQ function enable/disable operation
} KDRV_IPE_PARAM_IPL_FUNC_EN;


#if 0
#endif

/**
    IPE KDRV ITEM, for kdrv_ipe_set/kdrv_ipe_get
*/
typedef enum {
	KDRV_IPE_PARAM_IPL_OPENCFG = 0,     ///< IPE open configuration, data_type = [Set/Get] KDRV_IPE_OPENCFG
	KDRV_IPE_PARAM_IPL_IN_IMG,          ///< IPE input image info, data_type: [Set/Get] KDRV_IPE_IN_IMG_INFO
	KDRV_IPE_PARAM_IPL_IN_ADDR,         ///< IPE input Y/UV address, data_type: [Set/Get] KDRV_IPE_DMA_ADDR_INFO

	KDRV_IPE_PARAM_IPL_OUT_IMG,         ///< IPE output path image info, data_type: [Set/Get] KDRV_IPE_OUT_IMG_INFO
	KDRV_IPE_PARAM_IPL_OUT_ADDR,        ///< IPE output path Y/UV address, This is valid only when output to DRAM, data_type: [Set/Get] KDRV_IPE_DMA_ADDR_INFO

	KDRV_IPE_PARAM_IPL_SUBIN_ADDR,      ///< IPE subimg input address info, data_type: [Set/Get] KDRV_IPE_DMA_SUBIN_ADDR
	KDRV_IPE_PARAM_IPL_SUBOUT_ADDR,     ///< IPE subimg output address info, data_type: [Set/Get] KDRV_IPE_DMA_SUBOUT_ADDR

	KDRV_IPE_PARAM_IPL_OUTPUT_MODE,     ///< IPE dram output mode, data_type: [Set/Get] KDRV_IPE_DRAM_OUTPUT_MODE
	KDRV_IPE_PARAM_IPL_SINGLE_OUT,      ///< IPE dram single out channel, data_type: [Set/Get] KDRV_IPE_DRAM_SINGLE_OUT_EN

	KDRV_IPE_PARAM_IPL_ISR_CB,          ///< IPE external isr cb
	KDRV_IPE_PARAM_IPL_INTER,           ///< IPE interrupt enable, data_type: [Set/Get] KDRV_IPE_INTE
	KDRV_IPE_PARAM_IPL_LOAD,            ///< IPE set load, data_type: [Set] NULL or KDRV_IPE_SET_ITEM,

	KDRV_IPE_PARAM_IQ_EEXT,             ///< IPE Edge extraction parameters, data_type: [Set/Get] KDRV_IPE_EEXT_PARAM
	KDRV_IPE_PARAM_IQ_EEXT_TONEMAP,     ///< IPE Edge tone curve parameters, data type: [Set/Get] KDRV_IPE_EEXT_TONEMAP_PARAM
	KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT,   ///< IPE Edge overshoot reduction parameters, data_type: [Set/Get] KDRV_IPE_EDGE_OVERSHOOT_PARAM
	KDRV_IPE_PARAM_IQ_EPROC,            ///< IPE Edge process parameters, data_type: [Set/Get] KDRV_IPE_EPROC_PARAM
	KDRV_IPE_PARAM_IQ_RGBLPF,           ///< IPE RGB LPF parameters, data_type: [Set/Get] KDRV_IPE_RGBLPF_PARAM
	KDRV_IPE_PARAM_IQ_PFR,              ///< IPE PFR parameters, data_type: [Set/Get] KDRV_IPE_PFR_PARAM

	KDRV_IPE_PARAM_IQ_CC_EN,            ///< IPE Color correction enable, data_type: [Set/Get] KDRV_IPE_CC_EN_PARAM
	KDRV_IPE_PARAM_IQ_CC,               ///< IPE Color correction strength parameters, data_type: [Set/Get] KDRV_IPE_CC_PARAM
	KDRV_IPE_PARAM_IQ_CCM,              ///< IPE Color correction matrix parameters, data_type: [Set/Get] KDRV_IPE_CCM_PARAM
	KDRV_IPE_PARAM_IQ_CCTRL_EN,         ///< IPE Color control enable, data_type: [Set/Get] KDRV_IPE_CCTRL_EN_PARAM
	KDRV_IPE_PARAM_IQ_CCTRL,            ///< IPE Color control parameters, data_type: [Set/Get] KDRV_IPE_CCTRL_PARAM
	KDRV_IPE_PARAM_IQ_CCTRL_CT,         ///< IPE Color control parameters by color temperature, data_type: [Set/Get] KDRV_IPE_CCTRL_CT_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_EE,          ///< IPE Color componenet adjustment: edge enhancement/inversion parameters, data_type: [Set/Get] KDRV_IPE_CADJ_EE_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_YCCON,       ///< IPE Color componenet adjustment: YC contrast parameters, data_type: [Set/Get] KDRV_IPE_CADJ_YCCON_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_COFS,        ///< IPE Color componenet adjustment: color offset parameters, data_type: [Set/Get] KDRV_IPE_CADJ_COFS_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_RAND,        ///< IPE Color componenet adjustment: random noise parameters, data_type: [Set/Get] KDRV_IPE_CADJ_RAND_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_HUE,         ///< IPE Hue adjustment parameters, data_type: [Set/Get] KDRV_IPE_CADJ_HUE_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_FIXTH,       ///< IPE Color componenet adjustment: YC threshold parameters, data_type: [Set/Get] KDRV_IPE_CADJ_FIXTH_PARAM
	KDRV_IPE_PARAM_IQ_CADJ_MASK,        ///< IPE Color componenet adjustment: YC mask parameters, data_type: [Set/Get] KDRV_IPE_CADJ_MASK_PARAM
	KDRV_IPE_PARAM_IQ_CST,              ///< IPE Color space transform parameters, data_type: [Set/Get] KDRV_IPE_CST_PARAM
	KDRV_IPE_PARAM_IQ_CSTP,             ///< IPE Color space transform protection parameters, data_type: [Set/Get] KDRV_IPE_CSTP_PARAM

	KDRV_IPE_PARAM_IQ_GAMYRAND,         ///< IPE Gamma and Y curve: random output parameters parameters, data_type: [Set/Get] KDRV_IPE_GAMYRAND_PARAM
	KDRV_IPE_PARAM_IQ_GAMMA,            ///< IPE Gamma curve parameters, data_type: [Set/Get] KDRV_IPE_GAMMA_PARAM
	KDRV_IPE_PARAM_IQ_YCURVE,           ///< IPE Y curve parameters, data_type: [Set/Get] KDRV_IPE_YCURVE_PARAM

	KDRV_IPE_PARAM_IQ_SUBIMG,           ///< IPE subimg size info, data_type: [Set/Get] KDRV_IPE_SUBIMG_PARAM
	KDRV_IPE_PARAM_IQ_DEFOG,            ///< IPE Defog parameters, data_type: [Set/Get] KDRV_IPE_DEFOG_PARAM
	KDRV_IPE_PARAM_IQ_LCE,              ///< IPE LCE parameters, data_type: [Set/Get] KDRV_IPE_LCE_PARAM
	KDRV_IPE_PARAM_IQ_EDGEDBG,          ///< IPE Edge Debug mode parameters, data_type: [Set/Get] KDRV_IPE_EDGEDBG_PARAM

	KDRV_IPE_PARAM_IPL_ETH,             ///< IPE Edge thresholding parameters, data_type: [Set/Get] KDRV_IPE_ETH_INFO

	KDRV_IPE_PARAM_IPL_VA_ADDR,         ///< data_type: [Set/Get] KDRV_IPE_DMA_VA_ADDR
	KDRV_IPE_PARAM_IPL_VA_WIN_INFO,     ///< data_type: [Set/Get] KDRV_IPE_VA_WIN_INFO
	KDRV_IPE_PARAM_IQ_VA_PARAM,         ///< data_type: [Set/Get] KDRV_IPE_VA_PARAM
	KDRV_IPE_PARAM_IPL_VA_RSLT,         ///< data_type:     [Get] KDRV_IPE_VA_RST
	KDRV_IPE_PARAM_IPL_VA_INDEP_RSLT,   ///< data_type:     [Get] KDRV_IPE_VA_INDEP_RSLT
	KDRV_IPE_PARAM_IPL_EDGE_STCS_RSLT,   ///< IPE edge statistics, data_type: [Get] KDRV_IPE_EDGE_STCS
	KDRV_IPE_PARAM_IPL_EDGE_THRES_RSLT,  ///< IPE edge threshold suggest parameters, data_type: [Get] KDRV_IPE_EDGE_THRES_AUTO
	KDRV_IPE_PARAM_IPL_DEFOG_STCS_RSLT,  ///< IPE defog statistics, data_type: [Get] KDRV_IPE_DEFOG_STCS

	KDRV_IPE_PARAM_IPL_EXTEND_INFO,     ///< IPE Extend info, data_type: [Set/Get] KDRV_IPE_EXTEND_INFO

	KDRV_IPE_PARAM_IQ_ALL,              ///< IPE all IQ parameters, data_type: [Set    ] KDRV_IPE_PARAM_IQ_ALL_PARAM, default DISABLE
	KDRV_IPE_PARAM_IPL_ALL_FUNC_EN,     ///< IPE all IQ function enable, data_type: [Set    ] KDRV_IPE_PARAM_IPL_FUNC_EN, default DISABLE
	KDRV_IPE_PARAM_IPL_RESET,           // IPE reset
	KDRV_IPE_PARAM_IPL_BUILTIN_DISABLE,    //ipe disable builtin flow control     data_type: [Set    ] NULL
	KDRV_IPE_PARAM_IQ_EDGE_REGION_STR,  ///< IPE edge region strength control parameter, data type[Set/Get] KDRV_IPE_EDGE_REGION_PARAM    
	KDRV_IPE_PARAM_MAX,
	KDRV_IPE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IPE_PARAM_ID)
} KDRV_IPE_PARAM_ID;
STATIC_ASSERT((KDRV_IPE_PARAM_MAX &KDRV_IPE_PARAM_REV) == 0);

#define KDRV_IPE_IGN_CHK KDRV_IPE_PARAM_REV  //only support set/get function


#endif //_KDRV_IPE_H_
