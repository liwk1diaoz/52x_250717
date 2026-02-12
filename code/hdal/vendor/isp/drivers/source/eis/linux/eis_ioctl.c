#if defined(__LINUX)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <mach/fmem.h>
#endif
#include "../eis_int.h"
#include "eis_ioctl.h"

static void copy_cb(void * dest, void * src, int len)
{
	if (unlikely(copy_from_user(dest, (void __user *)src, len))) {
		DBG_ERR("os copy error\r\n");
	}
}

long eis_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int __user *argp = (int __user *)arg;

	DBG_IND("cmd = 0x%x\r\n", cmd);

	switch (cmd) {
	case EIS_IOC_SET_STATE: {
			EIS_STATE msg;

			//memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
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
	case EIS_IOC_WAIT_PROC: {
			static EIS_WAIT_PROC_INFO msg;

			//memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			_eis_tsk_get_msg(&msg);

			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case EIS_IOC_PUSH_DATA: {
			EIS_PUSH_DATA msg;

			//memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			_eis_tsk_put_out(&msg, copy_cb);
		}
		break;
	case EIS_IOC_PATH_INFO: {
			EIS_PATH_INFO msg;

			//memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			_eis_tsk_set_path_info(&msg);
		}
		break;
	case EIS_IOC_DEBUG_SIZE: {
			UINT32 dbg_size;

			if (unlikely(copy_from_user(&dbg_size, argp, sizeof(dbg_size)))) {
				return -EFAULT;
			}
			_eis_tsk_set_debug_size(dbg_size);
		}
		break;
	default:
		DBG_ERR("unknown cmd 0x%x\r\n", cmd);
		return -EINVAL;
	}
	return 0;
}

