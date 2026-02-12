/**
    IPE module driver

    NT98520 IPE driver extern header file.

    @file       Ipe_lib.h
    @ingroup    mIDrvIPP_IPE
    @note
\n IPE Driver has 4 states (Idel, Ready, Pause, Run)
\n Idle state: No operation, and IPE semaphore is not occupied.
\n Ready state: No operation, IPE parameters are not configured yet, but IPE semaphore is occupied.
\n Pause state: No operation, IPE parameters are already configured for operation, IPE semaphore is occupied.
\n Run state: In operation, some parameters are allowed to be changed dynamically, IPE semaphore is occupied.
\n ipe_open    Get IPE semaphore before operation starts
\n
\n Function list:
\n ipe_open() Get IPE semaphore before operation starts
\n ipe_close()   Release IPE semaphore after operation ends
\n ipe_changeSize()  Set all Dzoom size related params
\n ipe_change_param() Set IQ related params
\n ipe_change_input() Set input address, sync selection, and PPB control
\n ipe_changeOutput()    Set output source, address, and line offset
\n ipe_set_mode() Set all IPE params, including size, IQ param, I/O
\n ipe_start()   Trigger IPE start
\n ipe_pause()   Trigger IPE stop
\n ipe_reset()   SW reset IPE
\n ipe_enableDMAOut()    Turn on/off IPE output DMA channel
\n ipe_clr_frame_end() Clear IPE frame end flag
\n ipe_wait_frame_end()    Wait IPE frame end flag
\n ipe_clrDMAEnd()   Clear IPE DMA done flag
\n ipe_waitDMAEnd()  Wait IPE DMA done flag
\n
\n Dram to Dram usage example:
\n (1)ipe_open()
\n (2)ipe_set_mode()
\n (3)ipe_start()
\n (4)ipe_wait_frame_end()
\n (5)ipe_close()

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#ifndef _IPE_LIB_H
#define _IPE_LIB_H

#define _EMULATION_ 0

#define IPE_DEBUG   0
#define IPE_DEBUG_INFO   0

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
// NOTE: XXXXX
#include <kwrap/nvt_type.h>
//#include    "Type.h"//a header for basic variable type
#endif


/**
    @name Define IPE function enable.
*/
//@{
#define IPE_RGBLPF_EN                  0x00000001  ///< RGB LPF function enable
#define IPE_RGBGAMMA_EN                0x00000002  ///< RGB Gamma function enable
#define IPE_YCURVE_EN                  0x00000004  ///< Y curve function enable
#define IPE_CCR_EN                     0x00000008  ///< Color correction function enable
#define IPE_DEFOG_SUBOUT_EN            0x00000010  ///< Defog subimg output function enable
#define IPE_DEFOG_EN                   0x00000020  ///< Defog function enable
#define IPE_LCE_EN                     0x00000040  ///< Local contrast enhancement function enable
#define IPE_CST_EN                     0x00000080  ///< Color space transform function enable, RGB->YCC
#define IPE_CTRL_EN                    0x00000200  ///< Color control function enable
#define IPE_HADJ_EN                    0x00000400  ///< Hue adjustment function enable
#define IPE_CADJ_EN                    0x00000800  ///< Color component adjust function enable
#define IPE_CADJ_YENH_EN               0x00001000  ///< Color component edge enhance function enable, sub-function of IPE_CADJ_EN
#define IPE_CADJ_YCON_EN               0x00002000  ///< Color component Y contrast adjust function enable, sub-function of IPE_CADJ_EN
#define IPE_CADJ_CCON_EN               0x00004000  ///< Color component CbCr contrast adjust function enable, sub-function of IPE_CADJ_EN
#define IPE_YCTH_EN                    0x00008000  ///< Color component YCbCr substitution function enable, sub-function of IPE_CADJ_EN
#define IPE_CSTP_EN                    0x00010000  ///< Color space transform protect function enable
#define IPE_EDGE_DBG_EN                0x00020000  ///< Edge debug mode function enable
#define IPE_VACC_EN                    0x00080000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN0_EN           0x00100000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN1_EN           0x00200000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN2_EN           0x00400000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN3_EN           0x00800000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN4_EN          0x01000000  ///< Variation accumulation statistics function enable
#define IPE_PFR_EN                     0x02000000  ///< Purple fringe reductin function enable
//@}

/**
    @name Define IPE interrput enable.
*/
//@{
//#define IPE_INTE_IPESTART              0x00000001  ///< Interrupt enable for triggering IPE start
#define IPE_INTE_FRMEND                0x00000002  ///< Interrupt enable for frame done
#define IPE_INTE_ST                    0x00000004  ///< Interrupt enable for current stripe done
#define IPE_INTE_FRMSTART              0x00000008  ///< Interrupt enable for frame start
#define IPE_INTE_YCOUTEND              0x00000010  ///< Interrupt enable for YC DRAM output done
#define IPE_INTE_DMAIN0END             0x00000020  ///< Interrupt enable for loading Gamma LUT done
#define IPE_INTE_DFG_SUBOUT_END        0x00000040  ///< Interrupt enable for defog subimg DRAM output done
#define IPE_INTE_VAOUTEND              0x00000080  ///< Interrupt enable for DRAM VA output done
#define IPE_INTE_LL_DONE               0x00000100  ///< Interrupt enable for Linked List done
#define IPE_INTE_LL_JOB_END            0x00000200  ///< Interrupt enable for Linked List job end
#define IPE_INTE_LL_ERROR              0x00000400  ///< Interrupt enable for Linked List error command
#define IPE_INTE_LL_ERROR2             0x00000800  ///< Interrupt enable for Linked List error in direct mode
#define IPE_INTE_FRAMEERR              0x00001000  ///< Interrupt enable for frame start error in direct mode
#define IPE_INTE_ALL                   0x00001ffe  ///< All interrupt enable
//@}

/**
    @name Define IPE interrput status.
*/
//@{
//#define IPE_INT_IPESTART              0x00000001  ///< Interrupt enable for triggering IPE start
#define IPE_INT_FRMEND                0x00000002  ///< Interrupt status for frame done
#define IPE_INT_ST                    0x00000004  ///< Interrupt status for current stripe done
#define IPE_INT_FRMSTART              0x00000008  ///< Interrupt status for frame start
#define IPE_INT_YCOUTEND              0x00000010  ///< Interrupt status for YC DRAM output done
#define IPE_INT_DMAIN0END             0x00000020  ///< Interrupt status for loading Gamma LUT done
#define IPE_INT_DFG_SUBOUT_END        0x00000040  ///< Interrupt status for defog subimg DRAM output done
#define IPE_INT_VAOUTEND              0x00000080  ///< Interrupt status for DRAM VA output done
#define IPE_INT_LL_DONE               0x00000100  ///< Interrupt status for Linked List done
#define IPE_INT_LL_JOB_END            0x00000200  ///< Interrupt status for Linked List job end
#define IPE_INT_LL_ERROR              0x00000400  ///< Interrupt status for Linked List error command
#define IPE_INT_LL_ERROR2             0x00000800  ///< Interrupt status for Linked List error in direct mode
#define IPE_INT_FRAMEERR              0x00001000  ///< Interrupt status for frame start error in direct mode
#define IPE_INT_ALL                   0x00001ffe  ///< All interrupt enable
//@}

/**
    @name Define IPE engine value.
*/
//@{
//#define IPE_HSIZE_IODIFF     0

//#define VAC_WIN8                        8  ///< VA window number 8
//#define VAC_WIN16                       16  ///< VA window number 16
//#define IPE_VAC_HNF_MINIMUN             2  ///< Minimum VA hnf value
//#define IPE_VACC_VALUE_MSK              0x0003ffff  ///< VA DRAM data, contrast value mask
//#define IPE_VACC_NUM_MSK                0xfffc0000  ///< VA DRAM data, threshold number mask
//#define IPE_MAXVAWORD      512  ///< Maximum VA DRAM output word
//#define IPE_VA_MAXH_WIN8                    1030  ///< Maximum VA horizontal size when VA window number is 8x8
//#define IPE_VA_MAXH_WIN16                  2038  ///< Maximum VA horizontal size when VA window number is 16x16

//#define IPE_MAX_COLOR_OFFSET    255
//#define IPE_MAX_COLOR_GAIN      1023
//@}

