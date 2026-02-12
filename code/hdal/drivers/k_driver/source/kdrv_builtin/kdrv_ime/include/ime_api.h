#ifndef __ime_api_h_
#define __ime_api_h_
#include "ime_drv.h"


int nvt_ime_api_write_reg(PIME_INFO pime_info, unsigned char ucargc, char **pucargv);
int nvt_ime_api_write_pattern(PIME_INFO pime_info, unsigned char ucargc, char **pucargv);
int nvt_ime_api_read_reg(PIME_INFO pime_info, unsigned char ucargc, char **pucargv);

int nvt_ime_emu(PIME_INFO pime_info, unsigned char argc, char **pargv);

//int nvt_kdrv_ime_test(PIME_INFO pime_info, unsigned char argc, char **pargv);

int nvt_ime_api_dump(PIME_INFO pime_info, unsigned char argc, char **pargv);


#endif

