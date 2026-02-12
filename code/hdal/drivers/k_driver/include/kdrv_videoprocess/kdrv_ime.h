/**
    Public header file for dal_ime

    This file is the header file that define the API and data type for dal_ime.

    @file       kdrv_ime.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KDRV_IME_H_
#define _KDRV_IME_H_

#define KDRV_IME_PQ_NEW     1



#define KDRV_IME_PM_COORD_TAB               (4)
#define KDRV_IME_DBCS_WT_LUT_TAB            (16)
#define KDRV_IME_DS_PLT_TAB                         16
#define KDRV_IME_DS_COLOR_KEY_TAB                   4


#define KDRV_IME_TMNR_ME_SAD_PENALTY_TAB            8
#define KDRV_IME_TMNR_ME_SWITCH_THRESHOLD_TAB       8
#define KDRV_IME_TMNR_ME_DETAIL_PENALTY_TAB         8
#define KDRV_IME_TMNR_ME_PROBABILITY_TAB            8

#define KDRV_IME_TMNR_MD_SAD_COEFA_TAB              8
#define KDRV_IME_TMNR_MD_SAD_COEFB_TAB              8
#define KDRV_IME_TMNR_MD_SAD_STD_TAB                8
#define KDRV_IME_TMNR_MD_FINAL_THRESHOLD_TAB        2

#define KDRV_IME_TMNR_MD_ROI_FINAL_THRESHOLD_TAB    2

#define KDRV_IME_TMNR_MC_SAD_BASE_TAB               8
#define KDRV_IME_TMNR_MC_SAD_COEFA_TAB              8
#define KDRV_IME_TMNR_MC_SAD_COEFB_TAB              8
#define KDRV_IME_TMNR_MC_SAD_STD_TAB                8
#define KDRV_IME_TMNR_MC_FINAL_THRESHOLD_TAB        2

#define KDRV_IME_TMNR_MC_ROI_FINAL_THRESHOLD_TAB    2

#define KDRV_IME_TMNR_PS_MIX_RATIO_TAB              2
#define KDRV_IME_TMNR_PS_MIX_THRESHOLD_TAB          2
#define KDRV_IME_TMNR_PS_MIX_SLOPE_TAB              2
#define KDRV_IME_TMNR_PS_EDGE_THRESHOLD_TAB         2

#define KDRV_IME_TMNR_NR_FREQ_WEIGHT_TAB            4
#define KDRV_IME_TMNR_NR_LUMA_WEIGHT_TAB            8
#define KDRV_IME_TMNR_NR_PRE_FILTER_STRENGTH_TAB    4
#define KDRV_IME_TMNR_NR_PRE_FILTER_RATION_TAB      2
#define KDRV_IME_TMNR_NR_SFILTER_STRENGTH_TAB       3
#define KDRV_IME_TMNR_NR_TFILTER_STRENGTH_TAB       3
#define KDRV_IME_TMNR_NR_LUMA_THRESHOLD_TAB         4
#define KDRV_IME_TMNR_NR_LUMA_LUT_TAB               8
#define KDRV_IME_TMNR_NR_CHROMA_LUT_TAB             8
#define KDRV_IME_TMNR_NR_CHROMA_RATIO_TAB           2







#define KDRV_IME_TPLNR_DETAL_PENALTY_TAB    (6)
#define KDRV_IME_TPLNR_SAD_PENALTY_TAB      (8)
#define KDRV_IME_TPLNR_SNR_RATIO_TAB        (6)
#define KDRV_IME_TPLNR_TNR_RATIO_TAB        (6)
#define KDRV_IME_TPLNR_SQR_STR_GAIN_TAB     (8)
#define KDRV_IME_TPLNR_FREQ_RATIO_TAB       (7)
#define KDRV_IME_TPLNR_SNR_FREQ_TAB         (4)
#define KDRV_IME_TPLNR_TNR_STR_GAIN_TAB     (8)
#define KDRV_IME_TPLNR_MOT_TEN_VAL_TAB      (4)
#define KDRV_IME_TPLNR_MOT_CTRL_TAB         (16)
#define KDRV_IME_TPLNR_2D_WET_TAB           (4)
#define KDRV_IME_TPLNR_3D_WET_TAB           (4)
#define KDRV_IME_TPLNR_MIX_TH_TAB           (4)






/**
    KDRV IME enum - interrupt enable enum.
*/
typedef enum _KDRV_IME_INTE {
	KDRV_IME_INTE_CLR   = 0x00000000,
	KDRV_IME_INTE_LL_END              = 0x00000001,
	KDRV_IME_INTE_LL_ERR              = 0x00000002,
	KDRV_IME_INTE_LL_LATE             = 0x00000004,
	KDRV_IME_INTE_LL_JEND             = 0x00000008,
	KDRV_IME_INTE_BP1                 = 0x00000010,
	KDRV_IME_INTE_BP2                 = 0x00000020,
	KDRV_IME_INTE_BP3                 = 0x00000040,
	KDRV_IME_INTE_TMNR_SLICE_END      = 0x00000080,
	KDRV_IME_INTE_TMNR_MOT_END        = 0x00000100,
	KDRV_IME_INTE_TMNR_MV_END         = 0x00000200,
	KDRV_IME_INTE_TMNR_STA_END        = 0x00000400,
	KDRV_IME_INTE_P1_ENC_OVR          = 0x00000800,
	KDRV_IME_INTE_TMNR_ENC_OVR        = 0x00001000,
	KDRV_IME_INTE_TMNR_DEC_ERR        = 0x00002000,
	KDRV_IME_INTE_FRM_ERR             = 0x00004000,
	KDRV_IME_INTE_FRM_START           = 0x20000000,  ///< frame-start enable
	KDRV_IME_INTE_STRP_END            = 0x40000000,  ///< stripe-end enable
	KDRV_IME_INTE_FRM_END             = 0x80000000,  ///< frame-end enable
	ENUM_DUMMY4WORD(KDRV_IME_INTE)
} KDRV_IME_INTE;

/**
    KDRV IME enum - KDRV_IME_SET_ISRCB status
*/
typedef enum {
	KDRV_IME_INT_LL_END              = 0x00000001,
	KDRV_IME_INT_LL_ERR              = 0x00000002,
	KDRV_IME_INT_LL_LATE             = 0x00000004,
	KDRV_IME_INT_LL_JEND             = 0x00000008,
	KDRV_IME_INT_BP1                 = 0x00000010,
	KDRV_IME_INT_BP2                 = 0x00000020,
	KDRV_IME_INT_BP3                 = 0x00000040,
	KDRV_IME_INT_TMNR_SLICE_END      = 0x00000080,
	KDRV_IME_INT_TMNR_MOT_END        = 0x00000100,
	KDRV_IME_INT_TMNR_MV_END         = 0x00000200,
	KDRV_IME_INT_TMNR_STA_END        = 0x00000400,
	KDRV_IME_INT_P1_ENC_OVR          = 0x00000800,
	KDRV_IME_INT_TMNR_ENC_OVR        = 0x00001000,
	KDRV_IME_INT_TMNR_DEC_ERR        = 0x00002000,
	KDRV_IME_INT_FRM_ERR             = 0x00004000,
	KDRV_IME_INT_FB_FRM_END          = 0x10000000,  ///< fastboot last frame-end
	KDRV_IME_INT_FRM_START           = 0x20000000,  ///< frame-start
	KDRV_IME_INT_STRP_END            = 0x40000000,  ///< stripe-end
	KDRV_IME_INT_FRM_END             = 0x80000000,  ///< frame-end
	ENUM_DUMMY4WORD(KDRV_IME_ISR_STATUS)
} KDRV_IME_ISR_STATUS;


typedef void (*KDRV_IME_ISRCB)(KDRV_IME_ISR_STATUS, void *);

/**
    KDRV IME enum - Flush buffer selection
*/
typedef enum _KDRV_IME_BUF_FLUSH_SEL {
	KDRV_IME_DO_BUF_FLUSH       = 0,    ///< to do flush
	KDRV_IME_NOTDO_BUF_FLUSH    = 1,    ///< not to do flush
	ENUM_DUMMY4WORD(KDRV_IME_BUF_FLUSH_SEL)
} KDRV_IME_BUF_FLUSH_SEL;
//------------------------------------------------------------------


/**
    KDRV IME enum -  low delay mode path selection
*/
typedef enum _KDRV_IME_LOW_DELAY_CHL_SEL_ {
	KDRV_IME_LOW_DELAY_CHL_REFOUT = 0,   ///< IME 3dnr reference output
	KDRV_IME_LOW_DELAY_CHL_PATH1  = 1,   ///< IME output path1
	KDRV_IME_LOW_DELAY_CHL_PATH2  = 2,   ///< IME output path2
	KDRV_IME_LOW_DELAY_CHL_PATH3  = 3,   ///< IME output path3
	ENUM_DUMMY4WORD(KDRV_IME_LOW_DELAY_CHL_SEL)
} KDRV_IME_LOW_DELAY_CHL_SEL;


typedef enum _KDRV_IME_BREAK_POINT_MODE_ {
	KDRV_IME_BP_LINE_MODE  = 0,
	KDRV_IME_BP_PIXEL_MODE = 1,
	ENUM_DUMMY4WORD(KDRV_IME_BREAK_POINT_MODE)
} KDRV_IME_BREAK_POINT_MODE;


/**
    KDRV IME structure - break-point parameter

    support line-based and pixel-based
    pixel-based is only for nt98528
    line-baed is occurred at last stripe
*/
typedef struct _KDRV_IME_BREAK_POINT_INFO_ {
	UINT32 bp1;  ///< break point, unit: line or pixel, default = 0
	UINT32 bp2;  ///< break point, unit: line or pixel, default = 0
	UINT32 bp3;  ///< break point, unit: line or pixel, default = 0

	KDRV_IME_BREAK_POINT_MODE bp_mode;   ///< break-point mode, only for nt98528
} KDRV_IME_BREAK_POINT_INFO;


/**
    KDRV IME structure - low delay mode parameter
*/
typedef struct _KDRV_IME_LOW_DELAY_INFO_ {
	BOOL                  dly_enable;
	KDRV_IME_LOW_DELAY_CHL_SEL dly_ch;
} KDRV_IME_LOW_DELAY_INFO;