/**
    @name Define IPE function setting selection.
*/
//@{
#define IPE_SET_NONE                0x00000000  ///< Select don't update ANY parameters
#define IPE_SET_EDGEEXT             0x00000001  ///< Select to update edge related parameters
#define IPE_SET_EDGEPROC            0x00000002  ///< Select to update edge related parameters
#define IPE_SET_RGBLPF              0x00000004  ///< Select to update RGB LPF parameters
#define IPE_SET_PFR                 0x00000008  ///< Select to update RGB PFR parameters
#define IPE_SET_COLORCORRECT        0x00000010  ///< Select to update color correction parameters
#define IPE_SET_CST                 0x00000020  ///< Select to update color space transform parameters
#define IPE_SET_CCTRL               0x00000040  ///< Select to update color control parameters
#define IPE_SET_CADJ_EENH           0x00000080  ///< Select to update cadj edge enhance parameters
#define IPE_SET_CADJ_EINV           0x00000100  ///< Select to update cadj edge inversion parameters
#define IPE_SET_CADJ_YCCON          0x00000200  ///< Select to update cadj YC contrast parameters
#define IPE_SET_CADJ_COFS           0x00000400  ///< Select to update cadj color offset parameters
#define IPE_SET_CADJ_RAND           0x00000800  ///< Select to update cadj YC random noise parameters
#define IPE_SET_CADJ_FIXTH          0x00001000  ///< Select to update cadj fixed threshold parameters
#define IPE_SET_CADJ_MASK           0x00002000  ///< Select to update cadj YC mask parameters
#define IPE_SET_CSTP                0x00004000  ///< Select to update color space transform protection parameters
#define IPE_SET_DEFOG_SUBOUT        0x00008000  ///<
#define IPE_SET_DEFOG               0x00010000  ///<
#define IPE_SET_LCE                 0x00020000  ///<
#define IPE_SET_GAMMA               0x00040000  ///< Select to update gamma parameters
#define IPE_SET_YCURVE              0x00080000  ///< Select to update y curve parameters
#define IPE_SET_ETH                 0x00100000  ///< Select to update ETH parameters
#define IPE_SET_VA                  0x00200000  ///< Select to update VA parameters
#define IPE_SET_INDEP_VA            0x00400000  ///< Select to update Independent VA parameters
#define IPE_SET_EDGE_DBG            0x00800000  ///< Select to update edge debug parameters
#define IPE_SET_EDGE_REGION_STR     0x01000000  ///< Select to update edge region strength parameters



#define IPE_SET_ALL                 0xffffffff  ///< Select to update ALL IQ parameters
//@}

/**
    @name Enum IPE input control.
*/
//@{

/**
    IPE option of setting registers

\n Used for ipe_change_param().
*/
typedef enum {
	IPE_ALL_REG = 0,           ///< Set all registers
	IPE_FD_LATCHED = 1,        ///< Update FD-latched registers only
	IPE_NON_FD_LATCHED = 2,    ///< Updat Non-FD-latched registers only
	ENUM_DUMMY4WORD(IPE_REGTYPE)
} IPE_REGTYPE;

/**
    IPE load type selection

    Select ipe load type
*/
typedef enum {
	IPE_FRMEND_LOAD = 0,            ///< IPE frame end load
	IPE_START_LOAD = 1,             ///< IPE start load
	IPE_DIRECT_START_LOAD = 2,      ///< IPE start load
	ENUM_DUMMY4WORD(IPE_LOAD_TYPE)
} IPE_LOAD_TYPE;


/**
    IPE DRAM input SRAM update selection

\n Used for ipe_change_param().
*/
typedef enum {
	IPE_READ_LUT = 0,        ///< IPE read SRAM
	DMA_WRITE_LUT = 1,       ///< DMA write SRAM
	CPU_READ_LUT = 2,        ///< CPU read SRAM
	ENUM_DUMMY4WORD(IPE_RW_SRAM_ENUM)
} IPE_RW_SRAM_ENUM;

/**
    IPE Load from DRAM to Gamma SRAM update option

    Select Gamma load option
*/
typedef enum {
	LOAD_GAMMA_R = 0,          ///< Load Gamma R
	LOAD_GAMMA_RGB = 1,        ///< Load Gamma RGB
	LOAD_GAMMA_Y = 2,          ///< Load Y curve
	LOAD_GAMMA_R_Y = 3,        ///< Load Gamma R and then Y curve
	ENUM_DUMMY4WORD(IPE_RWGAM_OPT)
} IPE_RWGAM_OPT;

/**
    IPE mode selection

    Select ipe mode
*/
typedef enum {
	IPE_OPMODE_D2D             = 0,    ///< D2D
	IPE_OPMODE_IPP             = 1,    ///< direct mode, (IFE->)DCE->IPE->IME
	IPE_OPMODE_SIE_IPP         = 2,    ///< all direct mode, SIE->IFE->DCE->IPE->IME
	IPE_OPMODE_UNKNOWN         = 3,    ///< Undefined mode
	ENUM_DUMMY4WORD(IPE_OPMODE)
} IPE_OPMODE;

/**
    IPE dram single output mode selection

    Select dram single output mode
*/
typedef enum {
	IPE_DRAM_OUT_NORMAL            = 0,    ///<
	IPE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(IPE_DRAM_OUTPUT_MODE)
} IPE_DRAM_OUTPUT_MODE;

/**
    @name Enum IPE output control.
*/
//@{

/**
    IPE DRAM output selection

    Select IPE output data type
\n Used for ipe_changeOutput().
*/
typedef enum {
	IPE_ORIGINAL = 0,           ///< original size, use both Y & C channels
	//IPE_SUBOUT = 1,             ///< subsampled out
	IPE_REGION = 2,             ///< edge region, use output C channel
	IPE_ETH = 3,                ///< edge thresholding, use output Y & C channel
	ENUM_DUMMY4WORD(IPE_DMAOSEL)
} IPE_DMAOSEL;

/**
    IPE Direct to IFE2 enable

    Enable IPE direct to IFE2
\n Used for ipe_changeOutput().
*/
typedef enum {
	B_DISABLE = 0,         ///< disable
	B_ENABLE = 1,          ///< enable
	ENUM_DUMMY4WORD(IPE_ENABLE)
} IPE_ENABLE;


/**
    IPE output YCC format selection

    Select IPE YCC output format
\n Used for ipe_changeOutput().
*/
typedef enum {
	IPE_YCC444 = 0,         ///< Select YCC 424 format
	IPE_YCC422 = 1,         ///< Select YCC 422 format
	IPE_YCC420 = 2,          ///< Select YCC 420 format
	IPE_YONLY = 3,
	ENUM_DUMMY4WORD(IPE_INOUT_FORMAT)
} IPE_INOUT_FORMAT;

/**
    IPE output YCC subsample selection

    Select IPE YCC 422 420 subsample method
\n Used for ipe_changeOutput().
*/
typedef enum {
	YCC_DROP_RIGHT = 0,         ///< When 422 or 420 format is chosen, drop right pixel for subsample
	YCC_DROP_LEFT = 1,          ///< When 422 or 420 format is chosen, drop left pixel for subsample
	ENUM_DUMMY4WORD(IPE_HSUB_SEL_ENUM)
} IPE_HSUB_SEL_ENUM;

//@}

/**
    IPE H overlap selection for MST

    Select IPE H overlap format

\n Used for ipe_change_input().
*/
typedef enum {
	HOLAP_AUTO = 0,         ///< Auto calculation of H overlap pixels
	HOLAP_8PX  = 1,         ///< Keep H overlap to 8 pixels
	ENUM_DUMMY4WORD(IPE_HOVLP_SEL_ENUM)
} IPE_HOVLP_SEL_ENUM;

//@}

/**
    IPE parameter mode
*/
typedef enum {
	IPE_PARAM_AUTO_MODE  = 0,   ///< parameters using auto mode
	IPE_PARAM_USER_MODE  = 1,   ///< parameters using user mode
	ENUM_DUMMY4WORD(IPE_PARAM_MODE)
} IPE_PARAM_MODE;

/**
    IPE color gain range selection

    Select IPE color gain range
\n Used for ipe_change_param().
*/
typedef enum {
	CGRANGE_2_8 = 0,
	CGRANGE_3_7 = 1,
	ENUM_DUMMY4WORD(IPE_CG_RANGE_ENUM)
} IPE_CG_RANGE_ENUM;

/**
    IPE Gamma color channel selection

    Select IPE Gamma color channel
\n Used for ipe_change_param().
*/
typedef enum {
	IPE_R_CHANNEL = 0,      ///< Select R channle for setting
	IPE_G_CHANNEL = 1,     ///< Select G channle for setting
	IPE_B_CHANNEL = 2,      ///< Select B channle for setting
	ENUM_DUMMY4WORD(IPE_CHANNEL_ENUM)
} IPE_CHANNEL_ENUM;

