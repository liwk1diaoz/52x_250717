/**
    Public header file for kdrv_ife2

    This file is the header file that define the API and data type for kdrv_ife2.

    @file       kdrv_ife2.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KDRV_IFE2_H_
#define _KDRV_IFE2_H_



#define KDRV_IFE2_RANGE_TH_TAB      (3)
#define KDRV_IFE2_RANGE_WT_TAB      (4)
#define KDRV_IFE2_FILTER_TH_TAB     (5)
#define KDRV_IFE2_FILTER_WT_TAB     (6)

/**
    IFE2 KDRV interrupt enable enum.
*/
typedef enum _KDRV_IFE2_INTE {
	KDRV_IFE2_INTE_LL_END  = 0x00000001,
	KDRV_IFE2_INTE_LL_ERR  = 0x00000002,
	KDRV_IFE2_INTE_LL_LATE = 0x00000004,
	KDRV_IFE2_INTE_LL_JEND = 0x00000008,
	KDRV_IFE2_INTE_FRM_ERR = 0x20000000,
	KDRV_IFE2_INTE_OVFL    = 0x40000000,
	KDRV_IFE2_INTE_FMD     = 0x80000000,   ///< enable interrupt: frame end
} KDRV_IFE2_INTE;

/**
    type for KDRV_IFE2_SET_ISRCB
*/
typedef enum {
	KDRV_IFE2_INT_LL_END  = 0x00000001,
	KDRV_IFE2_INT_LL_ERR  = 0x00000002,
	KDRV_IFE2_INT_LL_LATE = 0x00000004,
	KDRV_IFE2_INT_LL_JEND = 0x00000008,
	KDRV_IFE2_INT_FRM_ERR = 0x20000000,
	KDRV_IFE2_INT_OVFL    = 0x40000000,
	KDRV_IFE2_INT_FMD     = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IFE2_ISR_STATUS)
} KDRV_IFE2_ISR_STATUS;

typedef void (*KDRV_IFE2_ISRCB)(KDRV_IFE2_ISR_STATUS, void *);

