/*
    Public header file for IFE module.

    This file is the header file that define the API and data type for IFE
    module.

    @file       ife_lib.h
    @ingroup    mIDrvIPP_IFE

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _IFE_LIB_680_H
#define _IFE_LIB_680_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
#include <kwrap/nvt_type.h>
#endif

/**
    @addtogroup mIDrvIPP_IFE
*/
//@{

/*
    Define IFE function enable.
*/
//@{
//#define IFE_ROWDEFC_EN                  0x00001000  ///< enable function: row defect concealment
#define IFE_OUTL_EN                     0x00002000  ///< enable function: outlier
#define IFE_FILTER_EN                   0x00004000  ///< enable function: filter
#define IFE_CGAIN_EN                    0x00008000  ///< enable function: color gain
#define IFE_VIG_EN                      0x00010000  ///< enable function: vignette
#define IFE_GBAL_EN                     0x00020000  ///< enable function: Gbalance
#define IFE_RBFILL_EN                   0x00400000  ///< enable function: RB NR Fill (only effective in RGBIr)
#define IFE_CENTERMOD_EN                0x00800000  ///< enable function: center modify
#define IFE_NRS_EN                      0x01000000  ///< enable function: NRS
#define IFE_FCG_EN                      0x02000000  ///< enable function: f_color gain
#define IFE_FUSION_EN                   0x04000000  ///< enable function: sensor HDR
#define IFE_FCURVE_EN                   0x20000000  ///< enable function: F curve
#define IFE_MIRROR_EN                   0x40000000  ///< enable function: mirror
#define IFE_DECODE_EN                   0x80000000  ///< enable function: decode
//#define IFE_FLIP_EN                   0x00200000  ///< removed in NT96680
#define IFE_FUNC_ALL                    0xe7c3e000  ///< all func enable mask
//@}


/**
    Define IFE interrput enable.
*/
//@{
#define IFE_INTE_FRMEND                 0x00000001  ///< frame end interrupt
#define IFE_INTE_DEC1_ERR               0x00000002  ///< frame end interrupt
#define IFE_INTE_DEC2_ERR               0x00000004  ///< frame end interrupt
#define IFE_INTE_LLEND                  0x00000008  ///< LL end interrupt
#define IFE_INTE_LLERR                  0x00000010  ///< LL error  interrupt
#define IFE_INTE_LLERR2                 0x00000020  ///< LL error2 interrupt
#define IFE_INTE_LLJOBEND               0x00000040  ///< LL job end interrupt
#define IFE_INTE_BUFOVFL                0x00000080  ///< buffer overflow interrupt
#define IFE_INTE_RING_BUF_ERR           0x00000100  ///< ring buffer error interrupt
#define IFE_INTE_FRAME_ERR              0x00000200  ///< frame error interrupt (for direct mode)
#define IFE_INTE_SIE_FRM_START          0x00001000  ///< SIE frame start interrupt
//#define IFE_INTE_ROWDEFFAIL             0x00000004  ///< removed in NT96680
#define IFE_INTE_ALL                    0x000013ff  ///< all interrupts
//@}

/**
    Define IFE interrput status.
*/
//@{
#define IFE_INT_FRMEND                 0x00000001  ///< frame end interrupt
#define IFE_INT_DEC1_ERR               0x00000002  ///< frame end interrupt
#define IFE_INT_DEC2_ERR               0x00000004  ///< frame end interrupt
#define IFE_INT_LLEND                  0x00000008  ///< LL end interrupt
#define IFE_INT_LLERR                  0x00000010  ///< LL error  interrupt
#define IFE_INT_LLERR2                 0x00000020  ///< LL error2 interrupt
#define IFE_INT_LLJOBEND               0x00000040  ///< LL job end interrupt
#define IFE_INT_BUFOVFL                0x00000080  ///< buffer overflow interrupt
#define IFE_INT_RING_BUF_ERR           0x00000100  ///< ring buffer error interrupt
#define IFE_INT_FRAME_ERR              0x00000200  ///< frame error interrupt (for direct mode)
#define IFE_INT_SIE_FRM_START          0x00001000  ///< SIE frame start interrupt
//#define IFE_INT_ROWDEFFAIL             0x00000004  ///< removed in NT96680
#define IFE_INT_ALL                    0x000013ff  ///< all interrupts
//@}

/**
    IFE Function Selection.

    IFE Function Selection.
\n  Used for ife_set_mode()
*/
//@{
typedef enum {
	IFE_OPMODE_D2D         = 0,        ///< D2D
	IFE_OPMODE_IPP         = 1,        ///< IPP mode
	IFE_OPMODE_ALL_DIRECT  = 2,        ///< All direct
	ENUM_DUMMY4WORD(IFE_OPMODE)
} IFE_OPMODE;
//@}

