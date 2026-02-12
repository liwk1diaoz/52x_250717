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

    @file       Dis_alg_hw.h
    @ingroup    mILibDIS
    @note       DIS HW control.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/** \addtogroup DISLib */
//@{

#ifndef _DIS_ALG_HW_H_
#define _DIS_ALG_HW_H_

#include "nvt_dis.h"


//#define MDIS_PRV_MAX_SIZE    2304* 1296 * 2 * 2 ///< MAXSIZE*1.44*FMT_422*2
//#define DISLIB_VECTOR_NORM      12      ///< output vector norm factor(2^12)

typedef struct {
	UINT32  hsz;
	UINT32  vsz;
} DIS_SIZE;

/**
    DIS ALG Entire Configuration

    Structure of DIS ALG parameters when one wants to configure this module.
*/
//@{
typedef struct _DISHW_PARAM_STRUCT {
	//DIS_ENGINE_OPERATION DIS_operation; ///< DIS engine operation
	UINT32 insize_h;                      ///< input horizontal pixels
	UINT32 insize_v;                      ///< input vertical pixels
	UINT32 inlineofs;                     ///< input line offset
	UINT32 inadd0;                        ///< input starting address 0
	UINT32 inadd1;                        ///< input starting address 1
	UINT32 inadd2;                        ///< input starting address 2
	UINT32 frame_cnt;                     ///< frame count
} DISHW_PARAM;
//@}

/**
    DIS Process Event Selection.

    Select DIS process event. Event control is carried out by PhotoDisTsk
*/
//@{
typedef enum {
	DIS_PROC_RESTART = 0,   ///< process restart
	DIS_PROC_UPDATE  = 1,   ///< process update display information
	DIS_PROC_PAUSE   = 2,   ///< process pause
	DIS_PROC_NOOP    = 3,   ///< no operation
	ENUM_DUMMY4WORD(DIS_PROC_EVENT)
} DIS_PROC_EVENT;
//@}


extern UINT32    dishw_get_prv_maxbuffer(void);
extern void      dishw_initialize(DIS_IPC_INIT *p_buf);
extern ER        dishw_process(void);
extern void      dishw_end(void);
extern ER        dishw_set_dis_infor(DISHW_PARAM *p_disinfo);
extern ER        dishw_chg_dis_sizeconfig(void);


#endif

