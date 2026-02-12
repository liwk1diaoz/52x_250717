#ifndef __SDE_API_h_
#define __SDE_API_h_
#include "sde_drv.h"

int nvt_sde_api_write_reg(PSDE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_sde_api_write_pattern(PSDE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_kdrv_sde_api_test_drv(PSDE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_kdrv_sde_api_test_kdrv(PSDE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_sde_api_read_reg(PSDE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
