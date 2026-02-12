/*
    DCE module driver

    NT96520 DCE registers header file.

    @file       dce_int.h

    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _DCE_INT_H
#define _DCE_INT_H


#define DCE_CTRL_SRAM     (0xffffffff)
#define DCE_NOT_CTRL_SRAM (0x00000000)
#define LUT2D_FRAC_BIT_NUM 24

/**
    @name DCE enum - Status of State Machine
*/
//@{
typedef enum {
	DCE_STATE_IDLE  = 0,    ///< Idel status
	DCE_STATE_READY = 1,    ///< Ready status
	DCE_STATE_PAUSE = 2,    ///< Pause status
	DCE_STATE_RUN   = 3,    ///< Run status
	ENUM_DUMMY4WORD(DCE_ENGINE_STATE)
} DCE_ENGINE_STATE;
//@}

/**
    @name DCE enum - Status of State Machine
*/
//@{
typedef enum {
	DCE_OP_OPEN         = 0,    ///< Open operation
	DCE_OP_CLOSE        = 1,    ///< Close operation
	DCE_OP_START        = 2,    ///< Start operation
	DCE_OP_SETMODE      = 3,    ///< Set mode operation
	DCE_OP_CHGPARAM     = 4,    ///< Change parameter operation
	DCE_OP_PAUSE        = 5,    ///< Pause operation
	DCE_OP_RESET        = 6,    ///< SW reset operation
	ENUM_DUMMY4WORD(DCE_OPERATION)
} DCE_OPERATION;
//@}

/**
    @name DCE enum - Engine semaphore lock status
*/
//@{
typedef enum {
	DCE_SEM_UNLOCKED = 0,   ///< Engine is not in used
	DCE_SEM_LOCKED = 1,     ///< Engine is in used
	ENUM_DUMMY4WORD(DCE_SEM_LOCK_STATUS)
} DCE_SEM_LOCK_STATUS;
//@}

/**
    DCE load timing selection

    Select load timing - start, frame end, immediate
\n Used for dce_set_load().
*/
typedef enum {
	DCE_START_LOAD_TYPE         = 0,  ///< Frame start loaded
	DCE_FRMEND_LOAD_TYPE        = 1,  ///< Frame end loaded
	DCE_IMD_LOAD_TYPE           = 2,  ///< Immediately load
	DCE_DIRECT_START_LOAD_TYPE  = 3,  ///< Frame start loaded
	ENUM_DUMMY4WORD(DCE_LOAD_TYPE)
} DCE_LOAD_TYPE;

typedef struct {
	BOOL enable;
	BOOL width_eq_lofst;
	UINT32 addr;
	UINT32 size;
} DCE_OUT_DMA_INFO;

ER dce_cal_stripe_with_ime_limitation(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result);

#endif
