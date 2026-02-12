/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_heap.c

    @brief      nvtmpp heap memory alloc , free.


    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#include "Utility.h"
#include "KerCPU.h"
#include "nvtmpp_heap.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nvtmpp_heap
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"