/**
    KDRV IME enum - interrupt enable enum.
*/
typedef enum _KDRV_IME_SINGLE_OUT_CHL {
	KDRV_IME_SOUT_LCA_SUBOUT        = 0x00000001,   ///< LCA subout path single output
	KDRV_IME_SOUT_TMNR_REFOUT       = 0x00000002,   ///< TMNR Ref-out path single output
	KDRV_IME_SOUT_TMNR_MS           = 0x00000004,   ///< TMNR motion status single output
	KDRV_IME_SOUT_TMNR_MS_ROI       = 0x00000008,   ///< TMNR motion status of ROI single output
	KDRV_IME_SOUT_TMNR_MV           = 0x00000010,   ///< TMNR motion vector single output
	KDRV_IME_SOUT_TMNR_STA          = 0x00000020,   ///< TMNR statistic data single output
	KDRV_IME_SOUT_PATH1             = 0x00000040,   ///< Output path1 single output
	KDRV_IME_SOUT_PATH2             = 0x00000080,   ///< Output path2 single output
	KDRV_IME_SOUT_PATH3             = 0x00000100,   ///< Output path3 single output
	KDRV_IME_SOUT_PATH4             = 0x00000200,   ///< Output path4 single output
	ENUM_DUMMY4WORD(KDRV_IME_SINGLE_OUT_CHL)
} KDRV_IME_SINGLE_OUT_CHL;

/**
    KDRV IME structure - single output parameter
*/
typedef struct _KDRV_IME_SINGLE_OUT_INFO_ {
	BOOL        sout_enable;    ///< single output enable
	UINT32      sout_chl;       ///< single output channel selection, refer to "KDRV_IME_SINGLE_OUT_CHL"
	///< for example: sout_chl = (KDRV_IME_SOUT_PATH1 | KDRV_IME_SOUT_PATH2 | KDRV_IME_SOUT_PATH3)
} KDRV_IME_SINGLE_OUT_INFO;




/**
    KDRV IME enum - KDRV_IME_IN_INFO inpus source
*/
typedef enum _KDRV_IME_IN_SRC {
	KDRV_IME_IN_SRC_DIRECT = 0,     //input direct from IFE engine
	KDRV_IME_IN_SRC_DRAM,           //input from dram
	KDRV_IME_IN_SRC_ALL_DIRECT,     //input direct from SIE engine
	ENUM_DUMMY4WORD(KDRV_IME_IN_SRC)
} KDRV_IME_IN_SRC;

/**
    KDRV IME enum - Path ID
*/
typedef enum _KDRV_IME_PATH_ID {
	KDRV_IME_PATH_ID_1   = 0,
	KDRV_IME_PATH_ID_2   = 1,
	KDRV_IME_PATH_ID_3   = 2,
	KDRV_IME_PATH_ID_4   = 3,
	KDRV_IME_PATH_ID_5   = 4,
	KDRV_IME_PATH_ID_MAX  = 5,
	ENUM_DUMMY4WORD(KDRV_IME_PATH_ID)
} KDRV_IME_PATH_ID;

/**
    KDRV IME enum - input image data format
*/
typedef enum _KDRV_IME_IN_FMT {
	KDRV_IME_IN_FMT_YUV444 = 0,     ///< YCC444 separate planar format for input
	KDRV_IME_IN_FMT_YUV422,         ///< YCC422 separate planar format for input
	KDRV_IME_IN_FMT_YUV420,         ///< YCC420 separate planar format for input
	KDRV_IME_IN_FMT_Y_PACK_UV422,   ///< YCC422 UV-packing format for input
	KDRV_IME_IN_FMT_Y_PACK_UV420,   ///< YCC420 UV-packing format for input
	KDRV_IME_IN_FMT_Y,              ///< Only Y channel for input
	KDRV_IME_IN_FMT_RGB,            ///< RGB for input
	ENUM_DUMMY4WORD(KDRV_IME_IN_FMT)
} KDRV_IME_IN_FMT;

/**
    KDRV IME enum - Output YCC format
*/
typedef enum _KDRV_IME_OUT_FMT {
	/* 3 channel */
	KDRV_IME_OUT_FMT_YUV444 = 0,     ///< 444 planar
	KDRV_IME_OUT_FMT_YUV422,         ///< 422 planar
	KDRV_IME_OUT_FMT_YUV420,         ///< 420 planar
	/* 2 channel */
	KDRV_IME_OUT_FMT_Y_PACK_UV444,   ///< 444 format y planar UV pack UVUVUVUVUV.....
	KDRV_IME_OUT_FMT_Y_PACK_UV422,   ///< 422 format y planar UV pack UVUVUVUVUV.....
	KDRV_IME_OUT_FMT_Y_PACK_UV420,   ///< 420 format y planar UV pack UVUVUVUVUV.....
	/* extend */
	KDRV_IME_OUT_FMT_Y,              ///< Only Y information
	KDRV_IME_OUT_FMT_RGB,            ///< RGB for output
	ENUM_DUMMY4WORD(KDRV_IME_OUT_FMT)
} KDRV_IME_OUT_FMT;