/**
    IFE Register type.

    IFE Register type.
\n  Used for ife_change_all()
*/
//@{
typedef enum {
	IFE_ALL_REG         = 0,    ///< Set all registers
	IFE_FD_LATCHED      = 1,    ///< Update VD-latched registers only
	IFE_NON_FD_LATCHED  = 2,    ///< Updat Non-VD-latched registers only
	ENUM_DUMMY4WORD(IFE_REGTYPE)
} IFE_REGTYPE;
//@}

/**
    IFE Function Control.

    Set IFE Function Control
\n  Used for ife_set_mode()
*/
//@{
typedef struct _IFE_CONTROL {

	BOOL            b_decode_en;      ///< RDE decode enable

	BOOL            b_nrs_en;          ///< enable of NRS enable
	BOOL            b_fcurve_en;       ///< enable of Fcurve enable
	BOOL            b_fcgain_en;       ///< enable of F_color gain enable
	BOOL            b_fusion_en;       ///< enable of Fusion (sensor HDR) enable
	BOOL            b_scompression_en; ///< enable of S compression enable

	BOOL            b_outl_en;        ///< enable of outlier filter
	BOOL            b_filter_en;      ///< enable of IFE filter
	BOOL            b_cen_mod_en;      ///< enable of center modify
	BOOL            b_rb_fill_en;      ///< enable RB_NRFill (only effective in RGBIr)
	BOOL            b_cgain_en;       ///< enable of color gain
	BOOL            b_vig_en;         ///< enable of vignette
	BOOL            b_gbal_en;        ///< enable of Gbalance

	BOOL            b_mirror_en;      ///< enable Mirror
} IFE_CONTROL;
//@}

#if 0//removed in NT96680
/**
    IFE Filter mode Selection.

    Set IFE filter mode
\n  Used for ife_set_mode()
*/
//@{
typedef enum {
	IFE_NORMAL_9x9 = 0,           ///< standard filter in 9x9 area
	IFE_SONLY_9x9  = 1,           ///< spatial filter only in 9x9 area
	ENUM_DUMMY4WORD(IFE_FILTMODE)
} IFE_FILTMODE;
//@}
#endif

/**
    IFE binning Selection.

    binning level selection
*/
//@{
typedef enum {
	IFE_BINNING_DISABLE = 0,        ///< Disable binning
	IFE_BINNINGX2       = 1,        ///< binning x2
	IFE_BINNINGX4       = 2,        ///< binning x4
	IFE_BINNINGX8       = 3,        ///< binning x8
	IFE_BINNINGX16      = 4,        ///< binning x16
	IFE_BINNINGX32      = 5,        ///< binning x32
	IFE_BINNINGX64      = 6,        ///< binning x64
	IFE_BINNINGX128     = 7,        ///< binning x128
	ENUM_DUMMY4WORD(IFE_BINNSEL)
} IFE_BINNSEL;
//@}

/**
    IFE CFA Pattern Selection.

    Set IFE start pixel of the CFA pattern
\n  Used for ife_set_mode()
*/
//@{
typedef enum {
	IFE_SONLY_3x3 = 0,           ///< spatial filter only in 3x3 area
	IFE_SONLY_5x5 = 1,           ///< spatial filter only in 5x5 area
	IFE_SONLY_7x7 = 2,           ///< spatial filter only in 7x7 area
	IFE_SONLY_9x9 = 3,           ///< spatial filter only in 9x9 area
	ENUM_DUMMY4WORD(IFE_SONLYLEN)
} IFE_SONLYLEN;
//@}


/**
    IFE Wait Mode Selection.

    IFE Wait Mode Selection.
\n  Used for ife_set_mode()
*/
//@{
typedef enum {
	IFE_WAIT    = 0,                ///< Wait for frame end
	IFE_NOWAIT  = 1,                ///< Do not wait for frame end
	ENUM_DUMMY4WORD(IFE_WAITMODE)
} IFE_WAITMODE;
//@}


/**
    IFE Bit Depth.

    Set IFE input/output bitdepth
*/
//@{
typedef enum {
	IFE_8BIT    = 0,                ///<  8bit data
	IFE_10BIT   = 1,                ///< 10bit data
	IFE_12BIT   = 2,                ///< 12bit data
	IFE_16BIT   = 3,                ///< 16bit data
	ENUM_DUMMY4WORD(IFE_BITDEPTH)
} IFE_BITDEPTH;
//@}

