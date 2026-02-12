

#if defined (__UITRON) || defined (__ECOS)

#elif defined (__FREERTOS )

#elif defined (__KERNEL__) || defined (__LINUX)

#include "ime_drv.h"

#endif



#if defined (__LINUX) || defined (__KERNEL__)

int nvt_ime_api_dump(PIME_INFO pime_info, unsigned char argc, char **pargv);

int nvt_kdrv_ime_test(PIME_INFO pime_info, unsigned char argc, char **pargv);

int nvt_ime_hw_power_saving(PIME_INFO pmodule_info, unsigned char argc, char **pargv);

int nvt_ime_fw_power_saving(PIME_INFO pmodule_info, unsigned char argc, char **pargv);

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

int nvt_ime_api_dump(unsigned char argc, char **pargv);

int nvt_kdrv_ime_test(unsigned char argc, char **pargv);

int nvt_ime_hw_power_saving(unsigned char argc, char **pargv);

int nvt_ime_fw_power_saving(unsigned char argc, char **pargv);

#else
#endif