#if 0
/**
    KDRV IME structure - trigger type
*/
typedef struct _KDRV_IME_TRIG_TYPE {
	BOOL    wait_end;       ///< wait ime frame end
	UINT32  time_out_ms;    ///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_IME_TRIG_TYPE;
#endif

/**
    KDRV IME structure - YCC channel information
*/
typedef struct _KDRV_IME_YCC_CH_INFO_ {
	UINT32 width;       ///< image width(pixel)
	UINT32 height;      ///< image height(pixel)
	UINT32 line_ofs;    ///< image line offset(byte)
} KDRV_IME_YCC_CH_INFO;

/**
    KDRV IME structure - ime open object
*/
typedef struct _KDRV_IME_OPENCFG_ {
	UINT32 ime_clock_sel;               ///< Engine clock selection
} KDRV_IME_OPENCFG, *pKDRV_IME_OPENCFG;

/**
    KDRV IME structure - YCC Address info

    @note   if YCC format is UV-Packing, AddrCr will be ignore
*/
typedef struct _KDRV_IME_DMA_ADDR_INFO_ {
	UINT32 addr_y;    ///< DMA address of Y channel
	UINT32 addr_cb;   ///< DMA address of Cb channel
	UINT32 addr_cr;   ///< DMA address of Cr channel
} KDRV_IME_DMA_ADDR_INFO;

/**
    KDRV IME structure - Input Image Information

    @note   if output format is UV-Packing, ch[KDRV_IPP_YUV_V] infor will be ignore
*/
typedef struct _KDRV_IME_IN_INFO {
	KDRV_IME_IN_SRC         in_src; //input image source
	KDRV_IME_IN_FMT         type;   ///< image type
	KDRV_IME_YCC_CH_INFO    ch[KDRV_IPP_YUV_MAX_CH];  ///< image channel information
	KDRV_IME_BUF_FLUSH_SEL  dma_flush;   ///< dma buffer flush
} KDRV_IME_IN_INFO;

/**
    KDRV IME structure - Output path YCC address info

    @note   output Y buffer size  = Y_output_lineoffset * Y_output_height;
            output Cb buffer size  = Cb_output_lineoffset * Cb_output_height;
            output Cr buffer size  = Cr_output_lineoffset * Cr_output_height;

    @note   if output format is UV-Packing,
            set output Cb buffer size = Cb+Cr buffer size and Cr buffer size = 0

    @note   if enable ime path1 encode
            LineoffsetY = ceil(Y_output_width, 128) << 2;

            KDRV_IME_SINFO_FMT = KDRV_IME_SINFO_FMT_HEVC:
                Height = ceil(Y_output_height, 64) >> 2;
            KDRV_IME_SINFO_FMT = KDRV_IME_SINFO_FMT_H264:
                Height = ceil(Y_output_height, 16) >> 2;

            BufferY = LineoffsetY * Height

            LineoffsetC = ceil(Cb_output_width, 128);

            KDRV_IME_SINFO_FMT = KDRV_IME_SINFO_FMT_HEVC:
                Height = ceil(Cb_output_height>>1,32);
            KDRV_IME_SINFO_FMT = KDRV_IME_SINFO_FMT_H264:
                Height = ceil(Cb_output_height>>1, 8);

            BufferC = LineoffsetC * Height * 0.75(fixed compress ratio)
*/
typedef struct _KDRV_IME_OUT_PATH_ADDR_INFO_ {
	KDRV_IME_PATH_ID        path_id;
	KDRV_IME_DMA_ADDR_INFO  addr_info;
} KDRV_IME_OUT_PATH_ADDR_INFO;


/**
    KDRV IME structure - Output Image Information
*/
typedef struct _KDRV_IME_OUT_PATH_IMG_INFO_ {
	KDRV_IME_PATH_ID        path_id;
	BOOL                    path_en;
	BOOL                    path_flip_en;
	KDRV_IME_OUT_FMT        type;
	KDRV_IME_YCC_CH_INFO    ch[KDRV_IPP_YUV_MAX_CH];  ///< image channel information
	URECT crp_window;

	KDRV_IME_BUF_FLUSH_SEL  dma_flush;          ///< dam buffer flush
} KDRV_IME_OUT_PATH_IMG_INFO;

/**
    KDRV IME structure - Output Image Separate Information

    @note   separate ime output image to two part when enable this function
    @note   sprt function only support Y/UV-Pack format
    @note   sprt_pos need <= ime_crop_width
    @note   sprt_pos must be 4x
    @note   main_out_width = sprt_pos
    @note   sprt_width = ime_crop_width - sprt_pos
    @note   main_out and sprt_out has independent address and lineoffset

     -----------     -----
    |           |   |     |
    |           |   |sprt |
    |  main out |   | out |
    |           |   |     |
     -----------     -----
*/
typedef struct _KDRV_IME_OUT_SPRT_ADDR_INFO_ {
	KDRV_IME_PATH_ID        path_id;
	KDRV_IME_DMA_ADDR_INFO   sprt_addr; ///< path output DMA address for separating output
} KDRV_IME_OUT_SPRT_ADDR_INFO;

typedef struct _KDRV_IME_OUT_SPRT_INFO_ {
	KDRV_IME_PATH_ID        path_id;
	BOOL                    enable;
	UINT32                  sprt_pos;               ///< path output separating start position
	UINT32                  sprt_out_line_ofs_y;    ///< path output Y lineoffset for separating output
	UINT32                  sprt_out_line_ofs_cb;   ///< path output U lineoffset for separating output
} KDRV_IME_OUT_SPRT_INFO;

/**
    KDRV IME structure - extend infor for stripe information

    @note   only valid for ime input source is KDRV_IME_IN_SRC_DIRECT
*/
typedef struct _KDRV_IME_EXTEND_INFO_ {
	UINT32  stripe_num;     ///<stripe window number
	UINT32  stripe_h_max;   ///<max horizontal stripe size
	UINT32  tmnr_en;       ///<3dnr enable
	UINT32  p1_enc_en;      ///<output path1 encoder enable
}  KDRV_IME_EXTEND_INFO;

/**
    KDRV IME enum - side-information of codec for encoder
*/
typedef enum _KDRV_IME_SINFO_FMT_ {
	KDRV_IME_SINFO_FMT_HEVC = 0,  ///< for hevc
	KDRV_IME_SINFO_FMT_H264 = 1,  ///< for h264
	ENUM_DUMMY4WORD(KDRV_IME_SINFO_FMT)
} KDRV_IME_SINFO_FMT;

/**
    KDRV IME structure - encode parameters

    @note   encode function only valid for ime path1
    @note   side info buffer size
            lof = IPL_ALIGN_ROUNDUP(Y_output_width, 128) >> 7;
            if (fmt == KDRV_IME_SINFO_FMT_HEVC) {
                height = ceil(Y_output_height, 64) >> 6;    //hevc, 64 bit align
                size = ceil((lof << 2), 32) * height;
            } else {
                height = ceil(Y_output_height, 16) >> 4;    //h264, 16 bit align
                size = ceil(lof, 32) * height;
            }
*/
typedef struct _KDRV_IME_ENCODE_INFO_ {
	BOOL                    enable;             ///< encoder enable
} KDRV_IME_ENCODE_INFO;


/**
    KDRV IME structure - range clamp
*/
typedef struct _KDRV_IME_CLAMP_INFO {
	UINT32 min_y;
	UINT32 max_y;
	UINT32 min_uv;
	UINT32 max_uv;
} KDRV_IME_CLAMP_INFO;


/**
    KDRV IME Scale method select
*/
typedef enum _KDRV_IME_SCALER {
	KDRV_IME_SCALER_BICUBIC = 0,
	KDRV_IME_SCALER_BILINEAR,
	KDRV_IME_SCALER_NEAREST,
	KDRV_IME_SCALER_INTEGRATION,
	KDRV_IME_SCALER_AUTO,
	KDRV_IME_SCALER_MAX
} KDRV_IME_SCALER;

typedef struct _KDRV_IME_SCL_METHOD_SEL_INFO_ {
	UINT32              scl_th;         /* scale done ratio threshold, [31..16]output, [15..0]input */
	KDRV_IME_SCALER     method_l;       /* scale method when scale down ratio <= scl_th(output/input) */
	KDRV_IME_SCALER     method_h;       /* scale method when scale down ratio >  scl_th(output/input) */
} KDRV_IME_SCL_METHOD_SEL_INFO;

#if 0
#endif
//=====================================================================================
// IQ function structure


/**
    KDRV IME enum - KDRV_IME_IN_INFO inpus source
*/
typedef enum _KDRV_IME_LCA_IN_SRC {
	KDRV_IME_LCA_IN_SRC_DIRECT = 0,     //input direct from IFE engine
	KDRV_IME_LCA_IN_SRC_DRAM,           //input from dram
	ENUM_DUMMY4WORD(KDRV_IME_LCA_IN_SRC)
} KDRV_IME_LCA_IN_SRC;


/**
    KDRV IME structure - image parameters of local chroma adaptation
*/
typedef struct _KDRV_IME_LCA_IMG_INFO_ {
	UINT32                  dma_addr;       ///< input reference imaeg buffer address
	KDRV_IME_YCC_CH_INFO    img_size;       ///< input reference image info
	KDRV_IME_LCA_IN_SRC     in_src;         ///< input reference image source, from IFE2 when select direct mode
} KDRV_IME_LCA_IMG_INFO;

/**
    KDRV IME enum - value range selection

    @note   used for chroma adaptation
*/
typedef enum _KDRV_IME_RANGE_SEL {
	KDRV_IME_RANGE_8   = 0,  ///< range 8
	KDRV_IME_RANGE_16  = 1,  ///< range 16
	KDRV_IME_RANGE_32  = 2,  ///< range 32
	KDRV_IME_RANGE_64  = 3,  ///< range 64
	KDRV_IME_RANGE_128 = 4,  ///< range 128
	ENUM_DUMMY4WORD(KDRV_IME_RANGE_SEL)
} KDRV_IME_RANGE_SEL;

/**
    KDRV IME structure - local chroma adaptation parameters
*/
typedef struct _KDRV_IME_LCA_IQC_PARAM_ {
	UINT32                  ref_y_wt;   ///< Chroma reference weighting for Y channels
	UINT32                  ref_uv_wt;  ///< Chroma reference weighting for UV channels
	UINT32                  out_uv_wt;  ///< Chroma adaptation output weighting

	KDRV_IME_RANGE_SEL      y_rng;      ///< Chroma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL      y_wt_prc;   ///< Chroma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT32                  y_th;       ///< Chroma adaptation range threshold for UV channels
	UINT32                  y_wt_s;     ///< Chroma adaptation weighting start value for UV channels
	UINT32                  y_wt_e;     ///< Chroma adaptation weighting end value for UV channels


	KDRV_IME_RANGE_SEL      uv_rng;     ///< Chroma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL      uv_wt_prc;  ///< Chroma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT32                  uv_th;      ///< Chroma adaptation range threshold for UV channels
	UINT32                  uv_wt_s;    ///< Chroma adaptation weighting start value for UV channels
	UINT32                  uv_wt_e;    ///< Chroma adaptation weighting end value for UV channels
} KDRV_IME_LCA_IQC_PARAM;

/**
    KDRV IME structure - luma suppression parameters of local chroma adaptation
*/
typedef struct _KDRV_IME_LCA_IQL_PARAM_ {
	BOOL                    enable;     ///< Function enable
	UINT32                  ref_wt;     ///< Luma adaptation reference weighting for UV channels
	UINT32                  out_wt;     ///< Luma adaptation output weighting
	KDRV_IME_RANGE_SEL      rng;        ///< Luma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL      wt_prc;     ///< Luma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT32                  th;         ///< Luma adaptation range threshold for UV channels
	UINT32                  wt_s;       ///< Luma adaptation weighting start value for UV channels
	UINT32                  wt_e;       ///< Luma adaptation weighting end value for UV channels
} KDRV_IME_LCA_IQL_PARAM;

/**
    KDRV IME structure - chroma adjustment parameters of local chroma adaptation
*/
typedef struct _KDRV_IME_LCA_CA_PARAM_ {
	BOOL                enable;      ///< Function enable
	KDRV_IME_RANGE_SEL  uv_rng;      ///< Chroma adjustment range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL  uv_wt_prc;   ///< Chroma adjustment weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT32              uv_th;       ///< Chroma adjustment range threshold for UV channels
	UINT32              uv_wt_s;     ///< Chroma adjustment weighting start value for UV channels
	UINT32              uv_wt_e;     ///< Chroma adjustment weighting end value for UV channels
} KDRV_IME_LCA_CA_PARAM;

typedef struct _KDRV_IME_LCA_CA_CENT_INFO_ {
	UINT32  cent_u;      ///< chroma adjustment reference center of U channel
	UINT32  cent_v;      ///< chroma adjustment reference center of V channel
} KDRV_IME_LCA_CA_CENT_INFO;

/**
    KDRV IME enum - Chroma adaptation sub-image output source selection
*/
typedef enum _KDRV_IME_LCA_SUBOUT_SRC {
	KDRV_IME_LCA_SUBOUT_SRC_A = 0, ///< source: after LCA
	KDRV_IME_LCA_SUBOUT_SRC_B = 1, ///< source: before LCA
} KDRV_IME_LCA_SUBOUT_SRC;


/**
    KDRV IME structure - local chroma adaptation parameters
*/
typedef struct _KDRV_IME_LCA_PARAM_ {
	//BOOL                        enable;    ///< Function enable
	KDRV_IME_LCA_IQC_PARAM      chroma;    ///< Function quality information for chroma channel
	KDRV_IME_LCA_IQL_PARAM      luma;      ///< Function quality information for luma channel
	KDRV_IME_LCA_CA_PARAM       ca;        ///< Chroma adjustment information
#if (KDRV_IME_PQ_NEW == 1)
	KDRV_IME_LCA_SUBOUT_SRC     sub_out_src;    ///< LCA subout source selection
#endif
} KDRV_IME_LCA_PARAM;


typedef struct _KDRV_IME_LCA_CTRL_PARAM_ {
	BOOL    enable;
	BOOL    bypass;
} KDRV_IME_LCA_CTRL_PARAM;




/**
    KDRV IME structure - local chroma adaptation subout info.

    @note   lca sub out format YUV444 pack (fixed)
    @note   The out width must be the multiple of 4 and height must be the multiple of 2
    @note   ime_input_width/sub_out_size.width must > 2.07
    @note   ime_input_height need >= sub_out_size.height
    @note   sub_out_size.line_ofs >= sub_out_size.width * 3 (YUV444 pack)
    @note   lca sub out total buffer size = sub_out_size.line_ofs * sub_out_size.height;
*/
typedef struct _KDRV_IME_LCA_SUBOUT_INFO_ {
	BOOL                    enable;         ///< LCA subout enable
#if (KDRV_IME_PQ_NEW == 0)
	KDRV_IME_LCA_SUBOUT_SRC sub_out_src;    ///< LCA subout source selection
#endif
	UINT32                  sub_out_addr;   ///< LCA subout buffer address
	KDRV_IME_YCC_CH_INFO    sub_out_size;   ///< LCA subout size info
} KDRV_IME_LCA_SUBOUT_INFO;

/**
    KDRV IME enum - dark and bright chroma suppression mode selection
*/
typedef enum _KDRV_IME_DBCS_MODE_SEL {
	KDRV_IME_DBCS_DK_MODE   = 0,   ///< dark mode
	KDRV_IME_DBCS_BT_MODE   = 1,   ///< bright mode
	KDRV_IME_DBCS_BOTH_MODE  = 2,  ///< dark and bright mode
	ENUM_DUMMY4WORD(KDRV_IME_DBCS_MODE_SEL)
} KDRV_IME_DBCS_MODE_SEL;

/**
    KDRV IME structure - dark and bright region chroma suppression
*/
typedef struct _KDRV_IME_DBCS_PARAM_ {
	BOOL                    enable;     ///< dark and bright region chroma suppression function enable
	KDRV_IME_DBCS_MODE_SEL  op_mode;    ///< Process mode
	UINT32                  cent_u;     ///< Center value for U channel
	UINT32                  cent_v;     ///< Center value for V channel
	UINT32                  step_y;     ///< Step for luma
	UINT32                  step_c;     ///< Step for chroma
	UINT32                  wt_y[KDRV_IME_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
	UINT32                  wt_c[KDRV_IME_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
} KDRV_IME_DBCS_PARAM;

/**
    KDRV IME structure - single image super-resolution parameters
*/
typedef struct _KDRV_IME_SSR_PARAM_ {
	BOOL    enable;         ///< super resolution function enable
	BOOL    auto_mode_en;   ///< auto mode of SSR, SSR function on/off controlled by IME driver
	///< SSR function controlled by USER if set DISABLE
	UINT32  diag_th;        ///< Threahold for diagonal direction
	UINT32  h_v_th;         ///< Threahold for horizontal and vertical direction
} KDRV_IME_SSR_PARAM;

#if 0
/**
    KDRV IME structure - color space transformation parameters
*/
typedef struct _KDRV_IME_CST_PARAM {
	BOOL   rgb2ycc_en;  ///< Function enable, RGB to YCbCr
	BOOL   ycc2rgb_en;  ///< Function enable, YcbCr to RGB
} KDRV_IME_CST_PARAM;
#endif

/**
    KDRV IME structure - RGB to YCbCr parameters
*/
typedef struct _KDRV_IME_RGB2YCC_INFO_ {
	BOOL   enable;  ///< Function enable, RGB to YCbCr
} KDRV_IME_RGB2YCC_INFO;

/**
    KDRV IME structure - YCbCr to RGB parameters
*/
typedef struct _KDRV_IME_YCC2RGB_INFO_ {
	BOOL   enable;  ///< Function enable, YcbCr to RGB
} KDRV_IME_YCC2RGB_INFO;



/**
    KDRV IME structure - film grain noise parameters
*/
typedef struct _KDRV_IME_FGN_PARAM_ {
	BOOL      enable;   ///< film grain enable
	UINT32    lum_th;   ///< luminance value threshold
	UINT32    nl_p1;    ///< path1 noise level
	UINT32    init_p1;  ///< Grain noise initial value for path1
	UINT32    nl_p2;    ///< path2 noise level
	UINT32    init_p2;  ///< Grain noise initial value for path2
	UINT32    nl_p3;    ///< path3 noise level
	UINT32    init_p3;  ///< Grain noise initial value for path3
	UINT32    nl_p5;    ///< path5 noise level
	UINT32    init_p5;  ///< Grain noise initial value for path5
} KDRV_IME_FGN_PARAM;

/**
    KDRV IME enum - OSD set index
*/
typedef enum _KDRV_IME_OSD_SET_IDX {
	KDRV_IME_OSD_SET_IDX_0   = 0,
	KDRV_IME_OSD_SET_IDX_1   = 1,
	KDRV_IME_OSD_SET_IDX_2   = 2,
	KDRV_IME_OSD_SET_IDX_3   = 3,
	KDRV_IME_OSD_SET_IDX_MAX = 4,
	ENUM_DUMMY4WORD(KDRV_IME_OSD_SET_IDX)
} KDRV_IME_OSD_SET_IDX;

/**
    KDRV IME enum - OSD color LUT mode selection
*/
typedef enum _KDRV_IME_OSD_FMT_SEL {
	KDRV_IME_OSD_FMT_RGB565   = 0,  ///< OSD format, RGB565
	KDRV_IME_OSD_FMT_RGB1555  = 1,  ///< OSD format, RGB1555
	KDRV_IME_OSD_FMT_RGB4444  = 2,  ///< OSD format, RGB4444
	KDRV_IME_OSD_FMT_RGB8888  = 3,  ///< OSD format, RGB8888
	ENUM_DUMMY4WORD(KDRV_IME_OSD_FMT_SEL)
} KDRV_IME_OSD_FMT_SEL;

/**
    KDRV IME enum - OSD palette mode selection
*/
typedef enum _KDRV_IME_OSD_PLT_MODE_SEL {
	KDRV_IME_OSD_PLT_1BIT_MODE = 0, ///< 1bit mode
	KDRV_IME_OSD_PLT_2BIT_MODE = 1, ///< 2bit mode
	KDRV_IME_OSD_PLT_4BIT_MODE = 2, ///< 4bit mode
	ENUM_DUMMY4WORD(KDRV_IME_OSD_PLT_MODE_SEL)
} KDRV_IME_OSD_PLT_MODE_SEL;

/**
    KDRV IME enum - OSD color key mode selection
*/
typedef enum _KDRV_IME_OSD_COLOR_KEY_MODE_SEL {
	KDRV_IME_OSD_CKEY_RGB_MODE = 0, ///< using RGB mode
	KDRV_IME_OSD_CKEY_ARGB_MODE = 1, ///< using ARGB mode
	ENUM_DUMMY4WORD(KDRV_IME_OSD_COLOR_KEY_MODE_SEL)
} KDRV_IME_OSD_COLOR_KEY_MODE_SEL;



/**
    KDRV IME structure - image parameters for OSD
*/
typedef struct _KDRV_IME_OSD_IMAGE_INFO_ {
	USIZE                   img_size;   ///< Image size, maximum 8192x8192
	KDRV_IME_OSD_FMT_SEL    img_fmt;    ///< Image format selection
	UPOINT                  pos;        ///< Blending position
	UINT32                  ime_lofs;   ///< Data lineoffset
	UINT32                  img_addr;   ///< Data address
} KDRV_IME_OSD_IMAGE_INFO;

/**
    KDRV IME structure - image parameters for OSD

    @note   Blending weighting Usage:
            if RGB565, weight = bld_wt_0 + (bld_wt_1 << 4).
            if ARGB1555, A = 0, by using bld_wt_0 to index internal weighting table; A = 1, by using bld_wt_1 to index internal weighting table
            if ARGB1555, by using value of A to index internal weighting table
            Internal weighting table: [0,  17,  34,  51,  68,  85,  102,  119,  136,  153,  170,  187,  204,  221,  238,  255]
*/
typedef struct _KDRV_IME_OSD_IQ_INFO_ {
	BOOL                            color_key_en;   ///< color key enable
	KDRV_IME_OSD_COLOR_KEY_MODE_SEL color_key_mode;
	UINT8                           color_key_val[KDRV_IME_DS_COLOR_KEY_TAB];   ///< Color key, ,valid when color_key_en = TRUE
	///< color_key_val[0]: for A channel,
	///< color_key_val[1]: for R channel,
	///< color_key_val[2]: for G channel,
	///< color_key_val[3]: for B channel,
	BOOL        plt_en;         ///< palette mode enable
	UINT32      bld_wt_0;       ///< Blending weighting, if RGB565, range: [0, 16]; others, range: [0, 15]
	UINT32      bld_wt_1;       ///< Blending weighting, range: [0, 15]
} KDRV_IME_OSD_IQ_INFO;

/**
    KDRV IME structure - OSD parameters of each set

    @note   only supports 2 sets, set0 and set3 when input src is KDRV_IME_IN_SRC_DRAM
*/
typedef struct _KDRV_IME_OSD_INFO_ {
	KDRV_IME_OSD_SET_IDX        ds_set_idx; ///< OSD set
	BOOL                        enable;         ///< Function enable
	KDRV_IME_OSD_IMAGE_INFO     image;          ///< Image information
	KDRV_IME_OSD_IQ_INFO        ds_iq;      ///< OSD quality information
} KDRV_IME_OSD_INFO;


/**
    KDRV IME structure - YUV color LUT parameters of OSD

    @note   maximum image size 8192x8192
*/
typedef struct _KDRV_IME_OSD_CST_INFO {
	BOOL    enable;             ///< color space trans enable
	BOOL    cst_auto_param_en;  ///< parameter auto mode enable
	UINT32  cst_coef0;          ///< Coefficient of color space transform
	UINT32  cst_coef1;          ///< Coefficient of color space transform
	UINT32  cst_coef2;          ///< Coefficient of color space transform
	UINT32  cst_coef3;          ///< Coefficient of color space transform
} KDRV_IME_OSD_CST_INFO;

/**
    KDRV IME structure - Palette LUT parameters of OSD
*/
typedef struct _KDRV_IME_OSD_PLT_INFO {
	KDRV_IME_OSD_PLT_MODE_SEL   plt_mode;       ///< palette mode selection

	UINT8       plt_a[KDRV_IME_DS_PLT_TAB];     ///< color palette of A channel, range: [0, 255]
	UINT8       plt_r[KDRV_IME_DS_PLT_TAB];     ///< color palette of R channel, range: [0, 255]
	UINT8       plt_g[KDRV_IME_DS_PLT_TAB];     ///< color palette of G channel, range: [0, 255]
	UINT8       plt_b[KDRV_IME_DS_PLT_TAB];     ///< color palette of B channel, range: [0, 255]
} KDRV_IME_OSD_PLT_INFO;


/**
    KDRV IME enum - histogram operating selection
*/
typedef enum _KDRV_IME_STL_SET_SEL {
	KDRV_IME_STL_SET0  = 0,  ///< set0 selection
	KDRV_IME_STL_SET1  = 1,  ///< set1 selection
	ENUM_DUMMY4WORD(KDRV_IME_STL_SET_SEL)
} KDRV_IME_STL_SET_SEL;

/**
    KDRV IME enum - stl roi set index
*/
typedef enum _KDRV_IME_STL_ROI_SET_IDX {
	KDRV_IME_STL_ROI_SET_IDX_0   = 0,
	KDRV_IME_STL_ROI_SET_IDX_1   = 1,
	KDRV_IME_STL_ROI_SET_IDX_2   = 2,
	KDRV_IME_STL_ROI_SET_IDX_3   = 3,
	KDRV_IME_STL_ROI_SET_IDX_4   = 4,
	KDRV_IME_STL_ROI_SET_IDX_5   = 5,
	KDRV_IME_STL_ROI_SET_IDX_6   = 6,
	KDRV_IME_STL_ROI_SET_IDX_7   = 7,
	KDRV_IME_STL_ROI_SET_IDX_MAX = 8,
	ENUM_DUMMY4WORD(KDRV_IME_STL_ROI_SET_IDX)
} KDRV_IME_STL_ROI_SET_IDX;

/**
    KDRV IME enum - statistical edge kernel selection
*/
typedef enum _KDRV_IME_STL_EKER_SEL {
	KDRV_IME_STL_EKER_HA  = 0,  ///< H map A
	KDRV_IME_STL_EKER_HB  = 1,  ///< H map B
	KDRV_IME_STL_EKER_VA  = 2,  ///< V map A
	KDRV_IME_STL_EKER_VB  = 3,  ///< V map B
	ENUM_DUMMY4WORD(KDRV_IME_STL_EKER_SEL)
} KDRV_IME_STL_EKER_SEL;

/**
    KDRV IME structure - statistical output image

    @note   edge map buffer size: output path4 crop_h*crop_v
            histogram buffer size: 128bin x 20bit x 2sets
*/
typedef struct _KDRV_IME_STL_OUT_IMG_INFO_ {
	UINT32  edge_map_addr;  ///< edge map output address
	UINT32  edge_map_lofs;  ///< edge map output lineoffset
	UINT32  hist_addr;      ///< output address for edge statistic
} KDRV_IME_STL_OUT_IMG_INFO;

/**
    KDRV IME structure - statistical edte parameters
*/
typedef struct _KDRV_IME_STL_EDGE_INFO_ {
	BOOL                    flip_en;

	BOOL                    ker0_enable;    ///< edge kernel enable for set0
	KDRV_IME_STL_EKER_SEL   ker0_sel;       ///< edge map kernel selection for map0
	UINT32                  shift0;         ///< edge map shift for map0

	BOOL                    ker1_enable;    ///< edge kernel enable for set1
	KDRV_IME_STL_EKER_SEL   ker1_sel;       ///< edge map kernel selection for map1
	UINT32                  shift1;         ///< edge map shift for map1
} KDRV_IME_STL_EDGE_INFO;

/**
    KDRV IME structure - statistical histogram parameters
*/
typedef struct _KDRV_IME_STL_HIST_INFO_ {
	KDRV_IME_STL_SET_SEL    set_sel0;   ///< operation set selection
	URECT                   crop_win0;  ///<crop window for edge statistic
	UINT32                  acc_targ0;  ///< histogram accumulation target
	KDRV_IME_STL_SET_SEL    set_sel1;   ///< operation set selection
	URECT                   crop_win1;  ///<crop window for edge statistic
	UINT32                  acc_targ1;  ///< histogram accumulation target
} KDRV_IME_STL_HIST_INFO;

/**
    KDRV IME structure - ROI threshold parameters of statistical
*/
typedef struct _KDRV_IME_STL_ROI_TH_INFO_ {
	KDRV_IME_STL_SET_SEL    src;    ///< ///< statistical source of ROI
	UINT32                  th0;    ///< statistical threshold of ROI for section0
	UINT32                  th1;    ///< statistical threshold of ROI for section1
	UINT32                  th2;    ///< statistical threshold of ROI for section2
} KDRV_IME_STL_ROI_TH_INFO;

/**
    KDRV IME structure - statistical parameters
*/
typedef struct _KDRV_IME_STL_ROI_INFO_ {
	UINT32 row0;  ///< row position0
	UINT32 row1;  ///< row posotion1
	KDRV_IME_STL_ROI_TH_INFO  roi[KDRV_IME_STL_ROI_SET_IDX_MAX];    ///< ROI for edge map, total 8 sets
} KDRV_IME_STL_ROI_INFO;


/**
    KDRV IME structure - statistical parameters

    @note   maximun image size is 1920x1080
    @note   output ime path4 only
*/
typedef struct _KDRV_IME_STL_INFO_ {
	BOOL                    enable;             ///< enable of statistic
	BOOL                    median_ftr_enable;  ///< filter enable of statistic
	BOOL                    img_out_aft_filt;   ///< output before or after filter image
	KDRV_IME_STL_EDGE_INFO  edge;
} KDRV_IME_STL_INFO;

/**
    KDRV IME enum - privacy mask set index
*/
typedef enum _KDRV_IME_PM_SET_IDX {
	KDRV_IME_PM_SET_IDX_0   = 0,
	KDRV_IME_PM_SET_IDX_1   = 1,
	KDRV_IME_PM_SET_IDX_2   = 2,
	KDRV_IME_PM_SET_IDX_3   = 3,
	KDRV_IME_PM_SET_IDX_4   = 4,
	KDRV_IME_PM_SET_IDX_5   = 5,
	KDRV_IME_PM_SET_IDX_6   = 6,
	KDRV_IME_PM_SET_IDX_7   = 7,
	KDRV_IME_PM_SET_IDX_MAX = 8,
	ENUM_DUMMY4WORD(KDRV_IME_PM_SET_IDX)
} KDRV_IME_PM_SET_IDX;

/**
    KDRV IME enum - privacy mask type
*/
typedef enum _KDRV_IME_PM_MASK_TYPE {
	KDRV_IME_PM_MASK_TYPE_YUV = 0,  ///< Using YUV color
	KDRV_IME_PM_MASK_TYPE_PXL = 1,  ///< Using pixilation
	ENUM_DUMMY4WORD(KDRV_IME_PM_MASK_TYPE)
} KDRV_IME_PM_MASK_TYPE;

/**
    KDRV IME structure - privacy mask parameter of each set

    @note   If overlap is occurred between (among) masks, for example Set0 and Set1, the overlap region between them, Set0 is always on top.
    @note   Only support convex quadrilateral mask

*/
typedef struct _KDRV_IME_PM_INFO_ {
	KDRV_IME_PM_SET_IDX         set_idx;                        ///< total 8 set, idx = 0~7
	BOOL                        enable;                         ///< privacy mask set enable
	BOOL                        hlw_enable;                     ///< privacy mask hollow function enable
	IPOINT                      coord[KDRV_IME_PM_COORD_TAB];   ///< privacy mask 4 coordinates(horizontal and vertical direction), must be Convex Hull
	IPOINT                      coord2[KDRV_IME_PM_COORD_TAB];   ///< privacy mask 4 coordinates(horizontal and vertical direction) for hollow, must be Convex Hull
	KDRV_IME_PM_MASK_TYPE       msk_type;                       ///< privacy mask type selection
	UINT32                      color[KDRV_IPP_YUV_MAX_CH];     ///< privacy mask using YUV color, range: [0, 255]
	UINT32                      weight;                         ///< privacy mask alpha weight, range: [0, 255]
} KDRV_IME_PM_INFO;


/**
    KDRV IME enum - privacy mask pixelation blocks size selection
*/
typedef enum _KDRV_IME_PM_PXL_BLK_SIZE {
	KDRV_IME_PM_PIXELATION_08 = 0,  ///< 8x8
	KDRV_IME_PM_PIXELATION_16 = 1,  ///< 16x16
	KDRV_IME_PM_PIXELATION_32 = 2,  ///< 32x32
	KDRV_IME_PM_PIXELATION_64 = 3,  ///< 64x64
	ENUM_DUMMY4WORD(KDRV_IME_PM_PXL_BLK_SIZE)
} KDRV_IME_PM_PXL_BLK_SIZE;

/**
    KDRV IME structure - privacy mask pixel image info

    @note   minimum image_size.w = (ime_in_w + (block_size - 1)) / block_size
    @note   minimum image_size.h = (ime_in_h + (block_size - 1)) / block_size
    @note   lofs need >= pm_pxl_image_size.w*3

    @example    if blk_size = KDRV_IME_PM_PIXELATION_64, IME Input Size is 3840x2160
    @example    img_size_w need >= (3840 + (64 - 1) ) / 64 = 60.984375, round to 60
    @example    img_size_h need >= (2160 + (64 - 1) ) / 64 = 37.734375, round to 34
*/
typedef struct _KDRV_IME_PM_PXL_IMG_INFO_ {
	USIZE                       img_size;   ///< pixelation input image size of privacy mask
	KDRV_IME_PM_PXL_BLK_SIZE    blk_size;   ///< block size of pixelation
	UINT32                      lofs;       ///< pixelation input image lineoffset
	UINT32                      dma_addr;   ///< pixelation input imaeg buffer address
} KDRV_IME_PM_PXL_IMG_INFO;


/**
    IME enum - Strength option for Y-channel pre-filter
*/
typedef enum _KDRV_IME_TMNR_PRE_FILTER_TYPE_ {
	KDRV_PRE_FILTER_OFF = 0,
	KDRV_PRE_FILTER_TYPE_A = 1,
	KDRV_PRE_FILTER_TYPE_B = 2,
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_PRE_FILTER_TYPE)
} KDRV_IME_TMNR_PRE_FILTER_TYPE;

/**
    IME enum - An option of update mode in motion estimation
*/
typedef enum _KDRV_IME_TMNR_ME_UPDATE_MODE_ {
	KDRV_ME_UPDATE_RAND  = 0,
	KDRV_ME_UPDATE_FIXED = 1,
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_ME_UPDATE_MODE)
} KDRV_IME_TMNR_ME_UPDATE_MODE;

/**
    IME enum - Method option of motion vector down-sampling
*/
typedef enum _KDRV_IME_TMNR_MV_DOWN_SAMPLE_MODE_ {
	KDRV_MV_DOWN_SAMPLE_NEAREST  = 0,
	KDRV_MV_DOWN_SAMPLE_AVERAGE  = 1,
	KDRV_MV_DOWN_SAMPLE_MINIMUM  = 2,
	KDRV_MV_DOWN_SAMPLE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_MV_DOWN_SAMPLE_MODE)
} KDRV_IME_TMNR_MV_DOWN_SAMPLE_MODE;

