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

    @file       Dis_alg.h
    @ingroup    mILibDIS
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/** \addtogroup DISLib */
//@{

#ifndef _DIS_ALG_H_
#define _DIS_ALG_H_



#include "dis_alg_fw.h"

/**
    DIS Process Configuration

    Structure of DIS process configuration
*/
//@{
typedef enum {
	DISALG_CFG_GLOBALMOTEN,      ///< configure enable of global motion calculation
	DISALG_CFG_ROIMOTEN,         ///< configure enable of ROI motion calculation
	DISALG_CFG_2MDSLAYEREN,      ///< configure enable of two MDS layers
	ENUM_DUMMY4WORD(DISALG_CFGID)
} DISALG_CFGID;
//@}


/**
    DIS Process Configuraiton Paramteres

    Structure of DIS process configuraiton parameters
*/
//@{
typedef struct _DISALG_CFGINFO_STRUCT {
	INT32 icfgval;              ///< configuration value
} DISALG_CFGINFO;
//@}

//#NT#2018/07/23#joshua liu -begin
//#NT#Support time stamp
#define	DIS_TSTAMP_BUF_SIZE		5
#define	DIS_MV_BUF_SIZE			5

typedef struct _DIS_TIME_STAMP {
	UINT32 frame_count;
	UINT64 time_stamp;
} DIS_TIME_STAMP;

typedef struct _DIS_MOV_VEC {
	BOOL             mv_ready;
	UINT32           frame_count;
    UINT64           time_stamp;
	DIS_MOTION_INFOR mv[DIS_MVNUMMAX];
} DIS_MOV_VEC;

typedef struct _DIS_ROI_MOV_VEC {
	BOOL             mv_ready;
	UINT32           frame_count;
	DIS_ROIGMV_OUT   mv;
} DIS_ROI_MOV_VEC;


//#NT#2018/07/23#joshua liu -end


/**
    Configuration parameters

    Structure of configuration parameters of DIS process
*/
//@{
typedef struct _DISPROC_CFGPARA_STRUCT {
	BOOL bglobal_motioncal_en;
	BOOL broi_motioncal_en;
	BOOL btwo_mdslayers_en;
} DISALG_CFGPARA;
//@}


extern void      dis_roi_setInputInfo(DIS_ROIGMV_IN *roiIn);
extern void      dis_roi_getInputInfo(DIS_ROIGMV_IN *roiIn);
extern INT32     dis_get_ready_roi_motionvec(DIS_ROI_MV_INFO *p_gmvinfo);
extern INT32     dis_get_match_roi_motionvec(DIS_ROI_MV_INFO *p_gmvinfo);
extern BOOL      dis_framecnt_set_in(UINT32 frame_count);




#endif

