/***************************************************************************
* Copyright  Novatek Microelectronics Corp. 2015.  All rights reserved.    *
*--------------------------------------------------------------------------*
* Name: DIS Library Module                                                  *
* Description:                                                             *
* Author: Connie Yu                                                         *
****************************************************************************/

/**
    DIS lib

    Sample module detailed description.

    @file       Dis_alg_fw.h
    @ingroup    mILibDIS
    @note       DIS FW process.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/** \addtogroup DISLib */
//@{

#ifndef _DIS_ALG_FW_H_
#define _DIS_ALG_FW_H_

#include "dis_alg_hw.h"


//#define MDIS_PRV_MAX_SIZE    2304* 1296 * 2 * 2 ///< MAXSIZE*1.44*FMT_422*2
#define DISLIB_VECTOR_NORM      12      ///< output vector norm factor(2^12)
#define DIS_ABS(x)              ((x)>=0?(x):(-(x)))

/**
    DIS Motion Vector.

    Structure of directional motion vector elements.
*/
//@{
typedef struct _DISALG_MOTION_VECTOR {
	INT32 ix; ///< x component
	INT32 iy; ///< y component
} DISALG_MOTION_VECTOR;
//@}

/**
    DIS ALG result Vector information

    Structure of DIS ALG result vector information.
*/
//@{
typedef struct _DISALG_VECTOR_STRUCT {
	DISALG_MOTION_VECTOR vector;///< result vector
	UINT16 frame_cnt;           ///< frame count
	UINT32 score;               ///< 1: useable, 0: not sure
} DISALG_VECTOR;
//@}

/**
    DIS Score Level of Screening Motion Vectors.

    Select the level of DIS motion vectors.
*/
//@{
typedef enum {
	DIS_MVSCORE_HIGH    = 0, ///< high score level
	DIS_MVSCORE_NORMAL  = 1, ///< normal score level
	DIS_MVSCORE_LOW     = 2, ///< low score level
	DIS_MVSCORE_LOW2    = 3, ///< low score level 2
	ENUM_DUMMY4WORD(DIS_MVSCORE_LEVEL)
} DIS_MVSCORE_LEVEL;
//@}
//----------------------------------------------------------------------

/**
    DIS Stickiness Level of Compensation.

    Select the level of how sticky the compensation is.
*/
//@{
typedef enum {
	DIS_STICKY_TOP      = 0,  ///< top level of stickiness
	DIS_STICKY_HIGH     = 1,  ///< high level of stickiness
	DIS_STICKY_NORMAL   = 2,  ///< normal level of stickiness
	DIS_STICKY_LOW      = 3,  ///< low level of stickiness
	DIS_STICKY_LOW2     = 4,  ///< low level 2 of stickiness
	ENUM_DUMMY4WORD(DIS_STICKY_LEVEL)
} DIS_STICKY_LEVEL;
//@}

typedef enum _DIS_ACCESS_TYPE_ {
	DIS_ACCESS_SET = 0,
	DIS_ACCESS_GET = 1,
	DIS_ACCESS_MAX = 0xFF,
	ENUM_DUMMY4WORD(DIS_ACCESS_TYPE)
} DIS_ACCESS_TYPE;

//----------------------------------------------------------------------
/**
    DIS Compensation Information

    Structure of compensation parameters.
*/
//@{
typedef struct _COMPENSATION_INFO {
	UINT32 ui_bound_h;    ///< horizontal boundary
	UINT32 ui_bound_v;    ///< vertical boundary
	UINT8  ui_dzoom_rate; ///< dzoom rate
} COMPENSATION_INFO;
//@}
//----------------------------------------------------------------------
/**
    DIS Global Motion Information

    Structure of DIS module parameters when calculating global motion.
*/
//@{
typedef struct DIS_GM_STRUCT {
	DIS_MOTION_VECTOR            *p_imv;    ///< integral motion vector
	COMPENSATION_INFO            *p_comp;   ///< compensation information
	DIS_MVSCORE_LEVEL            score_lv;  ///< Level of MV score screening
	DIS_STICKY_LEVEL             sticky_lv; ///< Stickiness level of compensation
	DIS_MOTION_INFOR             *p_mv;     ///< Motion verctors
} DIS_GM;
//@}

#if 0
/**
    Motion in tracking.

    Structure of information of a moving object.
*/
//@{
typedef struct _TRACK_DIS_MOTION_INFOR {
	INT32   iTrkX;         ///< x component
	INT32   iTrkY;         ///< y component
	UINT32  uiPosX;        ///< left
	UINT32  uiPosY;        ///< top
	BOOL    bMovObj;       ///< if it's a moving object
} TRACK_DIS_MOTION_INFOR;
//@}
#endif

/**
    DIS Global Motion Vector in ROI

    Structure of input coordinate and size.
*/
//@{
typedef struct _DIS_ROIGMV_IN_STRUCT {
	UINT8 ui_start_x;              ///< input x coordinate, 0~100%
	UINT8 ui_start_y;              ///< input y coordinate, 0~100%
	UINT8 ui_size_x;               ///< input width, 0~100%
	UINT8 ui_size_y;               ///< input height, 0~100%
} DIS_ROIGMV_IN;
//@}

/**
    DIS Global Motion Vector in ROI

    Structure of output global motion vector.
*/
//@{
typedef struct _DIS_ROIGMV_OUT_STRUCT {
	DISALG_MOTION_VECTOR vector;  ///< output result vector
	BOOL  bvalid_vec;             ///< if the result vector is valid
} DIS_ROIGMV_OUT;
//@}


typedef struct _MOTION_VECTOR_QUEUE {
	UINT32 ui_total_x_cnt;
	UINT32 ui_zero_x_cnt;
} MOTION_VECTOR_QUEUE;

typedef struct DIS_SGM_STRUCT {
	DIS_MOTION_INFOR *p_cur_mv; ///< current motion vector
	DIS_MOTION_INFOR *p_tmp_mv; ///< temporal motion vector
	DIS_MOTION_INFOR *p_blk_mv; ///< block motion vectors
	DIS_MOTION_INFOR *p_pre_mv; ///< previous motion vectors
	MOTION_VECTOR_QUEUE *p_last_motvect_que;
} DIS_SGM;


extern void disfw_process(DIS_MOTION_INFOR *p_mv);
extern void disfw_initialize(DIS_IPC_INIT *p_buf);
extern void disfw_reset(void);
extern void disfw_end(void);

extern void disfw_set_dis_infor(DIS_PARAM *p_disinfo);
extern UINT32 disfw_get_prv_maxbuffer(void);

extern void disfw_accum_update_process(DIS_MOTION_INFOR *p_mv);
extern UINT16 dis_get_dis_viewratio(void);
extern void dis_set_dis_viewratio(UINT16 ratio);
extern ER   dis_get_frame_corrvec(DISALG_VECTOR *p_corr, UINT32 frame_cnt);
extern ER   dis_get_frame_corrvec_pxl(DISALG_VECTOR *p_corr, UINT32 frame_cnt);
extern VOID dis_get_ois_det_motvec(DIS_SGM *p_iv);
extern UINT32 dis_access_ois_on_off(DIS_ACCESS_TYPE acc_type, UINT32 ui_isois_on);
extern UINT32 dis_set_ois_det_motvec(INT32 ix, INT32 iy);
extern DIS_ROIGMV_OUT disfw_get_roi_motvec(DIS_ROIGMV_IN *p_roi_in, DIS_MOTION_INFOR *p_mv);


#endif

