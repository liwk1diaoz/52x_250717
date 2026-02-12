/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       dce_kit.h

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2019/06/06
*/
#ifndef _DCE_KIT_H
#define _DCE_KIT_H


int kdrv_api_dce_set_dbg_level(unsigned char argc, char **pargv);
int kdrv_api_dce_test(unsigned char argc, char **pargv);
int kdrv_api_dce_dump_info(unsigned char argc, char **pargv);
int kdrv_api_dce_test(unsigned char argc, char **pargv);
int kdrv_api_dce_hw_power_saving(unsigned char argc, char **pargv);
int kdrv_api_dce_fw_power_saving(unsigned char argc, char **pargv);

#endif

