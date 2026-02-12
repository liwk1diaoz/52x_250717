#if defined(__FREERTOS)
#else
#include <linux/module.h>
#endif
#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// VPE version
//=============================================================================
VOS_MODULE_VERSION(nvt_vpe, 1, 09, 000, 00);

#define VPE_VER_MAJOR       1
#define VPE_VER_MINOR       0
#define VPE_VER_MINOR2      0
#define VPE_VER_MINOR3      0
#define VPE_VER             ((VPE_VER_MAJOR << 24) | (VPE_VER_MINOR << 16) | (VPE_VER_MINOR2 << 8) | VPE_VER_MINOR3)

//=============================================================================
// extern functions
//=============================================================================
UINT32 vpe_get_version(void)
{
	return VPE_VER;
}

