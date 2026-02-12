#ifndef __dce_api_h_
#define __dce_api_h_
#include "dce_drv.h"

int nvt_dce_api_write_reg(PDCE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dce_api_write_pattern(PDCE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dce_api_read_reg(PDCE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dce_show_stripe_settings(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_dce_api_dump(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);

#if !defined (CONFIG_NVT_SMALL_HDAL)
int nvt_dce_api_control_api_log(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_dce_api_test_kdrv_run_d2d(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_dce_api_hw_power_saving(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_dce_api_fw_power_saving(PDCE_INFO pmodule_info, unsigned char argc, char **pargv);
#endif

#endif
