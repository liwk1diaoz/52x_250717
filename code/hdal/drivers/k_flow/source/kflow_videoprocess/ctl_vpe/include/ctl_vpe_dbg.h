#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define __MODULE__	ctl_vpe
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "comm/hwclock.h"

extern unsigned int ctl_vpe_debug_level;

typedef enum _CTL_VPE_DBG_LVL {
	CTL_VPE_DBG_LVL_NONE = 0,
	CTL_VPE_DBG_LVL_ERR,
	CTL_VPE_DBG_LVL_WRN,
	CTL_VPE_DBG_LVL_IND,
	CTL_VPE_DBG_LVL_TRC,
	CTL_VPE_DBG_LVL_MAX,
	ENUM_DUMMY4WORD(CTL_VPE_DBG_LVL)
} CTL_VPE_DBG_LVL;

#define ctl_vpe_dbg_err(fmt, args...)    { if (ctl_vpe_debug_level >= CTL_VPE_DBG_LVL_ERR) { DBG_ERR(fmt, ##args);  }}
#define ctl_vpe_dbg_wrn(fmt, args...)    { if (ctl_vpe_debug_level >= CTL_VPE_DBG_LVL_WRN) { DBG_WRN(fmt, ##args);  }}
#define ctl_vpe_dbg_ind(fmt, args...)    { if (ctl_vpe_debug_level >= CTL_VPE_DBG_LVL_IND) { DBG_DUMP("[%s]: " fmt, __func__, ##args); }}
#define ctl_vpe_dbg_trc(fmt, args...)    { if (ctl_vpe_debug_level >= CTL_VPE_DBG_LVL_TRC) { DBG_DUMP("[%s]: " fmt, __func__, ##args); }}

#endif



