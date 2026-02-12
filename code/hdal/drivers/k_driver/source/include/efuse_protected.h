#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
#include "efuse_protected_na51055.h"
#elif defined(_BSP_NA51000_)
#include "efuse_protected_na51000.h"
#endif
#else
#if defined(_BSP_NA51055_)
#include "efuse_protected_na51055.h"
#endif
#endif