/**
    IFE color gain range selection

    Select IFE color gain range
*/
//@{
typedef enum {
	IFE_CGRANGE_2_8 = 0,    ///< 8bit decimal, set 256 for 1x
	IFE_CGRANGE_3_7 = 1,    ///< 7bit decimal, set 128 for 1x
	ENUM_DUMMY4WORD(IFE_CGRANGE)
} IFE_CGRANGE;
//@}

/**
    IFE input Bayer data start CFA selection

    Select intput Bayer raw data start channel - R, Gr, Gb, B.
*/
//@{
typedef enum {
	IFE_PAT0  = 0,  ///<
	IFE_PAT1  = 1,  ///<
	IFE_PAT2  = 2,  ///<
	IFE_PAT3  = 3,  ///<
	IFE_PAT4  = 4,  ///<
	IFE_PAT5  = 5,  ///<
	IFE_PAT6  = 6,  ///<
	IFE_PAT7  = 7,  ///<
	ENUM_DUMMY4WORD(IFE_CFASEL)
} IFE_CFASEL;
//@}

/**
    IFE input Bayer format selection

    Select intput Bayer format
*/
//@{
typedef enum {
	IFE_BAYER_RGGB  = 0, ///< Traditional RGGB format
	IFE_BAYER_RGBIR = 1, ///< RGBIr format
	ENUM_DUMMY4WORD(IFE_BAYERFMTSEL)
} IFE_BAYERFMTSEL;
//@}

/**
    IFE burst length define
*/
//@{
typedef enum {
	IFE_IN_BURST_64W    = 0,       ///< burst length 64 word
	IFE_IN_BURST_32W    = 1,       ///< burst length 32 word
	ENUM_DUMMY4WORD(IFE_IN_BURST_SEL)
} IFE_IN_BURST_SEL;

/**
    IFE output burst length define
*/
//@{
typedef enum {
	IFE_OUT_BURST_32W    = 0,       ///< burst length 32 word
	IFE_OUT_BURST_16W    = 1,       //removed in NT96680
	ENUM_DUMMY4WORD(IFE_OUT_BURST_SEL)
} IFE_OUT_BURST_SEL;


/**
    IFE SET ENUM
*/
//@{
typedef enum {
	IFE_SET_INFO,
	IFE_SET_WAIT_SIE2,
	IFE_SET_RING_BUF,
	IFE_SET_MIRROR,
	IFE_SET_RDE,
	IFE_SET_NRS,
	IFE_SET_FUSION,
	IFE_SET_FCURVE,
	IFE_SET_OUTL,
	IFE_SET_FILTER,
	IFE_SET_CGAIN,
	IFE_SET_VIG,
	IFE_SET_VIG_CENTER,
	IFE_SET_GBAL,
	IFE_SET_ALL,
	ENUM_DUMMY4WORD(IFE_UPDATE_SEL)
} IFE_SET_SEL;

/**
    IFE hdr wait sie2 first line end or not

    Select IFE hdr wait sie2 first line end or not
*/
//@{
/*typedef enum {
	IFE_HDR_WAIT_SIE2_FRM_ST_ENABLE  = 0,    ///< hdr wait sie2 first line end then start
	IFE_HDR_WAIT_SIE2_FRM_ST_DISABLE = 1,    ///< hdr don't wait sie2 first line end then start
	ENUM_DUMMY4WORD(IFE_HDR_WAIT_SIE2_FRM_ST)
} IFE_HDR_WAIT_SIE2_FRM_ST;*/



typedef struct _IFE_BURST_LENGTH {
	IFE_IN_BURST_SEL   burst_len_input;  ///< Input burst length
	IFE_OUT_BURST_SEL   burst_len_output; ///< Output burst length
} IFE_BURST_LENGTH;
//@}

/**
    Struct IFE open object.

    ISR callback function
*/
//@{
typedef struct _IFE_OPENOBJ {
	void (*FP_IFEISR_CB)(UINT32 ui_int_status); ///< isr callback function
	UINT32 ui_ife_clock_sel; ///< TBD(240 is tested by now)
} IFE_OPENOBJ;
//@}

/**
    Struct IFE stripe

    Parameters of stripe
*/
//@{
typedef struct _IFE_STRIPE {
	UINT32 hn;            ///< range filter threshold
	UINT32 hl;           ///< range filter threshold adjustment
	UINT32 hm;
} IFE_STRIPE;
//@}

