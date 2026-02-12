/*
    SDE module driver
    For stereo depth construction
    NT96680 SDE registers header file.

    @file       sde_int.h
    @ingroup    mIDrvIPP_SDE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/


#ifndef _SDE_INT_H
#define _SDE_INT_H

#include "sde_lib.h"
//#ifdef __cplusplus
//extern "C" {
//#endif

//#include "..\\IPP_AP_Driver_Common.h"

//#define SDE_REG_ADDR(ofs)       (IOADDR_SDE_REG_BASE+(ofs))    //  in "EngineDriver\\IPP_Driver_Common.h"
//#define SDE_SETREG(ofs,value)   (*(UINT32 volatile *)(IOADDR_SDE_REG_BASE + (ofs)) = (UINT32)(value))
//#define SDE_GETREG(ofs)         (*(UINT32 volatile *)(IOADDR_SDE_REG_BASE + (ofs)))
//#define SDE_GETDATA(addr, ofs)  (*(UINT32 volatile *)(addr + (ofs)))

//#define SDE_SETREG(ofs, value)      OUTW((IOADDR_SDE_REG_BASE + (ofs)), (value))
//#define SDE_GETREG(ofs)             INW(IOADDR_SDE_REG_BASE + (ofs))
//#define SDE_GETDATA(addr, ofs)      INW(addr + (ofs))

#define SDE_DMA_CACHE_HANDLE        1

// set constraint variable here

/*
    SDE Operation
*/
typedef enum {
	SDE_OP_OPEN         = 0,    ///< Open operation
	SDE_OP_CLOSE        = 1,    ///< Close operation
	SDE_OP_START        = 2,    ///< Start operation
	SDE_OP_SETMODE      = 3,    ///< Set mode operation
	SDE_OP_CHGPARAM     = 4,    ///< Change parameter operation
	SDE_OP_PAUSE        = 5,    ///< Pause operation
	ENUM_DUMMY4WORD(SDEOPERATION)
} SDEOPERATION;

/*
    SDE Operation update state machine
*/
typedef enum {
	NOTUPDATE   = 0,            ///< Do not update state machine, Check only
	UPDATE      = 1,            ///< Check and update state machine
	ENUM_DUMMY4WORD(SDESTATUSUPDATE)
} SDESTATUSUPDATE;

/*
    SDE State machine
*/
typedef enum {
	SDE_ENGINE_IDLE      = 0,   ///< Idle state
	SDE_ENGINE_READY     = 1,   ///< Ready state
	SDE_ENGINE_PAUSE     = 2,   ///< Pause state
	SDE_ENGINE_RUN       = 3,   ///< Run state
	ENUM_DUMMY4WORD(SDE_ENGINE_STATUS)
} SDE_ENGINE_STATUS;

//#ifdef __cplusplus
//}
//#endif

#endif