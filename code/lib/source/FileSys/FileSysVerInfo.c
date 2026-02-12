/*
    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.

    @file       FileSysVerInfo.c
    @ingroup    mISYSFileSys

    @brief      FileSys version and build date information
                FileSys version and build date information

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2015/01/09
*/

/** \addtogroup mISYSFileSys */
//@{
#if defined (__UITRON) || defined (__ECOS)
#include "NvtVerInfo.h"

NVTVER_VERSION_ENTRY(FileSys, 1, 00, 006, 00)
#else
#include <kwrap/verinfo.h>

VOS_MODULE_VERSION(FileSys, 1, 00, 000, 00);
#endif
//@}

