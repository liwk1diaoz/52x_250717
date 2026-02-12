#ifndef __ife2_api_h_
#define __ife2_api_h_
#include "ife2_drv.h"

int nvt_ife2_api_write_reg(PIFE2_INFO pife2_info, unsigned char ucargc, char **pucargv);
int nvt_ife2_api_write_pattern(PIFE2_INFO pife2_info, unsigned char ucargc, char **pucargv);
int nvt_ife2_api_read_reg(PIFE2_INFO pife2_info, unsigned char ucargc, char **pucargv);

//int nvt_kdrv_ife2_test(PIFE2_INFO pime_info, unsigned char argc, char **pargv);

//int nvt_ife2_api_dump(PIFE2_INFO pife2_info, unsigned char argc, char **pargv);


#endif
