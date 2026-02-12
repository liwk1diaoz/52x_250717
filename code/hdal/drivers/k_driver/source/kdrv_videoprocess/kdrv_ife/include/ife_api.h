#ifndef __module_api_h_
#define __module_api_h_

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
#include "ife_platform.h"
#else
#include "ife_drv.h"
#endif

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
int nvt_ife_api_dump(unsigned char argc, char **pargv);
#if !defined (CONFIG_NVT_SMALL_HDAL)	
int nvt_kdrv_ipp_api_ife_test(unsigned char argc, char **pargv);
#endif
#else
int nvt_ife_api_write_reg(PIFE_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ife_api_write_pattern(PIFE_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ife_api_read_reg(PIFE_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ife_api_dump(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
#if !defined (CONFIG_NVT_SMALL_HDAL)	
int nvt_kdrv_ipp_api_ife_test(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_ife_api_hw_power_saving(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_ife_api_fw_power_saving(PIFE_MODULE_INFO pmodule_info, unsigned char argc, char **pargv);
#endif
#endif
#endif
