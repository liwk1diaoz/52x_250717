#ifndef _VPE_DBG_H_
#define _VPE_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          vpe
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#include "vpe_alg.h"

#define VPE_DBG_NONE             0x00000000
#define VPE_DBG_ERR_MSG          0x00000001
#define VPE_DBG_CFG              0x00000004
#define VPE_DBG_DTS              0x00000008
#define VPE_DBG_UPDATE           0x00000010


#define PRINT_VPE(type, fmt, args...) {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_VPE_VAR(type, var)      {if (type) DBG_DUMP("VPE %s = %d\r\n", #var, var); }
#define PRINT_VPE_ARR(type, arr, len) {       \
	if (type) {                              \
		do {                                 \
			UINT32 i;                        \
			DBG_DUMP("VPE %s = { ", #arr);    \
			for (i = 0; i < len; i++)        \
				DBG_DUMP("%d, ", arr[i]);    \
			DBG_DUMP("}\r\n");               \
		} while (0);                         \
	};                                       \
}
#define PRINT_VPE_ERR(type, fmt, args...) {if (type) DBG_ERR(fmt, ## args); if (vpe_dbg_check_err_msg(type)) DBG_ERR(fmt, ## args); }

extern UINT32 vpe_dbg_get_dbg_mode(VPE_ID id);
extern void vpe_dbg_set_dbg_mode(VPE_ID id, UINT32 cmd);
extern void vpe_dbg_clr_dbg_mode(VPE_ID id, UINT32 cmd);
extern BOOL vpe_dbg_check_err_msg(BOOL show_dbg_msg);
extern void vpe_dbg_clr_err_msg(void);
extern UINT32 vpe_dbg_get_err_msg(void);

#endif