typedef enum _KDRV_IME_TMNR_MV_INFO_MODE_ {
	KDRV_MV_INFO_MODE_AVERAGE  = 0,
	KDRV_MV_INFO_MODE_LPF      = 1,
	KDRV_MV_INFO_MODE_MINIMUM  = 2,
	KDRV_MV_INFO_MODE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_MV_INFO_MODE)
} KDRV_IME_TMNR_MV_INFO_MODE;

/**
    IME enum - Method option of patch selection
*/
typedef enum _KDRV_IME_TMNR_PS_MODE_ {
	KDRV_PS_MODE_0 = 0,   ///< determined by object type
	KDRV_PS_MODE_1 = 1,   ///< determined by a LUT mode
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_PS_MODE)
} KDRV_IME_TMNR_PS_MODE;

/**
    IME enum - Option for SAD statistic data
*/
typedef enum _KDRV_IME_TMNR_ME_SDA_TYPE_ {
	KDRV_ME_SDA_TYPE_NATIVE = 0,         ///< native SAD
	KDRV_ME_SDA_TYPE_COMPENSATED = 1,   ///< compensated SAD
	ENUM_DUMMY4WORD(KDRV_IME_TMNR_ME_SDA_TYPE)
} KDRV_IME_TMNR_ME_SDA_TYPE;