/**
    Struct IFE stripe

    Parameters of stripe
*/
//@{
typedef struct _IFE_RING_BUF {
	BOOL   dmaloop_en;      ///< dma loop enable
	UINT32 dmaloop_line;    ///< dma loop dram line number
} IFE_RING_BUF;
//@}


/**
    Struct IFE RDE.

    Parameters of RDE
*/
//@{
typedef struct _IFE_RDESET {
	BOOL            b_degamma_en;               ///<  RDE Degamma enable
	BOOL            b_dithering_en;             ///<  RDE Dithering enable
	UINT8           encode_rate;               ///<  RDE encode rate of bitstream per segment
	BOOL            dither_reset;              ///<  RDE Dithering reset initial random seed

	UINT8           *p_dct_qtable_inx;        ///<  RDE the index for DCT decode Q table
	UINT16          *p_degamma_table;         ///<  RDE degamma table
	UINT8           rand1_init1;              ///<  RDE Random sequence init setting
	UINT16          rand1_init2;              ///<  RDE Random sequence init setting
	UINT8           rand2_init1;              ///<  RDE Random sequence init setting
	UINT16          rand2_init2;              ///<  RDE Random sequence init setting
} IFE_RDESET;
//@}


/**
    Struct IFE NRS.

    Parameters of NRS
*/
//@{
typedef struct _IFE_NRSSET {
	BOOL            b_ord_en;                ///<  NRS Order enable
	BOOL            b_bilat_en;              ///<  NRS Bilateral  enable
	BOOL            b_gbilat_en;             ///<  NRS G channel Bilateral enable
	UINT8           bilat_str;              ///<  NRS Bilateral strength
	UINT8           gbilat_str;             ///<  NRS G channel Bilateral strength
	UINT8           gbilat_w;               ///<  NRS G channel Bilateral weight
	UINT8           ord_range_bri;           ///<  NRS Order bright range
	UINT8           ord_range_dark;          ///<  NRS Order dark range
	UINT16          ord_diff_th;             ///<  NRS Order difference threshold
	UINT8           *p_ord_bri_w;             ///<  NRS Order bright weight
	UINT8           *p_ord_dark_w;            ///<  NRS Order dark weight
	UINT8           *p_bilat_offset;         ///<  NRS Bilateral offset
	UINT8           *p_gbilat_offset;        ///<  NRS G channel Bilateral offset
	UINT8           *p_bilat_weight;         ///<  NRS Bilateral weight
	UINT8           *p_gbilat_weight;        ///<  NRS G channel Bilateral weight
	UINT16          *p_bilat_range;          ///<  NRS Bilateral range
	UINT16          *p_gbilat_range;         ///<  NRS G channel Bilateral range
	UINT8           *p_bilat_th;             ///<  NRS Bilateral threshold
	UINT8           *p_gbilat_th;            ///<  NRS G channel Bilateral threshold
} IFE_NRSSET;
//@}

/**
    Struct IFE Fcurve.

    Parameters of Fcurve
*/
//@{
typedef struct _IFE_FCURVESET {
	UINT8            y_mean_select;      ///<  Fcurve Y Mean Method selection
	UINT8            yv_weight;         ///<  Fcurve Y mean and V weight
	UINT8           *p_y_weight_lut;         ///<  Fcurve Y weight
	UINT8           *p_index_lut;           ///<  Fcurve Index
	UINT8           *p_split_lut;           ///<  Fcurve Split
	UINT16          *p_value_lut;           ///<  Fcurve Value

} IFE_FCURVESET;
//@}

/**
    Struct IFE Fusion.

    Parameters of Fusion
*/
//@{
typedef struct _IFE_FUSIONSET {
	UINT8            y_mean_sel;
	UINT8            nor_blendcur_sel;
	UINT8            diff_blendcur_sel;
	UINT8            mode;
	UINT8            ev_ratio;

	UINT16           mc_lum_th;
	UINT8            mc_diff_ratio;
	UINT8            *p_mc_pos_diff_w;
	UINT8            *p_mc_neg_diff_w;
	UINT8            mc_diff_lumth_diff_w;
	BOOL             b_short_comp_en;
	UINT16           *p_short_comp_knee_point;
	UINT16           *p_short_comp_sub_point;
	UINT8            *p_short_comp_shift_bit;

	UINT16           *p_long_nor_blendcur_knee;
	UINT16           long_nor_blendcur_range;
	UINT8            long_nor_blendcur_wedge;
	UINT16           *p_short_nor_blendcur_knee;
	UINT16           short_nor_blendcur_range;
	UINT8            short_nor_blendcur_wedge;
	UINT16           *p_long_diff_blendcur_knee;
	UINT16           long_diff_blendcur_range;
	UINT8            long_diff_blendcur_wedge;
	UINT16           *p_short_diff_blendcur_knee;
	UINT16           short_diff_blendcur_range;
	UINT8            short_diff_blendcur_wedge;

	UINT16           *p_dark_sat_reduce_th;
	UINT8            *p_dark_sat_reduce_step;
	UINT8            *p_dark_sat_reduce_lowbound;

} IFE_FUSIONSET;
//@}


