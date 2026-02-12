#ifndef _KDRV_SDE_INT_H
#define _KDRV_SDE_INT_H
#if 0
#ifdef __KERNEL__

#include <mach/rcw_macro.h>
#include "kwrap/flag.h"
#include "kdrv_sde.h"
#include "sde_lib.h"

#else

#include "Type.h"

#endif
#endif
#include "sde_platform.h"
#include "kdrv_videoprocess/kdrv_sde.h"

#define KDRV_SDE_HANDLE_MAX_NUM (8)
#define KDRV_SDE_MAX_INTEEN_NUM 7

/**
    IPL process ID
*/
typedef enum _KDRV_SDE_ID {
	KDRV_SDE_ID_1 = 0,                                  ///< SDE id 1
	KDRV_SDE_ID_2 = 1,                                  ///< SDE id 2
	KDRV_SDE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_SDE_ID)
} KDRV_SDE_ID;

typedef INT32(*KDRV_SDE_ISRCB)(UINT32, UINT32, void *, void *);
typedef ER(*KDRV_SDE_SET_FP)(UINT32, void *);
typedef ER(*KDRV_SDE_GET_FP)(UINT32, void *);

//#ifdef __KERNEL__
extern ID FLG_ID_KDRV_SDE;
extern SEM_HANDLE SEMID_KDRV_SDE;
//#endif


extern void kdrv_sde_install_id(void);
extern void kdrv_sde_uninstall_id(void);
void kdrv_sde_flow_init(void);

/**
    SDE KDRV handle structure
*/
#define KDRV_SDE_HANDLE_LOCK    0x00000001

typedef struct _KDRV_SDE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_SDE_ISRCB isrcb_fp;
} KDRV_SDE_HANDLE;

/**
    SDE KDRV structure
*/
typedef struct {

	// input info
	KDRV_SDE_MODE_INFO mode_info;				///< sde mode info
	KDRV_SDE_IO_INFO io_info;					///< sde in/out info

	KDRV_SDE_COST_PARAM cost_param;             ///< sde cost parameter
	KDRV_SDE_LUMA_PARAM luma_param;             ///< sde luma related parameter
	KDRV_SDE_PENALTY_PARAM penalty_param;       ///< sde penalty parameter
	KDRV_SDE_SMOOTH_PARAM smooth_param;         ///< sde smooth term parameter
	KDRV_SDE_LRC_THRESH_PARAM lrc_th_param;     ///< sde left right check threshold parameter

} KDRV_SDE_PRAM, *pKDRV_SDE_PRAM;


#endif //_KDRV_SDE_INT_H


