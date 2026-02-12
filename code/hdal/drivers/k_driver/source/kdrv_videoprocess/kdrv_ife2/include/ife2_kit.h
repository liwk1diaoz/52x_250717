
#if defined (__UITRON) || defined (__ECOS)

#elif defined (__FREERTOS )

#elif defined (__KERNEL__) || defined (__LINUX)

#include "ife2_drv.h"

#endif



#if defined (__LINUX) || defined (__KERNEL__)

int nvt_ife2_api_dump(PIFE2_INFO pife2_info, unsigned char argc, char **pargv);

int nvt_kdrv_ife2_test(PIFE2_INFO pife2_info, unsigned char argc, char **pargv);

int nvt_ife2_hw_power_saving(PIFE2_INFO pmodule_info, unsigned char argc, char **pargv);

int nvt_ife2_fw_power_saving(PIFE2_INFO pmodule_info, unsigned char argc, char **pargv);


#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

int nvt_ife2_api_dump(unsigned char argc, char **pargv);

int nvt_kdrv_ife2_test(unsigned char argc, char **pargv);

int nvt_ife2_hw_power_saving(unsigned char argc, char **pargv);

int nvt_ife2_fw_power_saving(unsigned char argc, char **pargv);


#else
#endif