//@}

/**
    @name Enum IPE edge param.
*/
//@{

/**
    IPE edge extract gamma selection

    Select pre or post gamma for edge extract
\n Used for ipe_change_param().
*/
typedef enum {
	EEXT_PRE_GAM = 0,       ///< Select pre-gamma edge extraction
	EEXT_POST_GAM = 1,     ///< Select post-gamma edge extraction
	ENUM_DUMMY4WORD(IPE_EEXT_GAM_ENUM)
} IPE_EEXT_GAM_ENUM;

/**
    IPE edge extract channel selection

    Select G or Y for edge extract
\n Used for ipe_change_param().
*/
typedef enum {
	EEXT_G_CHANNEL = 0,       ///< Select G edge extraction
	EEXT_Y_CHANNEL = 1,     ///< Select Y edge extraction
	ENUM_DUMMY4WORD(IPE_EEXT_CH_ENUM)
} IPE_EEXT_CH_ENUM;

/**
    IPE edge mapping input selection

    Select intput data for edge mapping
\n Used for ipe_change_param().
*/
typedef enum {
	EIN_ENG = 0,            ///< Select edge energy for edge mapping input
	EIN_EEXT = 1,           ///< Select EEXT for edge mapping input
	EIN_AVG = 2,            ///< Select average of EEXT & edge energy for edge mapping input
	ENUM_DUMMY4WORD(IPE_EMAP_IN_ENUM)
} IPE_EMAP_IN_ENUM;

/**
    IPE RGB LPF: kernal size selection

    Select kernel size of low-pass filter

*/
typedef enum {
	LPFSIZE_3X3 = 0,    ///< 3x3 filter size
	LPFSIZE_5X5 = 1,    ///< 5x5 filter size
	LPFSIZE_7X7 = 2,    ///< 7x7 filter size
	ENUM_DUMMY4WORD(IPE_LPF_SIZE_ENUM)
} IPE_LPF_SIZE_ENUM;

/**
    IPE CC range: precision selection

    Select precision 2.8 or 3.7 of color correction

*/
typedef enum {
	CCRANGE_2_9 = 0,    ///< Precision 2.9
	CCRANGE_3_8 = 1,    ///< Precision 3.8
	CCRANGE_4_7 = 1,    ///< Precision 4.7
	ENUM_DUMMY4WORD(IPE_CC_RANGE_ENUM)
} IPE_CC_RANGE_ENUM;

/**
    IPE CC output: weighting reference

    Select identity or gray for output weighting

*/
typedef enum {
	CC2_IDENTITY = 0,   ///< Identity matrix
	CC2_GRAY = 1,       ///< Gray matrix
	ENUM_DUMMY4WORD(IPE_CC2_SEL_ENUM)
} IPE_CC2_SEL_ENUM;

/**
    IPE color correction gamma selection

    Select pre or post gamma for color correction
*/
typedef enum {
	CC_PRE_GAM = 0,       ///< Select pre-gamma color correction
	CC_POST_GAM = 1,      ///< Select post-gamma color correction
	ENUM_DUMMY4WORD(IPE_CC_GAMSEL)
} IPE_CC_GAMSEL;

/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	CC_MAX = 0,             ///< Select max
	CC_MIN = 1,             ///< Select min
	CC_MAX_MINUS_MIN = 2,   ///< Select max-min
	ENUM_DUMMY4WORD(IPE_CC_STABSEL)
} IPE_CC_STABSEL;

/**
    IPE color correction offset option

    Select mode of color correction offset
*/

typedef enum {
	CC_OFS_BYPASS = 0,      ///< Select bypass
	CC_OFS_Y_FULL = 1,      ///< Select offset for full YCC to RGB
	CC_OFS_Y_BTU  = 2,      ///< Select offset for Bt601/Bt709 YCC to RGB
	ENUM_DUMMY4WORD(IPE_CC_OFSSEL)
} IPE_CC_OFSSEL;

/**
    IPE Cb Cr contrast look-up tabel option

    Select mode of Cb Cr contrast look-up tabel
*/

typedef enum {
	SATURATION = 0,         ///<
	ABS_MEAN = 1,           ///<
	ENUM_DUMMY4WORD(IPE_CCONTAB_SEL)
} IPE_CCONTAB_SEL;

/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	CST_NOOP = 0,             ///< No operation
	CST_MINUS128 = 1,         ///< out = in - 128
	ENUM_DUMMY4WORD(IPE_CST_OFFSEL)
} IPE_CST_OFFSEL;

/**
    IPE color control source option

    Select edge reference of color control vdet
*/
typedef enum {
	CCTRL_1STORDER = 0,     ///< 5x5 kernel B output
	CCTRL_E5 = 1,           ///< 5x5 kernel A output
	CCTRL_E7 = 2,           ///< 7x7 kernel output
	ENUM_DUMMY4WORD(IPE_CCTRL_SEL_ENUM)
} IPE_CCTRL_SEL_ENUM;

/**
    IPE YC thresholding: replacement option

    Select replacement with the original or a value specified by the register
*/
typedef enum {
	YCTH_ORIGINAL_VALUE = 0,
	YCTH_REGISTER_VALUE = 1,
	ENUM_DUMMY4WORD(IPE_YCTH_SEL_ENUM)
} IPE_YCTH_SEL_ENUM;

/**
    IPE 3DCC rounding

    Select 3DCC rounding type
*/
typedef enum {
	ROUNDING = 0,             ///< rounding
	HALFTONE = 1,             ///< halftone rounding
	RANDOM = 2,               ///< random rounding
	ENUM_DUMMY4WORD(IPE_DEFOG_ROUNDOPT)
} IPE_DEFOG_ROUNDOPT;

/**
    IPE ETH ouput downsampling

    Select if ETH is downsampled at output
*/
typedef enum {
	ETH_OUT_ORIGINAL = 0,             ///< original size
	ETH_OUT_DOWNSAMPLED = 1,          ///< downsampled by 2x2
	ETH_OUT_BOTH = 2,                 ///< both origianl size and downsample size
	ENUM_DUMMY4WORD(IPE_ETH_OUTSEL)
} IPE_ETH_OUTSEL;

/**
    IPE ETH ouput format

    Select ETH output bitdepth
*/
typedef enum {
	ETH_OUT_2BITS = 0,             ///< 2 bits/pixel
	ETH_OUT_8BITS = 1,             ///< 8 bits/pixel
	ENUM_DUMMY4WORD(IPE_ETH_OUTFMT)
} IPE_ETH_OUTFMT;

/**
    IPE ETH ouput position

    Select ETH output position when downsampling
*/
typedef enum {
	ETH_POS_EVEN = 0,             ///< even position
	ETH_POS_ODD = 1,              ///< odd position
	ENUM_DUMMY4WORD(IPE_ETH_POSSEL)
} IPE_ETH_POSSEL;

typedef enum {
	VA_FLTR_MIRROR  = 0,   ///<
	VA_FLTR_INVERSE = 1,   ///<
	ENUM_DUMMY4WORD(IPE_VA_FLTR_SYMM_SEL)
} IPE_VA_FLTR_SYMM_SEL;

typedef enum {
	VA_FLTR_SIZE_1  = 0,   ///<
	VA_FLTR_SIZE_3  = 1,   ///<
	VA_FLTR_SIZE_5  = 2,   ///<
	VA_FLTR_SIZE_7  = 3,   ///<
	ENUM_DUMMY4WORD(IPE_VA_FLTR_SIZE_SEL)
} IPE_VA_FLTR_SIZE_SEL;


typedef enum {
	VA_OUT_GROUP1  = 0,   ///<
	VA_OUT_BOTH    = 1,   ///<
	ENUM_DUMMY4WORD(IPE_VA_OUTSEL)
} IPE_VA_OUTSEL;

/**
    @name Enum IPE wait frame end selection.
*/
//@{

/**
    IPE wait frame-end selection

    Select clear flag or not before waiting for IPE frame-end
\n Used for ipe_wait_frame_end().
*/
typedef enum {
	IPE_NOCLRFLG = 0,           ///< Select not to clear previous flag before waiting for frame-end
	IPE_CLRFLG = 1,             ///< Select to clear previous flag before waiting for frame-end
	ENUM_DUMMY4WORD(IPE_CLR_FLG_ENUM)
} IPE_CLR_FLG_ENUM;
//@}

