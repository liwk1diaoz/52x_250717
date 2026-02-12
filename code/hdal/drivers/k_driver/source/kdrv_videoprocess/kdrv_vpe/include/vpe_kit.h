#ifndef __VPE_KIT_H__
#define __VPE_KIT_H__

#if defined (__UITRON) || defined (__ECOS)

#elif defined (__FREERTOS )

#elif defined (__KERNEL__) || defined (__LINUX)

#include "vpe_drv.h"

#endif



#if defined (__LINUX) || defined (__KERNEL__)

int nvt_vpe_api_dump(PVPE_INFO pvpe_info, unsigned char argc, char **pargv);

int nvt_kdrv_vpe_test(PVPE_INFO pvpe_info, unsigned char argc, char **pargv);

int nvt_vpe_fw_power_saving(PVPE_INFO pmodule_info, unsigned char argc, char **pargv);

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

int nvt_vpe_api_dump(unsigned char argc, char **pargv);

int nvt_kdrv_vpe_test(unsigned char argc, char **pargv);

#else
#endif


#endif