/**
    Struct IFE Range Filter Setting A.

    Parameters of range filter
*/
//@{
typedef struct _IFE_RANGESETA {
	UINT32 *p_rngth_c0;            ///< range filter threshold
	UINT32 *p_rnglut_c0;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c1;            ///< range filter threshold
	UINT32 *p_rnglut_c1;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c2;            ///< range filter threshold
	UINT32 *p_rnglut_c2;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c3;            ///< range filter threshold
	UINT32 *p_rnglut_c3;           ///< range filter threshold adjustment
} IFE_RANGESETA;
//@}

/**
    Struct IFE Range Filter Advanced Parameter.

    Parameters of range filter
*/
//@{
typedef struct _IFE_CENMODSET {
	UINT32 bilat_th1;      ///< 3x3 bilateral filter threshold 1
	UINT32 bilat_th2;      ///< 3x3 bilateral filter threshold 2
} IFE_CENMODSET;
//@}

/**
    Struct IFE Range Filter Setting B.

    Parameters of range filter
*/
//@{
typedef struct _IFE_RANGESETB {
	UINT32 *p_rngth_c0;            ///< range filter threshold
	UINT32 *p_rnglut_c0;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c1;            ///< range filter threshold
	UINT32 *p_rnglut_c1;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c2;            ///< range filter threshold
	UINT32 *p_rnglut_c2;           ///< range filter threshold adjustment
	UINT32 *p_rngth_c3;            ///< range filter threshold
	UINT32 *p_rnglut_c3;           ///< range filter threshold adjustment
} IFE_RANGESETB;
//@}

/**
    Struct IFE Clamp and Weighting.

    Parameters of clamp and weight
*/
//@{
typedef struct _IFE_CLAMPSET {
	UINT16 clamp_th;             ///< clamp threshold
	UINT8  clamp_mul;            ///< weighting multiplier
	UINT16 clamp_dlt;            ///< clamp threshold adjustment
} IFE_CLAMPSET;
//@}

/**
    Struct IFE Color Gain.

    Parameters of color gain
*/
//@{
typedef struct _IFE_CGAINSET {
	BOOL        b_cgain_inv;         ///< color gain invert
	BOOL        b_cgain_hinv;        ///< color gain H-invert
	IFE_CGRANGE cgain_range;         ///< select color gain decimal range
	UINT32      cgain_mask;          ///< Cloro gain mask
	UINT32      p_cgain[5];          ///< color gain, always R/Gr/Gb/B order
	UINT32      p_cofs[5];           ///< color offset, always R/Gr/Gb/B order
} IFE_CGAINSET;
//@}

/**
    Struct IFE Fusion Color Gain.

    Parameters of fusion color gain
*/
//@{
typedef struct _IFE_FCGAINSET {
	IFE_CGRANGE fcgain_range;                   ///< select Fusion color gain decimal range
	UINT16      p_fusion_cgain_path0[5];          ///< color gain, always R/Gr/Gb/B order
	UINT16      p_fusion_cgain_path1[5];          ///< color gain, always R/Gr/Gb/B order
	UINT16      p_fusion_cofs_path0[5];           ///< color offset, always R/Gr/Gb/B order
	UINT16      p_fusion_cofs_path1[5];           ///< color offset, always R/Gr/Gb/B order
} IFE_FCGAINSET;
//@}

/**
    Struct IFE Outlier Filter.

    Parameters of outlier filter
*/
//@{
typedef struct _IFE_OUTLSET {
	UINT32          *p_bri_th;    ///< outlier threshold for bright points
	UINT32          *p_dark_th;   ///< outlier threshold for dark points
	UINT32          *p_outl_cnt;      ///< outlier counter
	UINT32          outl_w;     ///< outlier weighting
	UINT32          dark_ofs;    ///< outlier dark offset
	UINT32          bright_ofs;  ///< outlier bright offset

	UINT8           ord_range_bri;
	UINT8           ord_range_dark;
	UINT16          ord_protect_th;
	UINT8           ord_blend_w;
	BOOL            b_avg_mode;
	UINT8           *p_ord_bri_w;
	UINT8           *p_ord_dark_w;

	//BOOL            OutlCrsChanEn;  ///< remove in NT 98520
	//IFE_OUTL_SEL    OutlOp;         ///< outlier option for neighbour condition

} IFE_OUTLSET;
//@}

