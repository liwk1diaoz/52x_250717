#include "kdrv_vdocdc_dbg.h"
#if defined(VDOCDC_EMU)
unsigned int kdrv_vdocdc_debug_level = (NVT_DBG_WRN | NVT_DBG_ERR | NVT_DBG_INFO | NVT_DBG_IND | NVT_DBG_MSG);
#else
unsigned int kdrv_vdocdc_debug_level = (NVT_DBG_WRN | NVT_DBG_ERR);
#endif