/**
    @name Enum IPE function setting mode.
*/
//@{
/**
    IPE function set/enable/disable selection

    Select IPE function update mode
\n Used for ipe_change_param().
*/
typedef enum {
	IPE_FUNC_ENABLE = 0,        ///< Enable functions, FuncEn |= FuncSel, 1s in FuncSel are enabled
	IPE_FUNC_DISABLE = 1,       ///< Disable functions, FuncEn &= (~FuncSel), 1s in FuncSel are disabled
	IPE_FUNC_SET = 2,           ///< Set functions, FuncEn = FuncSel, 1s in FuncSel are enabled while 0s in FuncSel are disabled
	IPE_FUNC_NOUPDATE = 3,      ///< Don't update functions, FuncEn = FuncEn, disregard FuncSel
	ENUM_DUMMY4WORD(IPE_FUNC_UPDATE_ENUM)
} IPE_FUNC_UPDATE_ENUM;

//@}

/**
    @name Enum IPE wait selection.
*/
//@{
/**
    IPE D2D wait frame-end selection

    Select wait frame-end or not for Dram2Dram operation
\n Used for ipe_Dram2Dram().
*/
typedef enum {
	IPE_WAIT = 0,       ///< Wait for frame-end before exit api
	IPE_NOWAIT = 1,     ///< Don't wait for frame-end in api, external ipe_Dram2DramWaitEnd() should be called to make sure frame operation ends
	ENUM_DUMMY4WORD(IPE_WAIT_ENUM)
} IPE_WAIT_ENUM;
//@}



//@{
/**
    IPE burst length selection

    Select input/output burst length mode.
\n Used for ipe_Dram2Dram().
*/
typedef enum {
	IPE_BURST_32W    = 0,       ///< burst length 32 words
	IPE_BURST_16W    = 1,       ///< burst length 16 words
	ENUM_DUMMY4WORD(IPE_BURST_SEL)
} IPE_BURST_SEL;
//@}


/**
    IPE defog target method selection
*/
typedef enum {
	METHOD_A    = 0,       ///<
	METHOD_B    = 1,       ///<
	ENUM_DUMMY4WORD(IPE_DEFOG_METHOD_SEL)
} IPE_DEFOG_METHOD_SEL;

/**
    IPE edge debug mode selection
*/
typedef enum {
	DBG_EDGE_REGION    = 0,       ///<
	DBG_EDGE_WEIGHT    = 1,       ///<
	DBG_EDGE_STRENGTH  = 2,       ///<
	ENUM_DUMMY4WORD(IPE_EDGEDBG_SEL)
} IPE_EDGEDBG_SEL;

/**
    IPE ycc converter format selection
*/
typedef enum {
	RANGE_BT601    = 0,       ///<
	RANGE_BT709    = 1,       ///<
	ENUM_DUMMY4WORD(IPE_YCCCVT_SEL)
} IPE_YCCCVT_SEL;


/**
    IPE ycurve selection
*/
typedef enum {
	YCURVE_RGB_Y     = 0,       ///<
	YCURVE_RGB_V     = 1,       ///<
	YCURVE_YUV       = 2,       ///<
	ENUM_DUMMY4WORD(IPE_YCURVE_SEL)
} IPE_YCURVE_SEL;


//######################################################
// the following for structure define

/**
    IPE structure - IPE input/output burst length
*/
//@{
typedef struct _IPE_BURST_LENGTH {
	IPE_BURST_SEL   burstlen_in_y;  ///< Input burst length
	IPE_BURST_SEL   burstlen_in_c;  ///< Input burst length
	IPE_BURST_SEL   burstlen_out_y; ///< Output burst length
	IPE_BURST_SEL   burstlen_out_c; ///< Output burst length
} IPE_BURST_LENGTH;
//@}

/**
    IPE structure - IPE size.
*/
//@{
typedef struct {
	UINT32    h_size;    ///< Horizontal image size, unit : pix
	UINT32    v_size;    ///< Vertical image size, uint : line
} IPE_IMG_SIZE;
//@}

/**
    IPE structure - IPE stripe.
*/
//@{
typedef struct {
	UINT32    hn;        ///< Size of stripe except for last one, H unit : 16 pix, V unit : 1 line
	UINT32    hl;        ///< Size of last stripe, H unit : 16 pix, V unit : 1 line
	UINT32    hm;        ///< Number of total stripe - 1
	//IPE_OLAP_ENUM    Olap; ///< Overlap enable selection for multi-stripe mode
} STR_STRIPE_INFOR;
//@}

/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	UINT32 eext_enh;            ///< Legal range : 0~63, enhance term for kernel
	INT32 eext_div;             ///< Legal range : -4~3, normalize term for kernel
} EEXT_KER_STR;

/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	EEXT_KER_STR ker_a;         ///< strength of kernal A
	EEXT_KER_STR ker_c;         ///< strength of kernal C
	EEXT_KER_STR ker_d;         ///< strength of kernal D
} IPE_EEXT_KER_STRENGTH;

/**
    IPE structure - IPE edge eext, strength of edge contrast and edge energy
*/
//@{
typedef struct {
	INT32 eext_div_con;         ///< Legal range : -8~7, normalize term for edge contrast
	INT32 eext_div_eng;         ///< Legal range : -8~7, normalize term for edge engergy
	UINT32 wt_con_eng;          ///< Legal range : 0~8, blending weight between edge engergy and edge contrast, 0: all contrast, 8: all edge energy
} IPE_EEXT_ENG_CON;
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
	UINT32 wt_ker_thin;         ///< Legal range : 0~8, Thin kernel weighting between A & B, 8: all thinner kernel A
	UINT32 wt_ker_robust;       ///< Legal range : 0~8, robust kernel weighting between B & C, 8: all thinner kernel B
	UINT32 iso_ker_thin;        ///< Legal range : 0~8, Thin kernel weighting between A_B & C, 8: all thinner kernel A_B
	UINT32 iso_ker_robust;      ///< Legal range : 0~8, robust kernel weighting between B_C & D, 8: all thinner kernel B_C

} IPE_KER_THICKNESS;
//@}

/**
    IPE structure - IPE region segmetation parameters
*/
//@{
typedef struct {
	UINT32 wt_low;              ///< Legal range : 0~16, the kernel weighting of flat region
	UINT32 wt_high;             ///< Legal range : 0~16, the kernel weighting of edge region
	UINT32 th_flat;             ///< Legal range : 0~1023, the threshold for flat region
	UINT32 th_edge;             ///< Legal range : 0~1023, the threshold for edge region

	UINT32 wt_low_hld;          ///< Legal range : 0~16, the kernel weighting of flat region in HLD mode
	UINT32 wt_high_hld;         ///< Legal range : 0~16, the kernel weighting of edge region in HLD mode
	UINT32 th_flat_hld;         ///< Legal range : 0~1023, the threshold for flat region in HLD mode
	UINT32 th_edge_hld;         ///< Legal range : 0~1023, the threshold for edge region in HLD mode
	UINT32 th_lum_hld;          ///< Legal range : 0~1023, the luma threshold in HLD mode

	//slope mode
	IPE_PARAM_MODE region_slope_mode;   ///< input sub-image sclae mode selection

	UINT16 slope_con_eng;       ///< Legal range : 0~65535,
	UINT16 slope_con_eng_hld;   ///< Legal range : 0~65535,

} IPE_REGION_PARAM;
//@}


typedef struct {
    BOOL    region_str_en;      ///< Enable region strength control
    UINT8      enh_thin;        ///< Strength for thin kernel
    UINT8      enh_robust;      ///< Strength for robust kernel

    INT16     slope_flat;      ///< Strength slope of the flat region
    INT16     slope_edge;      ///< Strength slope of the edge region

    UINT8      str_flat;        ///< Strength for flat region
    UINT8      str_edge;        ///< Strength for edge region
} IPE_EDGE_REGION_PARAM;


/**
    IPE structure -
*/
//@{
typedef struct {

	UINT32 localmax_max;
	UINT32 coneng_max;
	UINT32 coneng_avg;

} IPE_EDGE_STCS_RSLT;
//@}


/**
    IPE structure - IPE edge statistic result
*/
//@{
typedef struct {
	BOOL overshoot_en;
	UINT32 wt_overshoot;
	UINT32 wt_undershoot;

	UINT32 th_overshoot;
	UINT32 th_undershoot;
	UINT32 th_undershoot_lum;
	UINT32 th_undershoot_eng;

	UINT32 slope_overshoot;         ///< Legal range : 0~32767,
	UINT32 slope_undershoot;        ///< Legal range : 0~32767,
	UINT32 slope_undershoot_lum;    ///< Legal range : 0~32767,
	UINT32 slope_undershoot_eng;    ///< Legal range : 0~32767,

	UINT32 clamp_wt_mod_lum;
	UINT32 clamp_wt_mod_eng;

	UINT32 strength_lum_eng;
	UINT32 norm_lum_eng;

} IPE_OVERSHOOT_PARAM;
//@}

