/*
    IPE module driver

    NT96520 IPE registers header file.

    @file       ipe_int.h
    @ingroup    mIDrvIPP_IPE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _IPE_INT_H
#define _IPE_INT_H

#include "ipe_reg.h"
#include "ipe_lib.h"

typedef enum {
	IPE_STATUS_IDLE,
	IPE_STATUS_READY,
	IPE_STATUS_RUN,
	IPE_STATUS_PAUSE
} IPE_STATUS;

typedef enum {
	IPE_OP_OPEN,
	IPE_OP_CLOSE,
	IPE_OP_SETMODE,
	IPE_OP_START,
	IPE_OP_PAUSE,
	IPE_OP_HWRESET,
	IPE_OP_CHGSIZE,
	IPE_OP_CHGPARAM,
	IPE_OP_CHGINOUT,
	IPE_OP_LOADLUT,
	IPE_OP_READLUT,
	IPE_OP_CHGPARAMALL
} IPE_OPERATION;

/*
    IPE update state machine
*/
typedef enum {
	NOTUPDATE   = 0,
	UPDATE      = 1,
	ENUM_DUMMY4WORD(IPESTATUSUPDATE)
} IPESTATUSUPDATE;


/**
    IPE structure - IPE size info.
*/
//@{
typedef struct {
	IPE_OPMODE ipe_mode;             ///< IPE operation mode
	IPE_IMG_SIZE in_size_y;           ///< Input image H V size, effective if StripeMode is IPE_SSTP or IPE_AUTOSTP. If StripeMode is IPE_MSTP, HSTRIPE and VSTRIPE must be input
	IPE_INOUT_FORMAT in_fmt;         ///< Input YCC format
	IPE_INOUT_FORMAT out_fmt;        ///< Output YCC format
	BOOL yc_to_dram_en;              ///< Enable of YC data to DRAM
	IPE_DMAOSEL dram_out_sel;          ///< DMA output channel data selection
	IPE_ETH_SIZE ethout_info;      ///< ETH output information
} IPE_STRPINFO;
//@}


//#define DEFAULT_HN      672//336//528 //88
//#define IPE_MAXHSTRP    (DEFAULT_HN)//336 //528 //88
//#define IPE_MINHSTRP    4  // 660 can be 4

#define IPE_MAXVSTRP    8191
#define IPE_MINVSTRP    16
#define IPE_VLINE    8

extern volatile NT98520_IPE_REGISTER_STRUCT *ipeg;


#endif
