#ifndef _KDRV_IFE2_INT_H_
#define _KDRV_IFE2_INT_H_

#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ife2.h"

#include "ife2_lib.h"

/**
    max ife2 dal handle number, current max is 8 due to flag config
*/
#define KDRV_IFE2_HANDLE_MAX_NUM (KDRV_IPP_CBUF_MAX_NUM)

// funtion type for dal_ife2
typedef ER(*KDRV_IFE2_SET_FP)(UINT32, void *);
typedef ER(*KDRV_IFE2_GET_FP)(UINT32, void *);


typedef enum _KDRV_IFE2_ID {
	KDRV_IFE2_ID_0 = 0,                                 ///< ISE id 1
	KDRV_IFE2_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_IFE2_ID)
} KDRV_IFE2_ID;


/**
    IFE2_BURSTLEN
*/
typedef enum _KDRV_IFE2_BURST_LEN {
	KDRV_IFE2_BURST_LEN_32   = 0,                ///< burst 32 word
	KDRV_IFE2_BURST_LEN_16   = 1,                ///< burst 16 word
	//KDRV_IFE2_BURST_LEN_96   = 2,                ///< burst 96 word
	//KDRV_IFE2_BURST_LEN_128  = 3,                ///< burst 128 word
} KDRV_IFE2_BURST_LEN;

typedef struct _KDRV_IFE2_BURST {
	KDRV_IFE2_BURST_LEN input;
} KDRV_IFE2_BURST;


/**
    IFE2 DAL structure
*/
typedef struct _KDRV_IFE2_PRAM {

	// input info
	UINT32                  in_pixel_addr;      ///< pixel RAW address
	KDRV_IFE2_IN_INFO       in_img_info;        ///< IFE2 input image info

	//output info
	UINT32                  out_pixel_addr;     ///< pixel RAW address
	KDRV_IFE2_OUT_IMG        out_img_info;       ///< IFE2 output image info

	// function setting
	KDRV_IFE2_INTE           inte_en;
	KDRV_IFE2_GRAY_STATIST   gray_statist;       ///< gray statistical (only for U/V channel)
	KDRV_IFE2_REFCENT_PARAM  refcent;         ///< reference center parameters for Y/UV channel
	KDRV_IFE2_FILTER_PARAM   filter;          ///< IFE2 filter para set
	KDRV_IFE2_BURST          burst;
} KDRV_IFE2_PRAM, *pKDRV_IFE2_PRAM;


/**
    IFE DAL handle structure
*/
#define KDRV_IFE2_HANDLE_LOCK   0x00000001

typedef struct _KDRV_IFE2_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_IFE2_HANDLE;
#endif //_KDRV_IFE2_INT_H_