typedef struct _KDRV_IME_TMNR_STATISTIC_PARAM_ {
	UINT32 sample_step_hori;  ///< step size of statistical sampling in horizontal direction
	UINT32 sample_step_vert;  ///< step size of statistical sampling in vertical direction
	UINT32 sample_num_x;   ///< total number of statistical sampling in horizontal direction
	UINT32 sample_num_y;   ///< total number of statistical sampling in vertical direction
	UINT32 sample_st_x;    ///< start point of horizontal direction for statistical sampling process
	UINT32 sample_st_y;    ///< start point of vertical direction for statistical sampling process
} KDRV_IME_TMNR_STATISTIC_PARAM;

/**
    IME structure - 3DNR motion estimation parameters
*/
typedef struct _KDRV_IME_TMNR_ME_PARAM_ {
	KDRV_IME_TMNR_ME_UPDATE_MODE update_mode;  ///< An option of update mode in motion estimation. Range 0~1
	BOOL             boundary_set;  ///< Set image boundary as still type or not.  Range 0~1
	KDRV_IME_TMNR_MV_DOWN_SAMPLE_MODE ds_mode;  ///< Method option of motion vector down-sampling.Range 0~3
	KDRV_IME_TMNR_ME_SDA_TYPE    sad_type;   ///< Option for SAD statistic data.  Range 0~1
	UINT32                  sad_shift;  ///< Right shift number for SAD statistic data, range = [0, 15]

	UINT32                  rand_bit_x; ///<range 1~7
	UINT32                  rand_bit_y; ///<range 1~7
	UINT32                  min_detail; ///< range 0~16383
	UINT32                  periodic_th;///<range 0~16383

	UINT32                  sad_penalty[KDRV_IME_TMNR_ME_SAD_PENALTY_TAB]; ///< range 0~1023
	UINT32                  switch_th[KDRV_IME_TMNR_ME_SWITCH_THRESHOLD_TAB]; ///< range 0~255
	UINT32                  switch_rto;   ///< range 0~255
	UINT32                  detail_penalty[KDRV_IME_TMNR_ME_DETAIL_PENALTY_TAB]; ///< range 0~15
	UINT32                  probability[KDRV_IME_TMNR_ME_PROBABILITY_TAB];    ///< range 0~1

} KDRV_IME_TMNR_ME_PARAM;


