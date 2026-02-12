#ifndef __VPE_API_H__
#define __VPE_API_H__

#include "vpe_drv.h"


int nvt_vpe_api_write_reg(PVPE_INFO pvpe_info, unsigned char ucargc, char **pucargv);
int nvt_vpe_api_write_pattern(PVPE_INFO pvpe_info, unsigned char ucargc, char **pucargv);
int nvt_vpe_api_read_reg(PVPE_INFO pvpe_info, unsigned char ucargc, char **pucargv);

int nvt_vpe_emu(PVPE_INFO pvpe_info, unsigned char argc, char **pargv);

//int nvt_kdrv_vpe_test(PVPE_INFO pvpe_info, unsigned char argc, char **pargv);

int nvt_vpe_api_dump(PVPE_INFO pvpe_info, unsigned char argc, char **pargv);


#endif