#if 0
/**
    IFE2 KDRV trigger type
*/
typedef struct _KDRV_IFE2_TRIG_TYPE {
	BOOL    wait_end;           ///< wait ife2 frame end
	UINT32  time_out_ms;        ///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_IFE2_TRIG_TYPE;
#endif

/**
    KDRV IFE2 structure - YUV channel information
*/
typedef struct _KDRV_IFE2_YCC_CH_INFO_ {
	UINT32 width;               ///< 80 <= UINT32 <= 0x3FFF, image width      (pixel)
	UINT32 height;              ///< 80 <= UINT32 <= 0x3FFF, image height     (pixel)
	UINT32 line_ofs;            ///<                         image line offset(byte)
} KDRV_IFE2_YCC_CH_INFO;


//structure data type
/**
    IFE2 structure - ife2 kdrv open object
*/
typedef struct _KDRV_IFE2_OPENCFG_ {
	UINT32 ife2_clock_sel;      ///< Engine clock selection
} KDRV_IFE2_OPENCFG, *pKDRV_IFE2_OPENCFG;

/**
    IFE2 enum - input image data format
*/
typedef enum _KDRV_IFE2_IN_FMT_ {
	KDRV_IFE2_IN_FMT_PACK_YUV444 = 0,    ///< YUVYUV Packing
	KDRV_IFE2_IN_FMT_Y           = 1,    ///< Single Channel
	ENUM_DUMMY4WORD(KDRV_IFE2_IN_FMT)
} KDRV_IFE2_IN_FMT;

/**
    IFE2 KDRV Input Image Information
*/
typedef struct _KDRV_IFE2_IN_INFO_ {
	KDRV_IFE2_IN_FMT         type;   ///< input image data format
	KDRV_IFE2_YCC_CH_INFO    ch;     ///< image channel information
} KDRV_IFE2_IN_INFO;

/**
    IFE2 KDRV Output Image Information
*/
typedef enum _KDRV_IFE2_OUT_DST {
	KDRV_IFE2_OUT_DST_DIRECT = 0,        ///< output direct to engine, IFE --> IME
	KDRV_IFE2_OUT_DST_ALL_DIRECT,    ///< output direct to engine, SIE --> IME
	KDRV_IFE2_OUT_DST_DRAM,              ///< output to dram
	ENUM_DUMMY4WORD(KDRV_IFE2_OUT_DST)
} KDRV_IFE2_OUT_DST;

/**
    IFE2 KDRV Output Image Information
*/
typedef struct _KDRV_IFE2_OUT_IMG_ {
	KDRV_IFE2_OUT_DST    out_dst;    ///< output image destination
	UINT32              line_ofs;    ///< output image line offset
} KDRV_IFE2_OUT_IMG;

/**
    Struct IFE2 Gray Statistical Setting
*/
typedef struct _KDRV_IFE2_GRAY_STATIST_ {
	UINT32 u_th0;           ///< Gray Statistical threshold for U channel
	UINT32 u_th1;           ///< Gray Statistical threshold for U channel
	UINT32 v_th0;           ///< Gray Statistical threshold for V channel
	UINT32 v_th1;           ///< Gray Statistical threshold for V channel
} KDRV_IFE2_GRAY_STATIST;

/**
    Struct IFE2 Gray Average for Cb/Cr channels
*/
typedef struct _KDRV_IFE2_GRAY_AVG_ {
	UINT32 u_avg;           ///< average for U channel
	UINT32 v_avg;           ///< average for V channel
} KDRV_IFE2_GRAY_AVG;

/**
    Struct IFE2 Reference Center Computation Setting
*/
typedef struct _KDRV_IFE2_REFCENT_ {
	UINT32 rng_th[KDRV_IFE2_RANGE_TH_TAB];     ///< UINT32[3], Range filter threshold for reference center
	UINT32 rng_wt[KDRV_IFE2_RANGE_WT_TAB];     ///< UINT32[4], Range filter weighting for reference center
	UINT32 cent_wt;                            ///< Reference center weighting
	UINT32 outl_dth;                           ///< Outlier difference threshold
	UINT32 outl_th;                            ///< Reference center outlier threshold
} KDRV_IFE2_REFCENT;

/**
    Struct IFE2 Reference Center Computation Setting for Y/UV channel
*/
typedef struct _KDRV_IFE2_REFCENT_PARAM_ {
	KDRV_IFE2_REFCENT  refcent_y;        ///< for Y channel
	KDRV_IFE2_REFCENT  refcent_uv;       ///< for UV channel
} KDRV_IFE2_REFCENT_PARAM;

/**
    IFE2 Filter Size Selection
*/
typedef enum _KDRV_IFE2_FILT_SIZE {
	KDRV_IFE2_FLTR_SIZE_3x3 = 0,             ///< 3x3 window
	KDRV_IFE2_FLTR_SIZE_5x5 = 1,             ///< 5x5 window
	KDRV_IFE2_FLTR_SIZE_7x7 = 2,             ///< 7x7 window
	KDRV_IFE2_FLTR_SIZE_9x9 = 3,             ///< 9x9 window
	ENUM_DUMMY4WORD(KDRV_IFE2_FILT_SIZE)
} KDRV_IFE2_FILT_SIZE;

/**
    Struct IFE2 Edge Direction Threshold
*/
typedef struct _KDRV_IFE2_EDG_DIR_TH_PARAM_ {
	UINT32  hv_th;                  ///< Edge direction threshold for two diagonal directions
	UINT32  pn_th;                  ///< Edge direction threshold for horizontal and vertical directions
} KDRV_IFE2_EDG_DIR_TH_PARAM;

/**
    IFE2 Edge Kernel Filter Size Selection
*/
typedef enum _KDRV_IFE2_EDGE_KER_SIZE {
	KDRV_IFE2_EKNL_SIZE_3x3 = 0,             ///< 3x3 window
	KDRV_IFE2_EKNL_SIZE_5x5 = 1,             ///< 5x5 window
	ENUM_DUMMY4WORD(KDRV_IFE2_EDGE_KER_SIZE)
} KDRV_IFE2_EDGE_KER_SIZE;

/**
    Struct IFE2 Filter Computation Setting
*/
typedef struct _KDRV_IFE2_FILTER_ {
	UINT32 filt_th[KDRV_IFE2_FILTER_TH_TAB];    ///< UINT32[5], Filter threshold LUT
	UINT32 filt_wt[KDRV_IFE2_FILTER_WT_TAB];    ///< UINT32[6], Filter weighting LUT
} KDRV_IFE2_FILTER;

/**
    IFE2_FILTER
*/
typedef struct _KDRV_IFE2_FILTER_PARAM {
	BOOL                        enable;             ///< filter Y channel enable
	KDRV_IFE2_FILT_SIZE         size;               ///< filter size
	KDRV_IFE2_EDG_DIR_TH_PARAM  edg_dir;            ///< filter edge direction threshold
	KDRV_IFE2_EDGE_KER_SIZE     edg_ker_size;       ///< edge kernel size
	KDRV_IFE2_FILTER        set_y;                  ///< Filter parameters for Y Channel
	KDRV_IFE2_FILTER        set_u;                  ///< Filter parameters for U Channel
	KDRV_IFE2_FILTER        set_v;                  ///< Filter parameters for V Channel
} KDRV_IFE2_FILTER_PARAM;


/* struct  _KDRV_IFE2_TRIGGER_PARAM: structure to describe a job
 *
 * @note For kdrv_ife2_set_trig()
 */
typedef struct _KDRV_IFE2_TRIGGER_PARAM {

} KDRV_IFE2_TRIGGER_PARAM;


/**
    KDRV IFE2 all IQ parameters
*/
typedef struct _KDRV_IFE2_PARAM_IQ_ALL_PARAM_ {
	KDRV_IFE2_GRAY_STATIST      *p_gray_stl;
	KDRV_IFE2_REFCENT_PARAM     *p_ref_cent;
	KDRV_IFE2_FILTER_PARAM      *p_filter;
} KDRV_IFE2_PARAM_IQ_ALL_PARAM;


/**
    KDRV IFE2 enum - function selection
*/
typedef enum _KDRV_IFE2_IQ_FUNC_ {
	KDRV_IFE2_IQ_FUNC_FILTER    = 0x00000001,
	ENUM_DUMMY4WORD(KDRV_IFE2_IQ_FUNC)
} KDRV_IFE2_IQ_FUNC;


/**
    KDRV IFE2 all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_IFE2_PARAM_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;  ///< IFE2 IQ function enable selection
	BOOL   enable;         ///< IFE2 IQ function enable/disable operation
} KDRV_IFE2_PARAM_IPL_FUNC_EN;



#if 0
#endif

/**
    IFE2 KDRV ITEM, for dal_ife2_set/ dal_ife2_get
*/
typedef enum _KDRV_IFE2_PARAM {
	KDRV_IFE2_PARAM_IPL_OPENCFG = 0,        //ife2 open engine info,                data_type: [Set    ] KDRV_IFE2_OPENCFG
	KDRV_IFE2_PARAM_IPL_IN_IMG_INFO,        //ife2 input image info                 data_type: [Set/Get] KDRV_IFE2_IN_INFO
	KDRV_IFE2_PARAM_IPL_IN_ADDR,            //ife2 input address                    data_type: [Set/Get] UINT32
	KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO,       //ife2 output path image info           data_type: [Set/Get] KDRV_IFE2_OUT_IMG
	KDRV_IFE2_PARAM_IPL_OUT_ADDR,           //ife2 output address                   data_type: [Set/Get] UINT32
	KDRV_IFE2_PARAM_IPL_ISR_CB,             //ife2 external isr cb                  data_type: [Set    ] KDRV_IFE2_ISRCB
	KDRV_IFE2_PARAM_IPL_INTER,              //ife2 interruput enable,               data_type: [Set    ] KDRV_IFE2_INTE
	KDRV_IFE2_PARAM_IQ_GRAY_STATIST,        //ife2 IQ parameter (gray statistic)    data_type: [Set/Get] KDRV_IFE2_GRAY_STATIST
	KDRV_IFE2_PARAM_IPL_GRAY_AVG,           //ife2 Cb/Cr channel range average      data_type: [Get    ] KDRV_IFE2_GRAY_AVG
	KDRV_IFE2_PARAM_IQ_REFCENT,             //ife2 IQ parameter (reference center)  data_type: [Set/Get] KDRV_IFE2_REFCENT_PARAM
	KDRV_IFE2_PARAM_IQ_FILTER,              //ife2 IQ parameter (filter)            data_type: [Set/Get] KDRV_IFE2_FILTER_PARAM
	KDRV_IFE2_PARAM_IQ_ALL,                 //ife2 all IQ parameters,               data_type: [Set    ] KDRV_IFE2_PARAM_IQ_ALL_PARAM
	KDRV_IFE2_PARAM_IPL_ALL_FUNC_EN,        //ife2 all IQ function enable           data_type: [Set    ] KDRV_IFE2_PARAM_IPL_FUNC_EN
	KDRV_IFE2_PARAM_IPL_LOAD,               //ife2 directly load parameters         data_type: [Set    ] NULL
	KDRV_IFE2_PARAM_IPL_RESET,              //ife2 channel reset                    data_type: [Set    ] NULL
	KDRV_IFE2_PARAM_IPL_BUILTIN_DISABLE,    //ife2 disable builtin flow control     data_type: [Set    ] NULL
	KDRV_IFE2_PARAM_MAX,
	KDRV_IFE2_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IFE2_PARAM_ID)
} KDRV_IFE2_PARAM_ID;
STATIC_ASSERT((KDRV_IFE2_PARAM_MAX &KDRV_IFE2_PARAM_REV) == 0);

#define KDRV_IFE2_IGN_CHK KDRV_IFE2_PARAM_REV   //only support set/get function

#endif //_KDRV_IFE2_H_