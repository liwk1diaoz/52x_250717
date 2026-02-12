/**
    Header file of Movie Stamp API

    Exported API of Movie Stamp functions.

    @file       MovieStampAPI.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

#ifndef _MOVIESTAMPAPI_H
#define _MOVIESTAMPAPI_H

#include "kwrap/type.h"

#define STAMP_DATETIME           0x00000001
#define STAMP_WATERLOGO          0x00000002
#define STAMP_MILTI_1            0x00000004
#define STAMP_ALL               (STAMP_DATETIME | STAMP_WATERLOGO | STAMP_MILTI_1)

/**
    @addtogroup
*/
//@{

/**
     Install movie stamp task, flag and semaphore id.

     Install movie stamp task, flag and semaphore id.

     @return void
*/
extern void MovieStamp_InstallID(void) _SECTION(".kercfg_text");
extern void MovieStamp_UnInstallID(void) _SECTION(".kercfg_text");

/**
    @name   Movie Stamp Function API
*/
//@{

/**
    Movie Stamp Callback Function

    The prototype of callback function for movie stamp data update

    @return void
*/
typedef void (*MOVIESTAMP_UPDATE_CB)(void);

/**
    Movie Stamp Callback Function

    The prototype of callback function for movie stamp data update

    @return void
*/
typedef UINT32 (*MOVIESTAMP_TRIGGER_UPDATE_CHECK_CB)(void);

/*
    Open movie stamp task.

    Open movie stamp task.

    @param[in] void

    @return
     - @b E_OK:     open successfully.
     - @b E_SYS:    movie stamp task is already open.
*/
extern ER   MovieStampTsk_Open(void);

/**
    Close movie stamp task.

    Close movie stamp task.

    @return
     - @b E_OK:     close successfully.
     - @b E_SYS:    movie stamp task is closed.
*/
extern ER   MovieStampTsk_Close(void);

/**
    Trigger movie stamp update.

    Trigger movie stamp update.

    @return void
*/
extern void MovieStampTsk_TrigUpdate(void);

/**
    To register movie stamp update callback function.

    To register movie stamp update callback function.

    @return void
*/
extern void MovieStampTsk_RegUpdateCB(MOVIESTAMP_UPDATE_CB fpMovieStampUpdate);

/**
    To register movie stamp trigger update check callback function.

    To register movie stamp trigger update check callback function.

    @return void
*/

extern void MovieStampTsk_RegTrigUpdateChkCB(MOVIESTAMP_TRIGGER_UPDATE_CHECK_CB fpMovieStampTrigUpdateChkCb);

//@}
//@}
extern BOOL MovieStampTsk_IsOpen(void);

extern void MovieStamp_EncodeStampEn(UINT32 uiVEncOutPortId, UINT32 mask, UINT32 Enable);

#endif