/**
    IME structure - 3DNR motion detection parameters
*/
typedef struct _KDRV_IME_TMNR_MD_PARAM_ {
	UINT32 sad_coefa[KDRV_IME_TMNR_MD_SAD_COEFA_TAB];  ///< Edge coefficient of motion detection. Range 0~63
	UINT32 sad_coefb[KDRV_IME_TMNR_MD_SAD_COEFB_TAB];  ///< Offset of motion detection. Range 0~16383
	UINT32 sad_std[KDRV_IME_TMNR_MD_SAD_STD_TAB];    ///< Standard deviation of motion detection. Range 0~16383
	UINT32 fth[KDRV_IME_TMNR_MD_FINAL_THRESHOLD_TAB];   ///< final threshold of motion detection. Range 0~63
} KDRV_IME_TMNR_MD_PARAM;

/**
    IME structure - 3DNR motion detection parameters for ROI
*/
typedef struct _KDRV_IME_TMNR_MD_ROI_PARAM_ {
	UINT32 fth[KDRV_IME_TMNR_MD_ROI_FINAL_THRESHOLD_TAB];   ///< final threshold of motion detection.Range 0~63
} KDRV_IME_TMNR_MD_ROI_PARAM;


/**
    IME structure - 3DNR motion compensation parameters
*/
typedef struct _KDRV_IME_TMNR_MC_PARAM_ {
	UINT32 sad_base[KDRV_IME_TMNR_MC_SAD_BASE_TAB];  ///< base level of noise. Range 0~16383
	UINT32 sad_coefa[KDRV_IME_TMNR_MC_SAD_COEFA_TAB];  ///< edge coefficient of motion compensation.Range 0~63
	UINT32 sad_coefb[KDRV_IME_TMNR_MC_SAD_COEFB_TAB];  ///< offset of motion compensation.Range 0~16383
	UINT32 sad_std[KDRV_IME_TMNR_MC_SAD_STD_TAB];    ///< standard deviation of motion compensation.Range 0~16383
	UINT32 fth[KDRV_IME_TMNR_MC_FINAL_THRESHOLD_TAB];   ///< final threshold of motion compensation. Range 0~63
} KDRV_IME_TMNR_MC_PARAM;

/**
    IME structure - 3DNR motion compensation parameters for ROI
*/
typedef struct _KDRV_IME_TMNR_MC_ROI_PARAM_ {
	UINT32 fth[KDRV_IME_TMNR_MC_ROI_FINAL_THRESHOLD_TAB];   ///< final threshold of motion compensation.Range 0~63
} KDRV_IME_TMNR_MC_ROI_PARAM;


/**
    IME structure - 3DNR patch selection parameters
*/
typedef struct _KDRV_IME_TMNR_PS_PARAM_ {
	BOOL smart_roi_ctrl_en;    ///< ROI control. Range 0~1
	BOOL mv_check_en;  ///< MV checkiong process in PS module. Range 0~1
	BOOL roi_mv_check_en; ///< MV checking process for ROI. Range 0~1
	KDRV_IME_TMNR_PS_MODE ps_mode;  ///< Method option of patch selection. Range 0~1
	KDRV_IME_TMNR_MV_INFO_MODE mv_info_mode; ///< Mode option for MV length calculation. Range 0~3
	UINT32 mv_th; ///< MV threshold. Range 0~63
	UINT32 roi_mv_th; ///< MV threshold for ROI. Range 0~63
	UINT32 mix_ratio[KDRV_IME_TMNR_PS_MIX_RATIO_TAB];   ///< Mix ratio in patch selection. Range 0~255
	UINT32 mix_th[KDRV_IME_TMNR_PS_MIX_THRESHOLD_TAB];  ///< Mix threshold in patch selection. Range 0~16383.
	//UINT32 mix_slope[KDRV_IME_TMNR_PS_MIX_SLOPE_TAB]; ///< Mix slope in patch selection
	UINT32 ds_th;   ///< Threshold of motion status down-sampling. Range 0~31
	UINT32 ds_th_roi; ///< Threshold for motion status down-sampling for ROI.Range 0~31
	UINT32 edge_wet;   ///< Start point of edge adjustment. Range 0~255
	UINT32 edge_th[KDRV_IME_TMNR_PS_EDGE_THRESHOLD_TAB];    ///< Edge adjustment threshold in patch selection. Range 0~16383
	//UINT32 edge_slope;  ///< Edge adjustment slope in patch selection
	UINT32 fs_th;  ////< Threshold of patch error. Range 0~16383.
} KDRV_IME_TMNR_PS_PARAM;


/**
    IME structure - 3DNR noise filter parameters
*/
typedef struct _KDRV_IME_TMNR_NR_PARAM_ {
	BOOL               luma_ch_en;    ///< noise filter enable for luma channel. Range 0~1
	BOOL               chroma_ch_en;  ///< noise filter enable for chroma channel.Range 0~1

	BOOL center_wzeros_y; ///< Set weighting as 0 to the center pixel in internal NR-filter. Range 0~1
	BOOL chroma_fsv_en; ///< False color control.Range 0~1
	UINT32      chroma_fsv; ///< False color value. Range 0~255

	UINT32 luma_residue_th; ///< Protection threshold of luma channel. Range 0~255
	UINT32 chroma_residue_th; ///< Protection threshold of luma channel. Range 0~255

	UINT32 freq_wet[KDRV_IME_TMNR_NR_FREQ_WEIGHT_TAB]; ///< Filter weighting for low frequency. Range 0~255
	UINT32 luma_wet[KDRV_IME_TMNR_NR_LUMA_WEIGHT_TAB]; ///< Filter intensity weighting. Range 0~255

	KDRV_IME_TMNR_PRE_FILTER_TYPE pre_filter_type;  ///< Strength option for luma channel pre-filter.Range 0~2
	UINT32 pre_filter_str[KDRV_IME_TMNR_NR_PRE_FILTER_STRENGTH_TAB]; ///< Strength of pre-filtering for low frequency. Range 0~255
	UINT32 pre_filter_rto[KDRV_IME_TMNR_NR_PRE_FILTER_RATION_TAB]; ///< adjustment ratio of pre-filtering for transitional object.Range 0~255

	UINT32 snr_str[KDRV_IME_TMNR_NR_SFILTER_STRENGTH_TAB]; ///< Strength of spatial filter for still object.Range 0~255
	UINT32 tnr_str[KDRV_IME_TMNR_NR_TFILTER_STRENGTH_TAB]; ///< Strength of temporal filter for still object.Range 0~255

	UINT32 snr_base_th; ///< Base threshold of spatial noise reduction.Range 0~65535
	UINT32 tnr_base_th; ///< Base threshold of spatial noise reduction.Range 0~65535

	UINT32 luma_3d_th[KDRV_IME_TMNR_NR_LUMA_THRESHOLD_TAB]; ///< Noise reduction threshold for luma channel. Range 0~63
	UINT32 luma_3d_lut[KDRV_IME_TMNR_NR_LUMA_LUT_TAB]; ///< Noise reduction LUT for luma channel.Range 0~127
	UINT32 chroma_3d_lut[KDRV_IME_TMNR_NR_CHROMA_LUT_TAB]; ///< Noise reduction LUT for chroma channel.Range 0~127
	UINT32 chroma_3d_rto[KDRV_IME_TMNR_NR_CHROMA_RATIO_TAB]; ///< Adjustment ratio 0 of noise reduction LUT for chroma channel.Range 0~255

	BOOL   luma_nr_type;	///< 3DNR filter option, Range 0~1
} KDRV_IME_TMNR_NR_PARAM;

