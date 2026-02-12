#if defined(__KERNEL__)
#include <linux/kernel.h>
#endif
#include "kwrap/type.h"
#include "sde_dbg.h"

#define MAX_CNT 10
static UINT32 sde_dbg_msg[SDE_ID_MAX_NUM] = {0};
static UINT32 err_msg_cnt;

UINT32 sde_dbg_get_dbg_mode(SDE_ID id)
{
	if (id >= SDE_ID_MAX_NUM) {
		return sde_dbg_msg[0];
	}

	return sde_dbg_msg[id];
}

void sde_dbg_set_dbg_mode(SDE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= SDE_ID_MAX_NUM) {
		for (i = 0; i < SDE_ID_MAX_NUM; i++) {
			sde_dbg_msg[i] = cmd;
		}
	} else {
		sde_dbg_msg[id] = cmd;
	}

	if (cmd == SDE_DBG_NONE) {
		sde_dbg_clr_err_msg();
	}
}

void sde_dbg_clr_dbg_mode(SDE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= SDE_ID_MAX_NUM) {
		for (i = 0; i < SDE_ID_MAX_NUM; i++) {
			sde_dbg_msg[i] &= ~cmd;
		}
	} else {
		sde_dbg_msg[id] &= ~cmd;
	}
}

BOOL sde_dbg_check_err_msg(BOOL show_dbg_msg)
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

void sde_dbg_clr_err_msg(void)
{
	err_msg_cnt = 0;
}

UINT32 sde_dbg_get_err_msg(void)
{
	return err_msg_cnt;
}
