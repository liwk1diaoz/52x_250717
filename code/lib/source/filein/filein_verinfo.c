#if defined (__UITRON) || defined (__ECOS)
#include "NvtVerInfo.h"

NVTVER_VERSION_ENTRY(ImageUnit_FileIn, 1, 00, 007, 00)
#else
#include <kwrap/verinfo.h>

VOS_MODULE_VERSION(filein, 1, 02, 001, 00);
#endif
