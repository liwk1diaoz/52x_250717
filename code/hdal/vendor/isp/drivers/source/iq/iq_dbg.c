#include "kwrap/type.h"
#include "iq_dbg.h"

#define MAX_CNT 10
static UINT32 iq_dbg_msg[IQ_ID_MAX_NUM] = {0};
static UINT32 wrn_msg_cnt;

void iq_print_arr_uint32(UINT32 *arr, UINT32 len, UINT32 w_len)
{
	UINT32 i;
	if (!((w_len == 32) || (w_len == 16) || (w_len == 10))) {
		w_len = 10;
		DBG_DUMP("not support width (%d)!!", w_len);
	};
	for (i = 0; i < len; i = i + w_len) {
		if (w_len == 32) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15], arr[i + 16], arr[i + 17], arr[i + 18], arr[i + 19]
			, arr[i + 20], arr[i + 21], arr[i + 22], arr[i + 23], arr[i + 24], arr[i + 25], arr[i + 26], arr[i + 27], arr[i + 28], arr[i + 29]
			, arr[i + 30], arr[i + 31]);
		} else if (w_len == 16) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15]);
		} else {
			if ((len - i) >= 10) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]);
			} else if ((len - i) >= 9) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8]);
			} else if ((len - i) >= 8) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7]);
			} else if ((len - i) >= 7) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6]);
			} else if ((len - i) >= 6) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5]);
			} else if ((len - i) >= 5) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4]);
			} else if ((len - i) >= 4) {
				DBG_DUMP("%5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3]);
			} else if ((len - i) >= 3) {
				DBG_DUMP("%5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2]);
			} else if ((len - i) >= 2) {
				DBG_DUMP("%5d, %5d, ", arr[i], arr[i + 1]);
			} else if ((len - i) >= 1) {
				DBG_DUMP("%5d, ", arr[i]);
			}
		}
	};
	DBG_DUMP("}\r\n");
}

void iq_print_arr_uint16(UINT16 *arr, UINT32 len, UINT32 w_len)
{
	UINT32 i;
	if (!((w_len == 32) || (w_len == 16) || (w_len == 10))) {
		w_len = 10;
		DBG_DUMP("not support width (%d)!!", w_len);
	};
	for (i = 0; i < len; i = i + w_len) {
		if (w_len == 32) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15], arr[i + 16], arr[i + 17], arr[i + 18], arr[i + 19]
			, arr[i + 20], arr[i + 21], arr[i + 22], arr[i + 23], arr[i + 24], arr[i + 25], arr[i + 26], arr[i + 27], arr[i + 28], arr[i + 29]
			, arr[i + 30], arr[i + 31]);
		} else if (w_len == 16) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15]);
		} else {
			if ((len - i) >= 10) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]);
			} else if ((len - i) >= 9) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8]);
			} else if ((len - i) >= 8) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7]);
			} else if ((len - i) >= 7) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6]);
			} else if ((len - i) >= 6) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5]);
			} else if ((len - i) >= 5) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4]);
			} else if ((len - i) >= 4) {
				DBG_DUMP("%5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3]);
			} else if ((len - i) >= 3) {
				DBG_DUMP("%5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2]);
			} else if ((len - i) >= 2) {
				DBG_DUMP("%5d, %5d, ", arr[i], arr[i + 1]);
			} else if ((len - i) >= 1) {
				DBG_DUMP("%5d, ", arr[i]);
			}
		}
	};
	DBG_DUMP("}\r\n");
}

