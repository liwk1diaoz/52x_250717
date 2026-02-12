/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UIControl_Int.h
    @ingroup    mIPRJAPCfg

    @brief

    @note       Nothing.

    @date       2005/12/13
*/

/** \addtogroup mIPRJAPCfg */
//@{

#ifndef _UICTRL_INT_H
#define _UICTRL_INT_H
#include <kwrap/type.h>


#define FLGFRAMEWORK_WINDOW_CREATED     0x00000001
#define FLGFRAMEWORK_FORCE_CLOSE        0x00000002
#define FLGFRAMEWORK_IDLE               0x00000020


extern ID UICTRL_WND_SEM_ID;
extern ID UICTRL_DRW_SEM_ID;
extern ID UICTRL_FLG_ID;

#endif //_UICTRL_INT_H