/**
    IME structure - 3DNR debug parameters
*/
typedef struct _KDRV_IME_TMNR_DBG_PARAM_ {
	BOOL dbg_mv0;       ///< debug for mv = 0
	UINT32 dbg_mode;    ///< debug mode selection. Range 0~6
} KDRV_IME_TMNR_DBG_PARAM;




/**
    KDRV IME structure - decoder parameters for TMNR

    @note   TMNR reference image decode information
*/
typedef struct _KDRV_KDRV_IME_TMNR_DEC_INFO_ {
	BOOL    enable;             ///< TMNR input reference decoder enable
} KDRV_IME_TMNR_DEC_INFO;



/**
    KDRV IME structure - TMNR reference input image information

    @note   ref image size need = IME Input image size
    @note   reference image format must be Y_UV_PACK420
*/
typedef struct _KDRV_IME_TMNR_REF_IN_IMG_INFO_ {
	BOOL                        flip_en;            ///< TMNR input image flip enable
	UINT32                      img_addr_y;         ///< TMNR buffer address of preivous image Y channel
	UINT32                      img_addr_cb;        ///< TMNR buffer address of preivous image Cb channel
	UINT32                      img_line_ofs_y;     ///< TMNR buffer lineoffset of preivous image Y channel
	UINT32                      img_line_ofs_cb;    ///< TMNR buffer lineoffset of preivous image Cb channel
	KDRV_IME_TMNR_DEC_INFO      decode;             ///< TMNR reference input decode
} KDRV_IME_TMNR_REF_IN_IMG_INFO;

/**
    KDRV IME structure - TMNR motion satus info.
*/
typedef struct _KDRV_IME_TMNR_MOTION_STATUS_INFO_ {
	UINT32  mot_sta_ofs; ///< TMNR motion status input and output lineoffset
	UINT32  mot_sta_in_addr; ///< TMNR motion status input DMA buffer address
	UINT32  mot_sta_out_addr; ///< TMNR motion status output DMA buffer address
} KDRV_IME_TMNR_MOTION_STATUS_INFO;

/**
    KDRV IME structure - TMNR motion satus ROI info.
*/
typedef struct _KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO_ {
	BOOL         mot_sta_roi_out_en;   ///< TMNR motion status output enable for ROI
	BOOL                mot_sta_roi_out_flip_en; ///< TMNR motion status output flip enable for ROI
	UINT32              mot_sta_roi_out_ofs; ///< TMNR motion status output lineoffset for ROI
	UINT32              mot_sta_roi_out_addr; ///< TMNR motion status output DMA buffer address for ROI
} KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO;


/**
    KDRV IME structure - TMNR motion vector info.
*/
typedef struct _KDRV_IME_TMNR_MOTION_VECTOR_INFO_ {
	UINT32              mot_vec_ofs; ///< 3DNR motion vector input and output lineoffset
	UINT32              mot_vec_in_addr; ///< 3DNR motion vector input DMA buffer address
	UINT32              mot_vec_out_addr; ///< 3DNR motion vector output DMA buffer address
} KDRV_IME_TMNR_MOTION_VECTOR_INFO;


/**
    KDRV IME structure - TMNR statistic data info.
*/
typedef struct _KDRV_IME_TMNR_STATISTIC_DATA_INFO_ {
	BOOL                            sta_out_en;   ///< control of statistic data output
	KDRV_IME_TMNR_STATISTIC_PARAM   sta_param;  ///< 3DNR statistic data output parameters
	UINT32                          sta_out_ofs; ///< 3DNR statistic data output lineoffset
	UINT32                          sta_out_addr; ///< 3DNR statistic data output DMA buffer address
} KDRV_IME_TMNR_STATISTIC_DATA_INFO;


/**
    KDRV IME structure - TMNR info.
*/
typedef struct _KDRV_IME_TMNR_PARAM_ {
	BOOL                            enable;             ///< 3DNR enable
	KDRV_IME_TMNR_ME_PARAM          me_param;
	KDRV_IME_TMNR_MD_PARAM          md_param;
	KDRV_IME_TMNR_MD_ROI_PARAM      md_roi_param;
	KDRV_IME_TMNR_MC_PARAM          mc_param;
	KDRV_IME_TMNR_MC_ROI_PARAM      mc_roi_param;
	KDRV_IME_TMNR_PS_PARAM          ps_param;
	KDRV_IME_TMNR_NR_PARAM          nr_param;
} KDRV_IME_TMNR_PARAM;


/**
    KDRV IME structure - TMNR reference output image information

    @note   ref image size need = IME Input image size
    @note   reference image format must be Y_UV_PACK420
*/
typedef struct _KDRV_IME_TMNR_REF_OUT_IMG_INFO_ {
	BOOL                        enable;             ///< TMNR output image enable
	BOOL                        flip_en;            ///< TMNR output image filpe enable
	UINT32                      img_addr_y;         ///< TMNR output buffer address of current image Y channel
	UINT32                      img_addr_cb;        ///< TMNR output buffer address of current image Cb channel
	UINT32                      img_line_ofs_y;     ///< TMNR output buffer lineoffset of current image Y channel
	UINT32                      img_line_ofs_cb;    ///< TMNR output buffer lineoffset of current image Cb channel
	KDRV_IME_ENCODE_INFO        encode;             ///< TMNR output reference encode
	KDRV_IME_BUF_FLUSH_SEL      buf_flush;      ///< TMNR output reference buffer flush control
} KDRV_IME_TMNR_REF_OUT_IMG_INFO;


/**
    KDRV IME structure - TMNR motion and statistic buffer info.
*/
typedef struct _KDRV_IME_TMNR_BUF_SIZE_INFO_ {
	UINT32 in_size_h;               ///< input image width
	UINT32 in_size_v;               ///< input image height

	UINT32 in_sta_max_num;          ///< statistic data maxima number

	UINT32 get_mv_lofs;             ///< get motion vector buffer lineoffset
	UINT32 get_mv_size;             ///< get motion vector buffer size

	UINT32 get_ms_lofs;             ///< get motion status buffer lineoffset
	UINT32 get_ms_size;             ///< get motion status buffer size

	UINT32 get_ms_roi_lofs;         ///< get motion status buffer lineoffset for ROI
	UINT32 get_ms_roi_size;         ///< get motion status buffer size for ROI

	KDRV_IME_TMNR_STATISTIC_PARAM get_sta_param;    ///< get statistic parameters
	UINT32 get_sta_lofs;                            ///< get statistic data buffer lineoffset
	UINT32 get_sta_size;                            ///< get statistic data buffer size
} KDRV_IME_TMNR_BUF_SIZE_INFO;


/**
    KDRV IME CMF parameters
*/
typedef struct _KDRV_IME_CMF_PARAM_ {
	BOOL    enable; ///< chroma median filter parameter
} KDRV_IME_CMF_PARAM;

/**
    KDRV IME P2I parameters
*/
typedef struct _KDRV_IME_P2I_INFO_ {
	BOOL    enable; ///< chroma median filter parameter
} KDRV_IME_P2I_INFO;


/**
    KDRV IME enum - YCbCr converter selection
*/
typedef enum _KDRV_IME_YCC_CVT_ {
	KDRV_IME_YCC_CVT_BT601 = 0,  ///< BT.601
	KDRV_IME_YCC_CVT_BT709 = 1,  ///< BT.709
	ENUM_DUMMY4WORD(KDRV_IME_YCC_CVT)
} KDRV_IME_YCC_CVT;


/**
    KDRV IME structure - YCbCr converter parameters
*/
typedef struct _KDRV_IME_YCC_CVT_PARAM {
	BOOL enable;   ///< YCbCrconverter function enable/disable
	KDRV_IME_YCC_CVT cvt_sel;  ///< YCbCr converter selection, BT.601 or BT.709
} KDRV_IME_YCC_CVT_PARAM;



/* struct  _KDRV_IME_TRIGGER_PARAM: structure to describe a job
 *
 * @note For kdrv_ime_set_trig()
 */
typedef struct _KDRV_IME_TRIGGER_PARAM {

} KDRV_IME_TRIGGER_PARAM;

/**
    KDRV IME all IQ parameters
*/
typedef struct _KDRV_IME_PARAM_IQ_ALL_PARAM_ {
	KDRV_IME_CMF_PARAM      *p_cmf;
	KDRV_IME_LCA_PARAM      *p_lca;
	KDRV_IME_DBCS_PARAM     *p_dbcs;
	KDRV_IME_SSR_PARAM      *p_ssr;
	KDRV_IME_FGN_PARAM      *p_fgn;
	KDRV_IME_TMNR_PARAM     *p_tmnr;
	KDRV_IME_YCC_CVT_PARAM  *p_ycccvt;
	KDRV_IME_TMNR_DBG_PARAM *p_tmnr_dbg;
} KDRV_IME_PARAM_IQ_ALL_PARAM;