/**
    IPE structure - IPE edge map
*/
//@{
typedef struct {
	UINT32     ethr_low;                   ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     ethr_high;                  ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     etab_low;                   ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32     etab_high;                  ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
	IPE_EMAP_IN_ENUM ein_sel;                ///< Edge mapping input selection
} IPE_EDGEMAP_INFOR;
//@}

/**
    IPE structure - IPE luminance map
*/
//@{
typedef struct {
	UINT32     ethr_low;                   ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     ethr_high;                  ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     etab_low;                   ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32     etab_high;                  ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
} IPE_ESMAP_INFOR;
//@}


/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	UINT32 lpfw;              ///< LPF weighting
	UINT32 sonly_w;            ///< S-Only weighting
	UINT32 range_th0;          ///< Range threshold 0
	UINT32 range_th1;          ///< Range threshold 1
	IPE_LPF_SIZE_ENUM filt_size;
} STR_RGBLPF_INFOR;
//@}

/**
    IPE structure - IPE PFR color wetting
*/
//@{
typedef struct {
	BOOL enable;
	UINT32 color_u;
	UINT32 color_v;
	UINT32 color_wet_r;
	UINT32 color_wet_b;
	UINT32 cdiff_th;
	UINT32 cdiff_step;
	UINT32 cdiff_lut[5];
} IPE_PFR_COLOR_WET;
//@}


/**
    IPE structure - IPE Y threshold 1
*/
//@{
typedef struct {
	UINT32 y_th;               ///< Y threshold
	UINT32 edgeth;               ///< Edge threshold
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 hit_value;          ///< The register value when hit
	UINT32 nhit_value;         ///< The register value when not-hit
} STR_YTH1_INFOR;
//@}

/**
    IPE structure - IPE Y threshold 2
*/
//@{
typedef struct {
	UINT32 y_th;               ///< Y threshold
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 hit_value;          ///< The register value when hit
	UINT32 nhit_value;         ///< The register value when not-hit
} STR_YTH2_INFOR;
//@}

/**
    IPE structure - IPE C threshold
*/
//@{
typedef struct {
	UINT32 edgeth;               ///< Edge threshold
	UINT32 yth_low;              ///< Low threshold of Y
	UINT32 yth_high;              ///< High threshold of Y
	UINT32 cbth_low;             ///< Low threshold of CB
	UINT32 cbth_high;             ///< High threshold of CB
	UINT32 crth_low;             ///< Low threshold of CR
	UINT32 crth_high;             ///< High threshold of CR
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 cb_hit_value;        ///< The register value of CB when hit
	UINT32 cb_nhit_value;       ///< The register value of CB when not-hit
	UINT32 cr_hit_value;        ///< The register value of CR when hit
	UINT32 cr_nhit_value;       ///< The register value of CR when not-hit
} STR_CTH_INFOR;
//@}

/**
    IPE structure - IPE random input of gamma and y curve
*/
//@{
typedef struct {
	IPE_ENABLE gam_y_rand_en;  ///< Enable of random input
	IPE_ENABLE gam_y_rand_rst; ///< Reset of random input
	UINT8 gam_y_rand_shft;   ///< Shift of random input
} IPE_GAMYRAND;
//@}

/**
    IPE structure - IPE 3DCC rounding
*/
//@{
typedef struct {
	IPE_DEFOG_ROUNDOPT rand_opt;   ///< Rounding option
	IPE_ENABLE rand_rst;           ///< Reset of random input
} IPE_DEFOGROUND;
//@}

/**
    IPE structure - Dram single output channel enable
*/
//@{
typedef struct {
	BOOL single_out_y_en;
	BOOL single_out_c_en;
	BOOL single_out_va_en;
	BOOL single_out_defog_en;
} IPE_DRAM_SINGLE_OUT_EN;
//@}

/**
    IPE structure - Output information of IPE Edge Thresholding
*/
//@{
typedef struct {
	IPE_ETH_OUTSEL eth_outsel;   ///< If ETH output is downsampled
	IPE_ETH_OUTFMT eth_outfmt;   ///< ETH output format
} IPE_ETH_SIZE;
//@}

/**
    IPE structure - Parameters of IPE Edge Thresholding
*/
//@{
typedef struct {
	UINT32 eth_low;              ///< Low edge threshold value
	UINT32 eth_mid;              ///< Middle edge threshold value
	UINT32 eth_high;             ///< High edge threshold value
	IPE_ETH_POSSEL eth_hout;     ///< Horizontal position selection when downsampling
	IPE_ETH_POSSEL eth_vout;     ///< Vertical position selection when downsampling
} IPE_ETH_PARAM;
//@}


typedef struct {
	UINT32              win_stx;    ///< Variation accumulation - horizontal starting point
	UINT32              win_sty;    ///< Variation accumulation - vertical starting point
	UINT32              win_numx;    ///< Variation accumulation - horizontal window number
	UINT32              win_numy;    ///< Variation accumulation - vertical window number
	UINT32              win_szx;    ///< Variation accumulation - horizontal window size
	UINT32              win_szy;    ///< Variation accumulation - vertical window size
	UINT32              win_spx;    ///< Variation accumulation - horizontal window spacing
	UINT32              win_spy;    ///< Variation accumulation - vertical window spacing
} IPE_VA_WIN_PARAM;


typedef struct {
	UINT8                   tap_a;     ///<
	INT8                    tap_b;      ///<
	INT8                    tap_c;      ///<
	INT8                    tap_d;      ///<
	IPE_VA_FLTR_SYMM_SEL    filt_symm;   ///<
	IPE_VA_FLTR_SIZE_SEL    fltr_size;   ///<
	UINT8                   div;      ///<
	UINT16                  th_low;      ///<
	UINT16                  th_high;      ///<
} IPE_VA_FLTR_PARAM;


typedef struct {
	IPE_VA_FLTR_PARAM       filt_h;
	IPE_VA_FLTR_PARAM       filt_v;
	BOOL                    linemax_en;   ///<
	BOOL                    cnt_en;       ///<
} IPE_VA_FLTR_GROUP_PARAM;

typedef struct {
	BOOL                indep_va_en;
	UINT32              win_stx;    ///< horizontal window start
	UINT32              win_sty;    ///< vertical window start
	UINT32              win_szx;    ///< horizontal window size
	UINT32              win_szy;    ///< vertical window size
	BOOL                linemax_g1_en;
	BOOL                linemax_g2_en;
} IPE_INDEP_VA_PARAM;

typedef struct {
	UINT32              va_g1_h;
	UINT32              va_g1_v;
	UINT32              va_g2_h;
	UINT32              va_g2_v;
	UINT32              va_cnt_g1_h;
	UINT32              va_cnt_g1_v;
	UINT32              va_cnt_g2_h;
	UINT32              va_cnt_g2_v;
} IPE_INDEP_VA_WIN_RSLT;

typedef struct {
	UINT32 *p_g1_h;
	UINT32 *p_g1_v;
	UINT32 *p_g2_h;
	UINT32 *p_g2_v;
	UINT32 *p_g1_h_cnt;
	UINT32 *p_g1_v_cnt;
	UINT32 *p_g2_h_cnt;
	UINT32 *p_g2_v_cnt;
} IPE_VA_RSLT;

typedef struct {
	BOOL va_en;
	UINT32 win_num_x;
	UINT32 win_num_y;
	IPE_VA_OUTSEL outsel;
	UINT32 address;
	UINT32 lineoffset;
} IPE_VA_SETTING;

//--------------------------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#endif
//--------------------------------------------------------------------------------------------------------------------------

/**
    IPE structure - IPE edge extraction.
*/
//@{
typedef struct {
	IPE_EEXT_GAM_ENUM eext_gam_sel;            ///< Select pre- or post-gamma edge extraction
	IPE_EEXT_CH_ENUM eext_ch_sel;              ///< Edge input channel selection
	INT16 *p_edgeker_b;                        ///< Edge extraction kernel setting
	IPE_EEXT_KER_STRENGTH eext_kerstrength;    ///<
	IPE_EEXT_ENG_CON eext_engcon;              ///<
	IPE_KER_THICKNESS ker_thickness;           ///<
	IPE_KER_THICKNESS ker_thickness_hld;       ///<
	IPE_REGION_PARAM eext_region;                   ///<
	IPE_OVERSHOOT_PARAM eext_overshoot;             ///<
	UINT32 *p_tonemap_lut;                     ///< Tone remap LUT
} IPE_EEXT_PARAM;
//@}

