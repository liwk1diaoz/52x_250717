/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       ipe_kit.h

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2019/06/06
*/
#ifndef _IPE_KIT_H
#define _IPE_KIT_H

#if defined (__KERNEL__)

#include "ipe_drv.h"

#elif defined (__FREERTOS)

#else

#endif



int kdrv_api_ipe_test(unsigned char argc, char **pargv);
int kdrv_api_ipe_iq_debug(unsigned char argc, char **pargv);
int kdrv_api_ipe_dump_info(unsigned char argc, char **pargv);
int kdrv_api_ipe_set_dbg_level(unsigned char argc, char **pargv);
int kdrv_api_ipe_set_dbg_cmd(unsigned char argc, char **pargv);

#if defined (__KERNEL__)

int nvt_ipe_hw_power_saving(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);

int nvt_ipe_fw_power_saving(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);

#elif defined (__FREERTOS)

int nvt_ipe_hw_power_saving(unsigned char argc, char **pargv);

int nvt_ipe_fw_power_saving(unsigned char argc, char **pargv);

#else

#endif


#endif