/**
    IFE structure - IFE RB Fill.
*/
//@{
typedef struct _IFE_RBFill_PARAM {
	UINT8 *p_rbfill_rbluma;  ///<
	UINT8 *p_rbfill_rbratio; ///<
	UINT8  rbfill_ratio_mode;
} IFE_RBFill_PARAM;
//@}

/**
    IFE structure - IFE VIG.
*/
//@{
typedef struct _IFE_VIG_PARAM {
	INT32 *p_vig_x;        ///< Legal range : -8192~8191,VIG x center of 4 channel
	INT32 *p_vig_y;        ///< Legal range : -8192~8191,VIG y center of 4 channel
	UINT32 vig_dist_th;       ///< Legal range : 0~1023,VIG disabled area
	UINT8  vig_tab_gain; ///< Legal range : 0~3,VIG gain factor
	BOOL   b_vig_dither_en; ///< VIG dithering enable
	BOOL   b_vig_dither_rst;///< VIG dithering reset, HW clear
	UINT32 vig_xdiv;    ///< Legal range : 0~4095,VIG distance gain factor in x direction. unsigned 6.6
	UINT32 vig_ydiv;    ///< Legal range : 0~4095,VIG distance gain factor in y direction. unsigned 6.6
	UINT32 *p_vig_lut_c0; ///< Legal range : 0~1023,VIG LUT of channel 0
	UINT32 *p_vig_lut_c1; ///< Legal range : 0~1023,VIG LUT of channel 1
	UINT32 *p_vig_lut_c2; ///< Legal range : 0~1023,VIG LUT of channel 2
	UINT32 *p_vig_lut_c3; ///< Legal range : 0~1023,VIG LUT of channel 3

	BOOL    vig_fisheye_gain_en;
	UINT8   vig_fisheye_slope;
	UINT32  vig_fisheye_radius;
} IFE_VIG_PARAM;
//@}

/**
    IFE structure - IFE Gbal.
*/
//@{
typedef struct _IFE_GBAL_PARAM {
	BOOL     b_protect_en;
	UINT16   diff_th_str;
	UINT8    diff_w_max;
	UINT16   edge_protect_th1;
	UINT16   edge_protect_th0;
	UINT8    edge_w_max;
	UINT8    edge_w_min;
	UINT8    *p_gbal_ofs;
} IFE_GBAL_PARAM;
//@}

/**
    IFE structure - IFE NlmKer.
*/
//@{
typedef struct _IFE_NLM_KER {
	BOOL  b_nlm_ker_en;    ///< enable bilateral/NLM kernel modification
	UINT8 ui_ker_slope0;  ///< legal range : 0~127, kernel slope1 = ui_ker_slope0/32
	UINT8 ui_ker_slope1;  ///< legal range : 0~127, kernel slope1 = ui_ker_slope1/4
	BOOL  b_locw_en;      ///< enable bilateral/NLM local weighting modification
	UINT8 ui_bilat_w_d1;   ///< legal range : 0~15, bilateral weighting 1
	UINT8 ui_bilat_w_d2;   ///< legal range : 0~15, bilateral weighting 2
	UINT8 *pui_ker_radius;///< kernel radius
} IFE_NLM_KER;
//@}

/**
    IFE structure - IFE NlmKer Weighting LUT.
*/
//@{
typedef struct _IFE_NLM_LUT {
	UINT8 *pui_mwth;     ///<
	UINT8 *pui_bilat_wa;  ///< legal range : 0~15, weighting table A
	UINT8 *pui_bilat_wb;  ///< legal range : 0~15, weighting table B
	UINT8 *pui_bilat_wc;  ///< legal range : 0~15, weighting table C
	UINT8 *pui_bilat_wbl; ///< weighting table BL
	UINT8 *pui_bilat_wbm; ///< weighting table BM
	UINT8 *pui_bilat_wbh; ///< weighting table BH
} IFE_NLM_LUT;
//@}

