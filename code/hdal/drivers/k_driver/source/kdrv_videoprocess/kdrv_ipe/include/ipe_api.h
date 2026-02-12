#ifndef __ipe_api_h_
#define __ipe_api_h_
#include "ipe_drv.h"

int nvt_ipe_api_write_reg(PIPE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ipe_api_write_pattern(PIPE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_kdrv_ipe_api_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_ipe_api_dump(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_ipe_api_read_reg(PIPE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ipe_api_iq_test(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_ipe_set_dbg_cmd(PIPE_INFO pmodule_info, unsigned char argc, char **pargv);

#endif