void iq_print_arr_uint8(UINT8 *arr, UINT32 len, UINT32 w_len)
{
	UINT32 i;
	if (!((w_len == 32) || (w_len == 16) || (w_len == 10))) {
		w_len = 10;
		DBG_DUMP("not support width (%d)!!", w_len);
	};
	for (i = 0; i < len; i = i + w_len) {
		if (w_len == 32) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15], arr[i + 16], arr[i + 17], arr[i + 18], arr[i + 19]
			, arr[i + 20], arr[i + 21], arr[i + 22], arr[i + 23], arr[i + 24], arr[i + 25], arr[i + 26], arr[i + 27], arr[i + 28], arr[i + 29]
			, arr[i + 30], arr[i + 31]);
		} else if (w_len == 16) {
			DBG_DUMP("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d "
			, arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]
			, arr[i + 10], arr[i + 11], arr[i + 12], arr[i + 13], arr[i + 14], arr[i + 15]);
		} else {
			if ((len - i) >= 10) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]);
			} else if ((len - i) >= 9) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8]);
			} else if ((len - i) >= 8) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7]);
			} else if ((len - i) >= 7) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6]);
			} else if ((len - i) >= 6) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5]);
			} else if ((len - i) >= 5) {
				DBG_DUMP("%5d, %5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4]);
			} else if ((len - i) >= 4) {
				DBG_DUMP("%5d, %5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3]);
			} else if ((len - i) >= 3) {
				DBG_DUMP("%5d, %5d, %5d, ", arr[i], arr[i + 1], arr[i + 2]);
			} else if ((len - i) >= 2) {
				DBG_DUMP("%5d, %5d, ", arr[i], arr[i + 1]);
			} else if ((len - i) >= 1) {
				DBG_DUMP("%5d, ", arr[i]);
			}
		}
	};
	DBG_DUMP("}\r\n");
}

void iq_print_arr_uint32_hex(UINT32 *arr, UINT32 len, UINT32 w_len)
{
	UINT32 i;
	for (i = 0; i < len; i = i + 10) {
		if ((len - i) >= 10) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8], arr[i + 9]);
		} else if ((len - i) >= 9) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7], arr[i + 8]);
		} else if ((len - i) >= 8) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6], arr[i + 7]);
		} else if ((len - i) >= 7) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5], arr[i + 6]);
		} else if ((len - i) >= 6) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4], arr[i + 5]);
		} else if ((len - i) >= 5) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3], arr[i + 4]);
		} else if ((len - i) >= 4) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2], arr[i + 3]);
		} else if ((len - i) >= 3) {
			DBG_DUMP("0x%08x, 0x%08x, 0x%08x, ", arr[i], arr[i + 1], arr[i + 2]);
		} else if ((len - i) >= 2) {
			DBG_DUMP("0x%08x, 0x%08x, ", arr[i], arr[i + 1]);
		} else if ((len - i) >= 1) {
			DBG_DUMP("0x%08x, ", arr[i]);
		}
	};
	DBG_DUMP("}\r\n");
}

UINT32 iq_dbg_get_dbg_mode(IQ_ID id)
{
	if (id >= IQ_ID_MAX_NUM) {
		//DBG_ERR("iq id out of range (%d) \r\n", id);
		return iq_dbg_msg[0];
	}

	return iq_dbg_msg[id];
}

void iq_dbg_set_dbg_mode(IQ_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= IQ_ID_MAX_NUM) {
		for (i = 0; i < IQ_ID_MAX_NUM; i++) {
			iq_dbg_msg[i] = cmd;
		}
	} else {
		iq_dbg_msg[id] = cmd;
	}

	if (cmd == IQ_DBG_NONE) {
		iq_dbg_clr_wrn_msg();
	}
}

void iq_dbg_clr_dbg_mode(IQ_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= IQ_ID_MAX_NUM) {
		for (i = 0; i < IQ_ID_MAX_NUM; i++) {
			iq_dbg_msg[i] &= ~cmd;
		}
	} else {
		iq_dbg_msg[id] &= ~cmd;
	}
}

BOOL iq_dbg_check_wrn_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((wrn_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	wrn_msg_cnt++;

	return rt;
}

void iq_dbg_clr_wrn_msg(void)
{
	wrn_msg_cnt = 0;
}

UINT32 iq_dbg_get_wrn_msg(void)
{
	return wrn_msg_cnt;
}
