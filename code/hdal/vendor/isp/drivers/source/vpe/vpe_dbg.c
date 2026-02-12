#if defined(__KERNEL__)
#include <linux/kernel.h>
#endif
#include "kwrap/type.h"
#include "vpe_dbg.h"

#define MAX_CNT 10
static UINT32 vpe_dbg_msg[VPE_ID_MAX_NUM] = {0};
static UINT32 err_msg_cnt;

UINT32 vpe_dbg_get_dbg_mode(VPE_ID id)
{
	if (id >= VPE_ID_MAX_NUM) {
		return vpe_dbg_msg[0];
	}

	return vpe_dbg_msg[id];
}

void vpe_dbg_set_dbg_mode(VPE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= VPE_ID_MAX_NUM) {
		for (i = 0; i < VPE_ID_MAX_NUM; i++) {
			vpe_dbg_msg[i] = cmd;
		}
	} else {
		vpe_dbg_msg[id] = cmd;
	}

	if (cmd == VPE_DBG_NONE) {
		vpe_dbg_clr_err_msg();
	}
}

void vpe_dbg_clr_dbg_mode(VPE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= VPE_ID_MAX_NUM) {
		for (i = 0; i < VPE_ID_MAX_NUM; i++) {
			vpe_dbg_msg[i] &= ~cmd;
		}
	} else {
		vpe_dbg_msg[id] &= ~cmd;
	}
}

BOOL vpe_dbg_check_err_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((err_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	err_msg_cnt++;

	return rt;
}

void vpe_dbg_clr_err_msg(void)
{
	err_msg_cnt = 0;
}

UINT32 vpe_dbg_get_err_msg(void)
{
	return err_msg_cnt;
}