/**
    Struct IFE Filter Parameters.

    IFE filter detailed parameters.
*/
//@{
typedef struct _IFE_FILT_PARA {
	IFE_CFASEL      cfa_pat;         ///< CFA pattern if RAW format selected
	//IFE_FILTMODE    filt_mode;       ///< filter mode select
	//BOOL            b_row_def_en;      ///< enable of row defect concealment
	BOOL            b_outl_en;        ///< enable of outlier filter
	BOOL            b_filter_en;      ///< enable of IFE filter
	//BOOL            b_crv_map_en;      ///< enable of curve mapping
	BOOL            b_cgain_en;       ///< enable of color gain
	BOOL            b_vig_en;         ///< enable of vignette
	BOOL            b_gbal_en;        ///< enable of gbalance
	BOOL            b_cen_mod_en;      ///< enable of center modify

	UINT32          ui_bayer_format;  ///< 0: bayer, 1: RGBIr, 2: RGBIr
	//BOOL            b_flip_en;        ///< enable of flip
	UINT32          *p_spatial_weight;    ///< spatial weighting
	IFE_RANGESETA   rng_thA;         ///< range filter threshold setting for filterA
	IFE_RANGESETB   rng_thB;         ///< range filter threshold setting for filterB
	IFE_CENMODSET   center_mod_set;

	//IFE_NLM_KER     nlm_ker_set;      ///< NLM kernel setting
	//IFE_NLM_LUT     nlm_lut_set;      ///< NLM kernel look up tables
	//UINT32          ui_bilatW;       ///< range filter A and B weighting
	UINT32          ui_rthW;         ///< range filter and spatial filter weighting
	IFE_BINNSEL     binn;           ///< image binning
	BOOL            bSOnly_en;       ///< spatial only enable
	IFE_SONLYLEN    SOnly_len;       ///< filter length of spatial only
	IFE_CLAMPSET    clamp_set;     ///< clamp and weighting setting
	IFE_CGAINSET    cgain_set;       ///< color gain setting
	IFE_OUTLSET     outl_set;        ///< outlier filter setting
	IFE_RBFill_PARAM rbfill_set;     ///< R-B filled setting
	//IFE_ROWDEF_FACT row_def_fact;     //removed in NT96680
	UINT32          ui_row_def_st_idx;  ///< start index of row defect table
	UINT32          ui_row_defVIni;   ///< vertical start of cropped window
	UINT32          *pui_row_def_tbl;  ///< table of row defect concealment
	IFE_VIG_PARAM   vig_set;         ///< vignette setting
	IFE_GBAL_PARAM  gbal_set;        ///< gbalance setting
	//UINT8           ui_bit_dither;    ///< select bit dithering while truncation
	UINT32          ui_repeat;       ///< repeat times
} IFE_FILT_PARA;
//@}

/**
    Struct IFE size Parameters.

    IFE Size related parameters.
*/
//@{
typedef struct _IFE_SIZE_PARA {
	UINT32 width;               ///< image width
	UINT32 height;              ///< image height
	UINT32 ui_ofsi_1;              ///< image input lineoffset
	UINT32 ui_ofsi_2;              ///< image input lineoffset
	UINT32 ui_ofso;                ///< image output lineoffset
} IFE_SIZE_PARA;
//@}

/**
    Struct IFE ROI Parameters.

    IFE Address, Cropping detailed parameters.
*/
//@{
typedef struct _IFE_ROI_PARA {
	IFE_SIZE_PARA   size;            ///< input/output size para.
	UINT32          ui_in_addr_1;      ///< input starting address
	UINT32          ui_in_addr_2;      ///< input starting address
	UINT32          ui_out_addr;       ///< output starting address
	UINT32          crop_width;     ///< image crop width
	UINT32          crop_height;    ///< image crop height
	UINT32          crop_hpos;      ///< image horizontal crop start pixel
	UINT32          crop_vpos;      ///< image virtical crop start pixel
	IFE_BITDEPTH    in_bit;           ///< input bit select
	IFE_BITDEPTH    out_bit;          ///< out bit select
	//BOOL            b_flip_en;        ///< enable of flip
} IFE_ROI_PARA;
//@}