/**
    KDRV IME enum - function selection
*/
typedef enum _KDRV_IME_IQ_FUNC_ {
	KDRV_IME_IQ_FUNC_CMF      = 0x00000001,
	KDRV_IME_IQ_FUNC_LCA      = 0x00000002,
	KDRV_IME_IQ_FUNC_DBCS     = 0x00000004,
	KDRV_IME_IQ_FUNC_SSR      = 0x00000008,
	KDRV_IME_IQ_FUNC_FGN      = 0x00000010,
	KDRV_IME_IQ_FUNC_TMNR     = 0x00000020,
	KDRV_IME_IQ_FUNC_YUVCVT   = 0x00000040,
	KDRV_IME_IQ_FUNC_ALL      = 0x0000007F,
	ENUM_DUMMY4WORD(KDRV_IME_IQ_FUNC)
} KDRV_IME_IQ_FUNC;


/**
    KDRV IME all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_IME_PARAM_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;  ///< IME IQ function enable selection
	BOOL   enable;         ///< IME IQ function enable/disable operation
} KDRV_IME_PARAM_IPL_FUNC_EN;

#if 0
#endif


/**
    KDRV IME PARAM ITEM, for kdrv_ime_set/kdrv_ime_get
*/

typedef enum {
	KDRV_IME_PARAM_IPL_OPENCFG = 0,         //ime open engine info,                             data_type: [Set]     KDRV_IME_OPENCFG
	KDRV_IME_PARAM_IPL_IN_IMG,              //ime input image info,                             data_type: [Set/Get] KDRV_IME_IN_INFO
	KDRV_IME_PARAM_IPL_IN_ADDR,             //ime input Y/U/V address,                          data_type: [Set/Get] KDRV_IME_DMA_ADDR_INFO
	KDRV_IME_PARAM_IPL_OUT_IMG,             //ime output path image info,                       data_type: [Set/Get] KDRV_IME_OUT_PATH_IMG_INFO
	KDRV_IME_PARAM_IPL_OUT_ADDR,            //ime output path Y/U/V address,                    data_type: [Set/Get] KDRV_IME_OUT_PATH_ADDR
	KDRV_IME_PARAM_IPL_OUT_SPRT,            //ime output path image info,                       data_type: [Set/Get] KDRV_IME_OUT_SPRT_INFO
	KDRV_IME_PARAM_IPL_OUT_SPRT_ADDR,       //ime output path Y/U/V address,                    data_type: [Set/Get] KDRV_IME_OUT_SPRT_ADDR
	KDRV_IME_PARAM_IPL_EXTEND,              //ime extend stripe overlap info                    data_type: [Set/Get] KDRV_IME_EXTEND_INFO
	KDRV_IME_PARAM_IPL_ENCODE,              //ime output encode info, Only for path1            data_type: [Set/Get] KDRV_IME_ENCODE_INFO
	KDRV_IME_PARAM_IPL_ISR_CB,              //ime external isr cb
	KDRV_IME_PARAM_IQ_CMF,                  //ime chroma median filter,                         data_type: [Set/Get] KDRV_IME_CMF_PARAM, default DISABLE
	KDRV_IME_PARAM_IQ_LCA,                  //ime local chroma adaption,                        data_type: [Set/Get] KDRV_IME_LCA_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_LCA_CA_CENT,         //ime local chroma adaption ca center U/V           data_type: [Set/Get] KDRV_IME_LCA_CA_CENT_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_LCA_REF_IMG,         //ime local chroma adaption ref in img info,        data_type: [Set/Get] KDRV_IME_LCA_IMG_INFO
	KDRV_IME_PARAM_IPL_LCA_SUB,             //ime local chroma adaption sub out,                data_type: [Set/Get] KDRV_IME_LCA_SUBOUT_INFO, default DISABLE
	KDRV_IME_PARAM_IQ_DBCS,                 //ime dark and bright region chroma supression,     data_type: [Set/Get] KDRV_IME_DBCS_PARAM, default DISABLE
	KDRV_IME_PARAM_IQ_SSR,                  //ime super resolution,                             data_type: [Set/Get] KDRV_IME_SSR_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_RGB2YCC,             //ime color space transformation, RGB to YCbCr,     data_type: [Set/Get] KDRV_IME_RGB2YCC_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_YCC2RGB,             //ime color space transformation, YCbCr to RGB      data_type: [Set/Get] KDRV_IME_YCC2RGB_INFO, default DISABLE
	KDRV_IME_PARAM_IQ_FGN,                  //ime film grain                                    data_type: [Set/Get] KDRV_IME_FGN_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_OSD,                 //ime OSD,                                          data_type: [Set/Get] KDRV_IME_OSD_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_OSD_CST,             //ime OSD cst,                                      data_type: [Set/Get] KDRV_IME_OSD_CST_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_OSD_PLT,             //ime OSD palette table                             data_type: [Set/Get] KDRV_IME_OSD_PLT_INFO
	KDRV_IME_PARAM_IPL_P2I,                 //ime progressive to interlaced parameter,          data_type: [Set/Get] KDRV_IME_P2I_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_STL,                 //ime statistical parameters,                       data_type: [Set/Get] KDRV_IME_STL_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_STL_ROI,             //ime statistical roi parameters,                   data_type: [Set/Get] KDRV_IME_STL_ROI_INFO
	KDRV_IME_PARAM_IPL_STL_HIST,            //ime statistical histogram parameters,             data_type: [Set/Get] KDRV_IME_STL_HIST_INFO
	KDRV_IME_PARAM_IPL_STL_OUT_IMG,         //ime statistical out image info,                   data_type: [Set/Get] KDRV_IME_STL_OUT_IMG_INFO
	KDRV_IME_PARAM_IPL_PM,                  //ime primacy mask,                                 data_type: [Set/Get] KDRV_IME_PM_INFO, default DISABLE
	KDRV_IME_PARAM_IPL_PM_PXL_IMG,          //ime primacy mask pixel image info,                data_type: [Set/Get] KDRV_IME_PM_PXL_IMG_INFO
	KDRV_IME_PARAM_IQ_TMNR,                 //ime tmnr,                                         data_type: [Set/Get] KDRV_IME_TMNR_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG,     //ime tmnr ref. input image,                        data_type: [Set/Get] KDRV_IME_TMNR_REF_IN_IMG_INFO
	KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG,    //ime tmnr ref. output image,                       data_type: [Set/Get] KDRV_IME_TMNR_REF_OUT_IMG_INFO
	KDRV_IME_PARAM_IPL_TMNR_MS,             //ime tmnr motion status in and out info.,          data_type: [Set/Get] KDRV_IME_TMNR_MOTION_STATUS_INFO
	KDRV_IME_PARAM_IPL_TMNR_MS_ROI,         //ime tmnr motion status out info. for ROI,         data_type: [Set/Get] KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO
	KDRV_IME_PARAM_IPL_TMNR_MV,             //ime tmnr motion vector in and out info.,          data_type: [Set/Get] KDRV_IME_TMNR_MOTION_VECTOR_INFO
	KDRV_IME_PARAM_IPL_TMNR_STA,            //ime tmnr statistic data out info.,                data_type: [Set/Get] KDRV_IME_TMNR_STATISTIC_DATA_INFO
	KDRV_IME_PARAM_IPL_SCL_METHOD_SEL,      //ime scale method select,                          data_type: [Set/Get] KDRV_IME_SCL_METHOD_SEL_INFO
	KDRV_IME_PARAM_IPL_ENCODE_INFO,         //get ktab info. from HW-device(GET-Only)           data_type: [Get    ] KDRV_IME_ENCODE_INFO
	KDRV_IME_PARAM_IQ_ALL,                  //ime all IQ parameters,                            data_type: [Set    ] KDRV_IME_PARAM_IQ_ALL_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_ALL_FUNC_EN,         //ime all IQ function enable                        data_type: [Set    ] KDRV_IME_PARAM_IPL_FUNC_EN, default DISABLE
	KDRV_IME_PARAM_IPL_LCA_FUNC_EN,         //ime lca function enable control                   data_type: [Set    ] KDRV_IME_LCA_CTRL_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_BREAK_POINT,         //ime break-point control                           data_type: [Set/Get] KDRV_IME_BREAK_POINT_INFO
	KDRV_IME_PARAM_IPL_SINGLE_OUTPUT,       //ime single output control                         data_type: [Set/Get] KDRV_IME_SINGLE_OUT_INFO
	KDRV_IME_PARAM_IPL_LOW_DELAY,           //ime low delay control                             data_type: [Set/Get] KDRV_IME_LOW_DELAY_INFO
	KDRV_IME_PARAM_IPL_INTER,               //ime interruput enable,                            data_type: [Set    ] KDRV_IME_INTE
	KDRV_IME_PARAM_IPL_LOAD,                //ime directly load parameters                      data_type: [Set    ] NULL
	KDRV_IME_PARAM_IQ_YCCCVT,               //ime YCbCr converter parameters                    data_type: [Set/Get] KDRV_IME_YCC_CVT_PARAM, default DISABLE
	KDRV_IME_PARAM_IPL_TMNR_BUF_SIZE,       //ime tmnr info. buffer size (MV, MS, MS-ROI, STA)  data_type: [Get    ] KDRV_IME_TMNR_BUF_SIZE_INFO
	KDRV_IME_PARAM_IQ_TMNR_DBG,             //ime tmnr debug info.                              data_type: [Set    ] KDRV_IME_TMNR_DBG_PARAM
	KDRV_IME_PARAM_IPL_SINGLE_OUTPUT_HW,    //ime single output control from HW                 data_type: [Get    ] KDRV_IME_SINGLE_OUT_INFO
	KDRV_IME_PARAM_IPL_RESET,               // ime channel reset                                data_type: [Set    ] NULL
	KDRV_IME_PARAM_IPL_BUILTIN_DISABLE,    //ime disable builtin flow control     data_type: [Set    ] NULL
	KDRV_IME_PARAM_IPL_TMNR_REFIN_USE_VA,   // ime tmnr reference input using virtual addr.     data_type: [Set    ] NULL
	KDRV_IME_PARAM_MAX,
	KDRV_IME_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IME_PARAM_ID)
} KDRV_IME_PARAM_ID;
STATIC_ASSERT((KDRV_IME_PARAM_MAX &KDRV_IME_PARAM_REV) == 0);

#define KDRV_IME_IGN_CHK KDRV_IME_PARAM_REV //only support set/get function

#endif //_KDRV_IME_H_