/**
    IPE structure - IPE edge process.
*/
//@{
typedef struct {
	IPE_EDGEMAP_INFOR *p_edgemap_th;      ///< ED map thresholds
	UINT8 *p_edgemap_lut;                 ///< ED map LUT(16 entries). Legal range: 0~255. 1X = 255
	IPE_ESMAP_INFOR *p_esmap_th;          ///< ES map thresholds
	UINT8 *p_esmap_lut;                   ///< ES map LUT(16 entries). Legal range: 0~255. 1X = 64
} IPE_EPROC_PARAM;
//@}

/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	STR_RGBLPF_INFOR    lpf_r_info;  ///< LPF setting of R channel
	STR_RGBLPF_INFOR    lpf_g_info;  ///< LPF setting of G channel
	STR_RGBLPF_INFOR    lpf_b_info;  ///< LPF setting of B channel
} IPE_RGBLPF_PARAM;
//@}

/**
    IPE structure - IPE PFR
*/
//@{
typedef struct {
	BOOL uv_filt_en;
	BOOL luma_level_en;
	UINT32 wet_out;
	UINT32 edge_th;
	UINT32 edge_strength;
	UINT32 luma_th;
	UINT32 *p_luma_lut;
	UINT32 color_wet_g;
	IPE_PFR_COLOR_WET color_wet_set[4];

} IPE_PFR_PARAM;
//@}


/**
    IPE structure - IPE CC.
*/
//@{
typedef struct {
	IPE_CC_RANGE_ENUM cc_range;   ///< Range of matrix coefficients,0: 3.8 format,1: 4.7format
	IPE_CC2_SEL_ENUM cc2_sel;     ///< Select the other data of CC weighting
	IPE_CC_GAMSEL cc_gam_sel;     ///< Select pre- or post-gamma CC
	IPE_CC_STABSEL cc_stab_sel;   ///< Select CC stab input
	IPE_CC_OFSSEL cc_ofs_sel;     ///< Select CC offset mode //sc
	INT16 *p_cc_coeff;            ///< Color correction matrix [rr rg rb gr gg gb br bg bb]
	UINT8 *p_fstab;               ///< Color correction stab
	UINT8 *p_fdtab;               ///< Color correction dtab
} IPE_CC_PARAM;
//@}

/**
    IPE structure - IPE CST.
*/
//@{
typedef struct {
	INT16 *p_cst_coeff;            ///< Color space transform matrix [yr yg yb ur ug ub vr vg vb]
	IPE_CST_OFFSEL cstoff_sel;    ///< Operation when CST is off
} IPE_CST_PARAM;
//@}

/**
    IPE structure - IPE CCTRL.
*/
//@{
typedef struct {
	INT16 int_ofs;                  ///< Legal range : -128~127, Y offset, Y' = Y + Intofs
	INT16 sat_ofs;                  ///< Legal range : -128~127, color saturation adjust, 0 : NO color saturation adjust
	BOOL hue_rotate_en;                ///< Hue adjust rotation option, 0 : NO rotation, 1 : CHUEM[n] LSB 2 bit = 0 : 0 degree rotation, 1 : 90 degree rotation, 2 bit = 2 : 180 degree rotation, 3 : 270 degree rotation
	BOOL hue_c2g_en;                   ///< Select G ch for hue calculation, 0: input is G channel, 1: input is C channel
	IPE_CCTRL_SEL_ENUM cctrl_sel;    ///< Color control: vdet source selection
	UINT8 vdet_div;                ///< Vdet weighting. out =  in*vdetdiv>>8
	UINT8 *p_hue_tab;               ///< Legal range : 0~255, color control hue adjust table, 128 : NO hue adjust
	INT32 *p_sat_tab;                ///< Legal range : -128~127, color control sat adjust table, 0 : NO sat adjust, -128 : Min sat suppress, 127 : Max sat enhance
	INT32 *p_int_tab;                ///< Legal range : -128~127, color control int adjust table, 0 : NO int adjust, -128 : Min int suppress, 127 : Max int enhance
	UINT8 *p_edge_tab;               ///< Legal range : 0~255, color control edge adjust table, 128 : NO edge adjust
	UINT8 *p_dds_tab;               ///< Legal range : 0~255, color control weighting table for SatTab, IntTab, and EdgTab, 0 : NO adjust, 32 : 1X adjust weighting
} IPE_CCTRL_PARAM;
//@}

/**
    IPE structure - IPE CADJ Eenh.
*/
//@{
typedef struct {
	UINT32 enh_p;         ///< Positive edge enhance weighting. Legal range: 0~1023. 1X = 64.
	UINT32 enh_n;         ///< Negative edge enhance weighting. Legal range: 0~1023. 1X = 64.
} IPE_CADJ_EENH_PARAM;
//@}

/**
    IPE structure - IPE CADJ edge inverse.
*/
//@{
typedef struct {
	BOOL einv_p_en; ///< Legal range : 0~1, if EInvP is 1, positive edge(overshoot) is inverted to negative edge(undershoot)
	BOOL einv_n_en; ///< Legal range : 0~1, if EInvN is 1, negative edge(undershoot) is inverted to positive edge(overshoot)
} IPE_CADJ_EINV_PARAM;
//@}

/**
    IPE structure - IPE CADJ YC contrast.
*/
//@{
typedef struct {
	UINT8 ycon;           ///< Legal range : 0~255, Y contrast adjust, 128 : NO contrst adjust, Y' = (Y * Ycon) >> 7
	UINT8 ccon;           ///< Legal range : 0~255, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
	UINT32 *p_ccon_tab;      ///< Legal range : 0~1024, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
	IPE_CCONTAB_SEL ccontab_sel;    ///< Legal range : 0~1, saturation mapping selection, 0 : use saturation as x-axis, 1: abs mean of cb, cr
} IPE_CADJ_YCCON_PARAM;
//@}

/**
    IPE structure - IPE CADJ CbCr offset.
*/
//@{
typedef struct {
	UINT8 cb_ofs;    ///< Legal range : 0~255, Cb color offset, 128 : NO color offset
	UINT8 cr_ofs;    ///< Legal range : 0~255, Cr color offset, 128 : NO color offset
} IPE_CADJ_COFS_PARAM;
//@}

/**
    IPE structure - IPE noise effect.
*/
//@{
typedef struct {
	IPE_ENABLE yrand_en;     ///< Enable of random noise on Y
	IPE_ENABLE crand_en;     ///< Enable of random noise on C
	UINT32 yrand_level;    ///< Legal range : 0~7, 0 : NO Y random noise, 7 : MAX Y random noise level
	UINT32 crand_level;    ///< Legal range : 0~7, 0 : NO CbCr random noise, 7 : MAX CbCr random noise level
	BOOL yc_rand_rst;        ///< Legal range : 0~1, 0 : No reset for random pattern sequence, 1 : Frame start reset for random pattern seqeunce.
} IPE_CADJ_RAND_PARAM;
//@}

/**
    IPE structure - IPE YC fix effect.
*/
//@{
typedef struct {
	STR_YTH1_INFOR *p_yth1;      ///< Y threshold setting 1
	STR_YTH2_INFOR *p_yth2;      ///< Y threshold setting 2
	STR_CTH_INFOR *p_cth;        ///< C threshold setting
} IPE_CADJ_FIXTH_PARAM;
//@}

/**
    IPE structure - IPE YC mask effect.
*/
//@{
typedef struct {
	UINT8 ymask;          ///< Mask of Y channel
	UINT8 cbmask;         ///< Mask of CB channel
	UINT8 crmask;         ///< Mask of CR channel
} IPE_CADJ_MASK_PARAM;
//@}

/**
    IPE structure - IPE CSTP effect.
*/
//@{
typedef struct {
	UINT8 cstp_rto;      ///< Ratio of color space transform protection
} IPE_CSTP_PARAM;
//@}

/**
    IPE structure - IPE Ycurve.
*/
//@{
typedef struct {
	UINT32 addr_ycurve;          ///< Ycurve DRAM input address
	IPE_YCURVE_SEL ycurve_sel;
} IPE_YCURVE_PARAM;
//@}

/**
    IPE structure - Defog sub-image factor
*/
typedef struct {
	UINT32 factor_h;    ///< horizontal sub-image scale factor
	UINT32 factor_v;    ///< vertical sub-image scale factor
} DEFOG_FACTOR;

