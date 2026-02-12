#include "kwrap/cpu.h"
#include "../eis_int.h"
#include "eis_ioctl.h"

#define EFAULT 14
#define EINVAL (1)

int eis_open(char* file, int flag)
{
	return 1;
}

int eis_close(int fd)
{
	return 0;
}

int eis_ioctl(int fd, unsigned int cmd, void *arg)
{
	DBG_IND("cmd = 0x%x\r\n", cmd);
	switch (cmd) {
	case EIS_IOC_SET_STATE: {
			EIS_STATE msg;

			msg = *(EIS_STATE *)arg;
			if (msg == EIS_STATE_OPEN) {
				if (_eis_tsk_open()) {
					return -EFAULT;
				}
			} else if (msg == EIS_STATE_CLOSE) {
				if (_eis_tsk_close()) {
					return -EFAULT;
				}
			} else if (msg == EIS_STATE_IDLE) {
				if (_eis_tsk_idle()) {
					return -EFAULT;
				}
			} else {
				DBG_IND("unsported state %d\r\n", msg);
				return -EFAULT;
			}
		}
		break;
	case EIS_IOC_WAIT_PROC:
			_eis_tsk_get_msg((EIS_WAIT_PROC_INFO *)arg);
		break;
	case EIS_IOC_PUSH_DATA:
			_eis_tsk_put_out((EIS_PUSH_DATA *)arg, NULL);
		break;
	case EIS_IOC_PATH_INFO:
			_eis_tsk_set_path_info((EIS_PATH_INFO *)arg);
		break;
	case EIS_IOC_DEBUG_SIZE: {
			UINT32 dbg_size;

			dbg_size = *(UINT32 *)arg;
			_eis_tsk_set_debug_size(dbg_size);
		}
		break;
	default:
		DBG_ERR("unknown cmd 0x%x\r\n", cmd);
		return -EINVAL;
	}
	return 0;
}