/**
    Struct IFE Information.

    IFE parameter and function selection

\n  Used for ife_set_mode()
*/
//@{
typedef struct _IFE_PARAM {

	IFE_OPMODE      mode;           ///< operation mode
	IFE_CFASEL      cfa_pat;         ///< CFA pattern if RAW format selected
	IFE_BITDEPTH    in_bit;          ///< input bit select
	IFE_BITDEPTH    out_bit;         ///< out bit select

	IFE_CONTROL     ife_ctrl;
	BOOL            ife_set_sel[IFE_SET_ALL];

	UINT32          bayer_format;  ///< 0: Bayer, 1: RGBIr, 2: RGBIr2
	IFE_STRIPE      ife_stripe_info;
	IFE_RING_BUF    ife_ringbuf_info;

	UINT32          *p_spatial_weight;    ///< spatial weighting
	IFE_RANGESETA   rng_th_a;         ///< range filter threshold setting for filterA
	IFE_RANGESETB   rng_th_b;         ///< range filter threshold setting for filterB
	IFE_CENMODSET   center_mod_set;

	UINT32          bilat_w;       ///< range filter A and B weighting
	UINT32          rth_w;         ///< range filter and spatial filter weighting
	IFE_BINNSEL     binn;           ///< image binning
	//BOOL            bSOnlyEn;       ///< spatial only enable
	//IFE_SONLYLEN    SOnlyLen;       ///< filter length of spatial only
	IFE_CLAMPSET    clamp_set;       ///< clamp and weighting setting
	IFE_CGAINSET    cgain_set;       ///< color gain setting
	IFE_OUTLSET     outl_set;        ///< outlier filter setting
	IFE_RBFill_PARAM rb_fill_set;     ///< R-B Filled setting

	IFE_RDESET      rde_set;
	IFE_NRSSET      nrs_set;         ///< nrs setting
	IFE_FCURVESET   fcurve_set;      ///< fcurve setting
	IFE_FUSIONSET   fusion_set;      ///< fusion setting
	IFE_FCGAINSET   fcgain_set;      ///< fusion color gain setting

	IFE_VIG_PARAM   vig_set;         ///< vignette setting
	IFE_GBAL_PARAM  gbal_set;        ///< gbalance setting
	//UINT8           uiBitDither;    ///< Select bit dithering while truncation
	UINT32          in_addr0;      ///< input starting address 0
	UINT32          in_addr1;      ///< input starting address 1
	UINT32          out_addr;      ///< output starting address
	UINT32          width;        ///< image width
	UINT32          height;       ///< image height
	UINT32          crop_width;    ///< crop image width
	UINT32          crop_height;   ///< crop image height
	UINT32          crop_hpos;     ///< crop image horizontal start position
	UINT32          crop_vpos;     ///< crop image vertical start position
	UINT32          in_ofs0;       ///< image input lineoffset0
	UINT32          in_ofs1;       ///< image input lineoffset1
	UINT32          out_ofs;       ///< image output lineoffset
	UINT32          intr_en;       ///< interrupt enable
	UINT8           h_shift;      ///< D2D horizontal crop
	UINT8           fusion_number;   ///< Input path
	
	BOOL            b_ife_dma_wait_sie2_st_disable; ///< ife dma wait sie2 start and first line end or not, only active in 528
} IFE_PARAM;
//@}
extern UINT32   ife_fw_power_saving_en;
extern VOID ife_set_builtin_flow_disable(VOID);
extern ER       ife_set_mode(IFE_PARAM *p_filter_info);
//extern ER       ife_change_all(IFE_PARAM *p_filter_info, IFE_REGTYPE reg_type);
//extern ID       ife_get_lock_status(void);
extern ER       ife_open(IFE_OPENOBJ *p_obj_cb);
extern BOOL     ife_is_opened(void);
extern ER       ife_close(void);
extern ER       ife_reset(VOID);
extern ER       ife_enable_int(UINT32 ui_intr);
extern UINT32   ife_get_int_enable(void);
extern UINT32   ife_get_int_status(void);
extern ER       ife_clear_int(UINT32 ui_intr);
extern void     ife_wait_flag_frame_end(void);
extern ER       ife_enable_function(BOOL b_enable, UINT32 ui_func);
extern BOOL     ife_check_function_enable(UINT32 ui_func);
extern IFE_OPMODE ife_check_op_mode(void);
extern UINT32   ife_get_stripe_cnt(void);
extern BOOL     ife_d2d_is_busy(void);
extern ER       ife_change_param(IFE_PARAM *pIFEPara);
//extern void     ife_change_size(IFE_SIZE_PARA *p_size_para);
extern ER       ife_change_roi(IFE_PARAM *pROIPara);
extern ER       ife_start(void);
extern ER       ife_dbg_start(UINT32 loadtype);
extern ER       ife_pause(void);
extern UINT32   ife_get_clock_rate(void);
extern ER       ife_get_burst_length(IFE_BURST_LENGTH *p_burst_len);
//FPGA USE
extern ER ife_set_op_mode(IFE_OPMODE mode);

#if defined (_NVT_EMULATION_)
extern BOOL ife_end_time_out_status;
#endif

//extern void ife_create_resource(void);
//extern void ife_set_base_addr(UINT32 ui_addr);
//extern void ife_isr(void);
#endif

//@}