/**
    IPE structure - Defog input blending parameters
*/
typedef struct {
	UINT8 *p_in_blend_wt;
} DEFOG_IN_BLD_PARAM;


/**
    IPE structure - Defog scale parameters
*/
typedef struct {
	UINT8              *p_interp_diff_lut;
	UINT8              interp_wdist;
	UINT8              interp_wcenter;
	UINT8              interp_wout;
	UINT8              interp_wsrc;
} DEFOG_SCAL_PARAM;

/**
    IPE structure - Defog output blending parameters
*/
typedef struct {
	IPE_PARAM_MODE      airlight_param_mode;           ///< contrast adjustment
	UINT32              airlight_nfactor;
	BOOL                outbld_ref_sel;
	BOOL                outbld_local_en;
	UINT8               *p_outbld_lum_wt;
	UINT8               *p_outbld_diff_wt;
} DEFOG_OUTBLD_PARAM;

/**
    IPE structure - Defog output blending parameters
*/
typedef struct {
	BOOL                dfg_self_comp_en;
	UINT16              dfg_min_diff;
	UINT16              *p_dfg_airlight;
	UINT16              *p_fog_mod_lut;
} DEFOG_ENV_ESTIMATION;

/**
    IPE structure - Defog strength parameters
*/
typedef struct {
	IPE_DEFOG_METHOD_SEL    str_mode_sel;
	UINT16                  *p_target_lut;
	UINT8                   fog_ratio;
	UINT8                   dgain_ratio;
	UINT8                   gain_th;

} DEFOG_STRENGTH_PARAM;

/**
    IPE structure - Defog statistics
*/
typedef struct {
	UINT32              dfg_stcs_pixelcnt;

} DEFOG_STCS_PARAM;

/**
    IPE structure - Defog statistics result
*/
typedef struct {
	UINT16              airlight[3];

} DEFOG_STCS_RSLT;

/**
    IPE structure - Defog sub-image output
*/
typedef struct {
	IPE_IMG_SIZE        input_size;             ///< Input image H V size,
	IPE_IMG_SIZE        dfg_subimg_size;        ///< defog sub-image size

	IPE_PARAM_MODE      subimg_param_mode;      ///< contrast adjustment sub-image output parameter mode
	IPE_IMG_SIZE        subout_blksize;         ///< contrast adjustment sub-image output block size
	DEFOG_FACTOR        subout_blk_centfact;    ///< contrast adjustment sub-image output block center factors

	DEFOG_FACTOR        subin_scaling_factor;   ///< sub-image scale factor
	UINT8              *p_subimg_ftrcoef;      ///< sub-image low pass filter coefficients, 3 entries

} IPE_DEFOG_SUBIMG_PARAM;

/**
    IPE structure - IPE Defog parameter.
*/
//@{
typedef struct {
	DEFOG_SCAL_PARAM        scalup_param;       ///< defog subin scaling parameters
	DEFOG_IN_BLD_PARAM      input_bld;
	DEFOG_ENV_ESTIMATION    env_estimation;
	DEFOG_STRENGTH_PARAM    dfg_strength;
	DEFOG_OUTBLD_PARAM      dfg_outbld;
	DEFOG_STCS_PARAM        dfg_stcs;
} IPE_DEFOG_PARAM;
//@}

/**
    IPE structure - IPE LCE parameter.
*/
//@{
typedef struct {
	UINT8                  diff_wt_pos;
	UINT8                  diff_wt_neg;
	UINT8                  diff_wt_avg;
	UINT8                  *p_lum_wt_lut;
} IPE_LCE_PARAM;
//@}


//--------------------------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#endif
//--------------------------------------------------------------------------------------------------------------------------

/**
    IPE structure - IPE open object.
*/
//@{
typedef struct {
	void (*FP_IPEISR_CB)(UINT32 intstatus); ///< Call back function pointer to hook with IPE interrupt service routine
	UINT32 ipe_clk_sel;                   ///< IPE engine clock selection
} IPE_OPENOBJ;
//@}

/**
    IPE structure - IPE input info.
*/
//@{
typedef struct {
	IPE_OPMODE ipe_mode;              ///< IPE operation mode
	IPE_IMG_SIZE in_size_y;           ///< Input image H V size, effective if StripeMode is IPE_SSTP or IPE_AUTOSTP. If StripeMode is IPE_MSTP, HSTRIPE and VSTRIPE must be input
	UINT32 addr_in0;                  ///< Legal range : multiples of 4(word aligned), IPE input address ID 0, data type selected by dram_out_sel
	UINT32 addr_in1;                  ///< Legal range : multiples of 4(word aligned), IPE input address ID 1, data type selected by dram_out_sel
	UINT32 addr_in_defog;             ///< Legal range : multiples of 4(word aligned), IPE input address ID 2, for defog subimg
	IPE_INOUT_FORMAT in_fmt;          ///< Input YCC format
	IPE_INOUT_FORMAT out_fmt;         ///< Output YCC format
	UINT32 lofs_in0;                  ///< Legal range : multiples of 4(word aligned), IPE input image DRAM line offset 0, data type selected by dram_out_sel
	UINT32 lofs_in1;                  ///< Legal range : multiples of 4(word aligned), IPE input image DRAM line offset 1, data type selected by dram_out_sel
	UINT32 lofs_in_defog;             ///< Legal range : multiples of 4(word aligned), IPE input image DRAM line offset 2, for defog subimg
	IPE_ENABLE in_rand_en;            ///< Enable of YCC random input
	IPE_ENABLE in_rand_rst;           ///< Reset of YCC random input
	BOOL yc_to_dram_en;               ///< Enable of YC data to DRAM
	BOOL defog_subin_en;              ///< Enable of defog subimg in data to DRAM
	IPE_DMAOSEL dram_out_sel;         ///< DMA output channel data selection
	IPE_ETH_SIZE ethout_info;         ///< ETH output information
	IPE_HOVLP_SEL_ENUM hovlp_opt;     ///< Choose IPE H overlap to fixed 8 pixels (when MST + SQUARE)
} IPE_INPUTINFO;
//@}

/**
    IPE structure - IPE output info.
*/
//@{
typedef struct {
	BOOL yc_to_ime_en;                      ///< Enable of YC data to IME
	BOOL yc_to_dram_en;                     ///< Enable of YC data to DRAM
	BOOL va_to_dram_en;                     ///< Enable of VA data to DRAM, combine with VACC
	BOOL defog_to_dram_en;                  ///< Enable of Defog data to DRAM
	IPE_DRAM_OUTPUT_MODE dram_out_mode;     ///< IPE DRAM output mode selection, 0: normal, 1: single out mode
	IPE_DRAM_SINGLE_OUT_EN single_out_en;   ///< IPE DRAM single output channel enable
	IPE_INOUT_FORMAT yc_format;             ///< IPE DRAM output source data selection
	IPE_HSUB_SEL_ENUM sub_h_sel;            ///< IPE output YCC subsample selection
	UINT32 addr_y;                          ///< Legal range : multiples of 4(word aligned), DRAM output address, data type selected by dram_out_sel
	UINT32 addr_c;                          ///< Legal range : multiples of 4(word aligned), DRAM output address, data type selected by dram_out_sel
	UINT32 addr_va;                         ///< Legal range : multiples of 4(word aligned), VA DRAM output address
	UINT32 addr_defog;                      ///< Legal range : multiples of 4(word aligned), Defog DRAM output address
	UINT32 lofs_y;                          ///< Legal range : multiples of 4(word aligned), DRAM output lineoffset, data type selected by dram_out_sel
	UINT32 lofs_c;                          ///< Legal range : multiples of 4(word aligned), DRAM output lineoffset, data type selected by dram_out_sel
	UINT32 lofs_va;                         ///< Legal range : multiples of 4(word aligned), VA DRAM output lineoffset
	UINT32 lofs_defog;                      ///< Legal range : multiples of 4(word aligned), Defog DRAM output lineoffset

	IPE_DMAOSEL dram_out_sel;               ///< DMA output channel data selection
	IPE_ETH_SIZE ethout_info;               ///< ETH output information
	IPE_VA_OUTSEL va_outsel;                ///< VA output information

} IPE_OUTYCINFO;
//@}

