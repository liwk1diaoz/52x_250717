#if defined(__FREERTOS)
#else
#include <linux/module.h>
#endif
#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// SDE version
//=============================================================================
VOS_MODULE_VERSION(nvt_sde, 1, 02, 000, 00);

#define SDE_VER_MAJOR       1
#define SDE_VER_MINOR       0
#define SDE_VER_MINOR2      0
#define SDE_VER_MINOR3      0
#define SDE_VER             ((SDE_VER_MAJOR << 24) | (SDE_VER_MINOR << 16) | (SDE_VER_MINOR2 << 8) | SDE_VER_MINOR3)

//=============================================================================
// extern functions
//=============================================================================
UINT32 sde_get_version(void)
{
	return SDE_VER;
}

