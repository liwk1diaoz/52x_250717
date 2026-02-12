/*
    IFE module driver

    NT96680 IFE registers header file.

    @file       ife_int.h
    @ingroup    mIDrvIPP_IFE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#ifndef _IFE_INT_H
#define _IFE_INT_H

//#define IFE_ROWDEFECT_MAX_NUM       16
#define IFE_DMA_CACHE_HANDLE        (1)
/*
    IFE Operation
*/
typedef enum {
	IFE_OP_OPEN         = 0,    ///< Open operation
	IFE_OP_CLOSE        = 1,    ///< Close operation
	IFE_OP_START        = 2,    ///< Start operation
	IFE_OP_SETMODE      = 3,    ///< Set mode operation
	IFE_OP_CHGSIZE      = 4,    ///< Change size operation
	IFE_OP_CHGPARAM     = 5,    ///< Change parameter operation
	IFE_OP_PAUSE        = 6,    ///< Pause operation
	IFE_OP_HWRESET      = 7,    ///< HW Reset operation
	ENUM_DUMMY4WORD(IFEOPERATION)
} IFEOPERATION;

/*
    IFE Operation update state machine
*/
typedef enum {
	NOTUPDATE   = 0,            ///< Do not update state machine, Check only
	UPDATE      = 1,            ///< Check and update state machine
	ENUM_DUMMY4WORD(IFESTATUSUPDATE)
} IFESTATUSUPDATE;

/*
    IFE State machine
*/
typedef enum {
	IFE_ENGINE_IDLE      = 0,   ///< Idle state
	IFE_ENGINE_READY     = 1,   ///< Ready state
	IFE_ENGINE_PAUSE     = 2,   ///< Pause state
	IFE_ENGINE_RUN       = 3,   ///< Run state
	ENUM_DUMMY4WORD(IFE_ENGINE_STATUS)
} IFE_ENGINE_STATUS;

typedef enum {
	IFE_FROM_DRAM = 0,       ///< Input from DRAM
	IFE_FROM_SIE = 1,        ///< debug only
	//IFE_FROM_RDE = 2,        ///< Not support in NT96680
	IFE_FROM_RHE = 3,        ///< Input from RHE
	ENUM_DUMMY4WORD(IFE_IN_PATH)
} IFE_IN_PATH;

typedef enum {
	IFE_TO_DRAM = 0,            ///< Output to DRAM
	IFE_TO_DCE = 1,             ///< Output to DCE
	ENUM_DUMMY4WORD(IFE_OUT_PATH)
} IFE_OUT_PATH;


#endif
