#ifndef _SDE_DBG_H_
#define _SDE_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          sde
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#include "sde_alg.h"

#define SDE_DBG_NONE             0x00000000
#define SDE_DBG_ERR_MSG          0x00000001
#define SDE_DBG_CFG              0x00000004
#define SDE_DBG_DTS              0x00000008
#define SDE_DBG_RUN_SET          0x00000010
#define SDE_DBG_PERFORMANCE      0x00000020

#define PRINT_SDE(type, fmt, args...) {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_SDE_VAR(type, var)      {if (type) DBG_DUMP("SDE %s = %d\r\n", #var, var); }
#define PRINT_SDE_ARR(type, arr, len) {       \
	if (type) {                              \
		do {                                 \
			UINT32 i;                        \
			DBG_DUMP("SDE %s = { ", #arr);    \
			for (i = 0; i < len; i++)        \
				DBG_DUMP("%d, ", arr[i]);    \
			DBG_DUMP("}\r\n");               \
		} while (0);                         \
	};                                       \
}
#define PRINT_SDE_ERR(type, fmt, args...) {if (type) DBG_ERR(fmt, ## args); if (sde_dbg_check_err_msg(type)) DBG_ERR(fmt, ## args); }

extern UINT32 sde_dbg_get_dbg_mode(SDE_ID id);
extern void sde_dbg_set_dbg_mode(SDE_ID id, UINT32 cmd);
extern void sde_dbg_clr_dbg_mode(SDE_ID id, UINT32 cmd);
extern BOOL sde_dbg_check_err_msg(BOOL show_dbg_msg);
extern void sde_dbg_clr_err_msg(void);
extern UINT32 sde_dbg_get_err_msg(void);

#endif