/**
    IPE structure - IPE IQ info.
*/
//@{
typedef struct {
	UINT32 func_sel;                                ///< IPE function set/enable/disable selection, combined usage with func_update_sel
	IPE_FUNC_UPDATE_ENUM func_update_sel;           ///< IPE function set/enable/disable selection
	UINT32 param_update_sel;                        ///< IPE parameters update selection, combined usage with definition IPE_SET_XXX
	IPE_EEXT_PARAM *p_eext_param;                   ///< Edge extraction parameters
	IPE_EPROC_PARAM *p_eproc_param;                 ///< Edge process parameters
	IPE_RGBLPF_PARAM *p_rgblpf_param;               ///< RGB LPF parameters
	IPE_PFR_PARAM *p_pfr_param;                     ///< Purple Fringe Reduction parameters
	IPE_CC_PARAM *p_cc_param;                       ///< Color correction parameters
	IPE_CST_PARAM *p_cst_param;                     ///< Color space transform parameters
	IPE_CCTRL_PARAM *p_cctrl_param;                 ///< Color control parameters
	IPE_CADJ_EENH_PARAM *p_cadj_eenh_param;         ///< Color componenet adjustment: edge enhancement parameters
	IPE_CADJ_EINV_PARAM *p_cadj_einv_param;         ///< Color componenet adjustment: edge inversion parameters
	IPE_CADJ_YCCON_PARAM *p_cadj_yccon_param;       ///< Color componenet adjustment: YC contrast parameters
	IPE_CADJ_COFS_PARAM *p_cadj_cofs_param;         ///< Color componenet adjustment: color offset parameters
	IPE_CADJ_RAND_PARAM *p_cadj_rand_param;         ///< Color componenet adjustment: random noise parameters
	IPE_CADJ_FIXTH_PARAM *p_cadj_fixth_param;       ///< Color componenet adjustment: YC threshold parameters
	IPE_CADJ_MASK_PARAM *p_cadj_mask_param;         ///< Color componenet adjustment: YC mask parameters
	IPE_CSTP_PARAM *p_cstp_param;                   ///< Color space transform protection parameters
	IPE_DEFOG_SUBIMG_PARAM *p_dfg_subimg_param;     ///< Defog subout parameters
	IPE_DEFOG_PARAM *p_defog_param;                 ///< Defog parameters
	IPE_LCE_PARAM *p_lce_param;                     ///< LCE parameters
	IPE_EDGEDBG_SEL edge_dbg_mode_sel;              ///< Edge debug mode selection, 0: region, 1: kerenl weighting
	UINT32 addr_gamma;                              ///< Dram input address of Gamma
	IPE_YCURVE_PARAM *p_ycurve_param;               ///< Ycurve parameters
	IPE_GAMYRAND *p_gamycur_rand;                   ///< Gamma and Y curve: random output parameters
	IPE_DEFOGROUND *p_defog_rand;                   ///< Gamma and Y curve: random output parameters
	IPE_ETH_PARAM *p_eth;                           ///< Edge thresholding parameters
	IPE_VA_FLTR_GROUP_PARAM *p_va_filt_g1;          ///< VA Group 1 filter parameters
	IPE_VA_FLTR_GROUP_PARAM *p_va_filt_g2;          ///< VA Group 2 filter parameters
	IPE_VA_WIN_PARAM    *p_va_win_info;             ///< VA window parameters
	IPE_INDEP_VA_PARAM  p_va_indep_win_info[5];     ///< Independent VA window parameters

	IPE_EDGE_REGION_PARAM *p_edge_region_param;     ///< Edge region strength parameters
} IPE_IQINFO;
//@}

/**
    IPE structure - IPE mode info.
*/
//@{
typedef struct {
	UINT32 intr_en;                 ///< Interrupt enable selection
	IPE_INPUTINFO in_info;           ///< IPE DRAM input related parameters
	IPE_OUTYCINFO out_info;          ///< IPE DRAM output related parameters
	IPE_IQINFO iq_info;              ///< IPE IQ related parameters
} IPE_MODEINFO;
//@}

extern void ipe_clr_frame_end(void);
extern void ipe_wait_frame_end(IPE_CLR_FLG_ENUM clr_flg_sel);
extern void ipe_clr_dma_end(void);
extern void ipe_wait_dma_end(IPE_CLR_FLG_ENUM clr_flg_sel);
extern void ipe_clr_gamma_in_end(void);
extern void ipe_wait_gamma_in_end(void);
extern void ipe_clr_frame_start(void);
extern void ipe_wait_frame_start(IPE_CLR_FLG_ENUM clr_flg_sel);
extern void ipe_clr_ll_done(void);
extern void ipe_wait_ll_done(IPE_CLR_FLG_ENUM clr_flg_sel);
extern void ipe_clr_ll_job_end(void);
extern void ipe_wait_ll_job_end(IPE_CLR_FLG_ENUM clr_flg_sel);

extern ER ipe_open(IPE_OPENOBJ *p_ipe_openinfo);
extern ER ipe_close(void);
// NOTE: XXXXX
//extern UINT32 ipe_get_clock_rate(void);
extern ER ipe_change_param(IPE_IQINFO *p_iq_info);
extern ER ipe_change_interrupt(UINT32 int_en);
extern ER ipe_change_input(IPE_INPUTINFO *p_in_info);
extern ER ipe_change_output(IPE_OUTYCINFO *p_out_info);
extern VOID ipe_set_builtin_flow_disable(VOID);
extern ER ipe_set_mode(IPE_MODEINFO *p_mode_info);
//extern ER ipe_change_all(IPE_MODEINFO *p_mode_info, IPE_REGTYPE reg_type);
extern void ipe_flush_cache(IPE_MODEINFO *p_mode_info);
extern ER ipe_start_linkedlist(void);
extern ER ipe_stop_linkedlist(void);
extern ER ipe_start(void);
extern ER ipe_pause(void);
extern ER ipe_reset(BOOL reset_en);
extern ER ipe_load_gamma(IPE_RWGAM_OPT opt);
extern ER ipe_read_gamma(BOOL cpu_read_en);

extern UINT32 ipe_get_version(void);
extern UINT32 ipe_get_int_status(void);
extern STR_STRIPE_INFOR ipe_get_h_stripe(void);
extern STR_STRIPE_INFOR ipe_get_v_stripe(void);
extern BOOL ipe_check_function_enable(UINT32 ipe_function);
extern UINT32 ipe_get_debug_status(void);
extern void ipe_clear_int(UINT32 intr);
extern BOOL ipe_check_busy(void);
extern UINT32 ipe_get_int_enable(void);
extern ER ipe_get_burst_length(IPE_BURST_LENGTH *p_burstlen);
extern UINT32 ipe_get_va_out_addr(void);
extern UINT32 ipe_get_va_out_lofs(void);
extern BOOL ipe_get_va_outsel(void);
extern void ipe_get_win_info(IPE_VA_WIN_PARAM *p_va_win);
extern IPE_INDEP_VA_PARAM ipe_get_indep_win_info(UINT32 win_idx);
extern void ipe_get_indep_va_win_rslt(IPE_INDEP_VA_WIN_RSLT *p_indepva_rslt, UINT32 win_idx);
extern void ipe_get_va_rslt(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt);
extern void ipe_get_va_result(IPE_VA_SETTING *p_va_info, IPE_VA_RSLT *p_va_rslt);
extern void ipe_get_va_rslt_manual(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt, IPE_VA_WIN_PARAM *p_va_win, UINT32 address, UINT32 lineoffset);
extern void ipe_get_defog_stcs(DEFOG_STCS_RSLT *stcs);
extern void ipe_get_defog_airlight_setting(UINT16 *p_val);
extern void ipe_get_edge_stcs(IPE_EDGE_STCS_RSLT *stcs_result);

//parameter check
extern ER ipe_check_param(IPE_MODEINFO *p_mode_info);
extern ER ipe_check_va_win_info(IPE_VA_WIN_PARAM *p_va_win, IPE_IMG_SIZE size);
extern ER ipe_check_va_lofs(IPE_VA_WIN_PARAM *p_va_win, IPE_OUTYCINFO *out_info);

extern void ipe_set_h_stripe(STR_STRIPE_INFOR hstripe_infor);
extern void ipe_set_v_stripe(STR_STRIPE_INFOR vstripe_infor);

extern UINT16 ipe_int_to_2comp(INT16 val, UINT16 bits);
extern INT16 ipe_2comp_to_int(UINT16 val, UINT16 bits);

#if defined (_NVT_EMULATION_)
//extern void ipe_update_opmode(IPE_OPMODE ipe_opmode_set);
//extern ER ipe_set_mode_fix_state(VOID);
extern BOOL ipe_end_time_out_status;
#endif

#endif
