#ifndef _KDRV_IFE_INT_H_
#define _KDRV_IFE_INT_H_

#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "ife_lib.h"


/**
    max ife kdrv handle number, current max is 8 due to flag config
*/
#define KDRV_IFE_HANDLE_MAX_NUM (KDRV_IPP_CBUF_MAX_NUM)

/**
    IFE RAW Image bits
*/
typedef enum _KDRV_IFE_RAW_BIT {
	KDRV_IFE_RAW_BIT_8  = 0,    ///<  8 bits
	KDRV_IFE_RAW_BIT_10 = 1,    ///< 10 bits
	KDRV_IFE_RAW_BIT_12 = 2,    ///< 12 bits
	KDRV_IFE_RAW_BIT_16 = 3,    ///< 16 bits
	KDRV_IFE_RAW_BIT_MAX_CNT,
	ENUM_DUMMY4WORD(KDRV_IFE_RAW_BIT)
} KDRV_IFE_RAW_BIT;

/**
    RAW Image Information
*/
typedef struct _KDRV_IFE_RAW_INFO_INT {
	UINT32           width;        ///< image width      (pixel)
	UINT32           height;       ///< image height     (pixel)

	UINT32           crop_width;   ///< image crop width      (pixel)
	UINT32           crop_height;  ///< image crop height     (pixel)
	UINT32           crop_hpos;    ///< image crop horzontal start
	UINT32           crop_vpos;    ///< image crop virtical start
	/*
	    UINT32           stripe_hm;
	    UINT32           stripe_hn;
	    UINT32           stripe_hl;
	    UINT8            h_shift;
	    */
	KDRV_IFE_RAW_BIT bit;          ///< bits /pixel
	//UINT8            cfa_pat;
	KDRV_IPP_PIX     st_pix;       ///< image start pixel
} KDRV_IFE_RAW_INFO_INT;

/**
    IFE KDRV Input Image Information
*/
typedef struct _KDRV_IFE_IN_IMG_INFO_INT {
	KDRV_IFE_IN_MODE         in_src;        ///< input image source
	KDRV_IFE_IN_FMT          type;       ///< image type
	KDRV_IFE_RAW_INFO_INT    img_info;   ///< image information
} KDRV_IFE_IN_IMG_INFO_INT;

/**
    IFE KDRV Output Image Information
*/
typedef struct _KDRV_IFE_OUT_IMG_INFO_INT {
	UINT32           line_ofs;   ///< image out offset
	KDRV_IFE_RAW_BIT bit;        ///< bits /pixel
} KDRV_IFE_OUT_IMG_INFO_INT;

// funtion type for kdrv_ife
typedef ER(*KDRV_IFE_SET_FP)(UINT32, void *);
typedef ER(*KDRV_IFE_GET_FP)(UINT32, void *);

/**
    IFE KDRV structure
*/
typedef struct _KDRV_IFE_PRAM {
	// input info
	UINT32                       in_pixel_addr_1;    ///< pixel RAW address
	UINT32                       in_pixel_addr_2;    ///< pixel RAW address
	UINT32                       line_ofs_1;         ///< image line offset 1(byte)
	UINT32                       line_ofs_2;         ///< image line offset 2(byte)
	KDRV_IFE_IN_IMG_INFO_INT     in_img_info;        ///< IFE input image info

	// output info
	UINT32                       out_pixel_addr;     ///< pixel RAW address
	KDRV_IFE_OUT_IMG_INFO_INT    out_img_info;       ///< IFE output image info

	// function setting
	KDRV_IFE_INTE                inte_en;            ///< IFE interrupt enable

	KDRV_IFE_RDE_INFO            info_rde;            ///< IFE IQ parameter (rde)
	KDRV_IFE_FUSION_INFO         info_fusion;
	KDRV_IFE_RING_BUF_INFO       info_ring_buf;
	KDRV_IFE_WAIT_SIE2_DISABLE_INFO info_wait_sie2;  ///< IFE dma wait sie2 start and first line end or not, only 528 
	KDRV_IFE_NRS_PARAM           par_nrs;            ///< IFE IQ parameter (nrs)
	KDRV_IFE_FUSION_PARAM        par_fusion;         ///< IFE IQ parameter (fusion)
	KDRV_IFE_FCURVE_PARAM        par_fcurve;         ///< IFE IQ parameter (fcurve)
	KDRV_IFE_MIRROR              par_mirror;         ///< IFE IQ parameter (mirror)

	KDRV_IFE_OUTL_PARAM          par_outl;           ///< IFE IQ parameter (outlier)
	KDRV_IFE_FILTER_PARAM        par_filter;         ///< IFE IQ parameter (filter)
	KDRV_IFE_CGAIN_PARAM         par_cgain;          ///< IFE IQ parameter (color gain)
	KDRV_IFE_VIG_PARAM           par_vig;            ///< IFE IQ parameter (vignette)
	KDRV_IFE_VIG_CENTER          vig_info;
	KDRV_IFE_GBAL_PARAM          par_gbal;           ///< IFE IQ parameter (G balance)
} KDRV_IFE_PRAM, *pKDRV_IFE_PRAM;


/**
    IFE KDRV handle structure
*/
#define KDRV_IFE_HANDLE_LOCK    0x00000001

typedef struct _KDRV_IFE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_IFE_HANDLE;
#endif //_KDRV_IFE_INT_H_
