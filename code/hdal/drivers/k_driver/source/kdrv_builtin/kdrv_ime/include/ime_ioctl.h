#ifndef __IME_IOCTL_CMD_H_
#define __IME_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define IME_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[IME_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define IME_IOC_COMMON_TYPE 'M'
#define IME_IOC_START                   _IO(IME_IOC_COMMON_TYPE, 1)
#define IME_IOC_STOP                    _IO(IME_IOC_COMMON_TYPE, 2)

#define IME_IOC_READ_REG                _IOWR(IME_IOC_COMMON_TYPE, 3, void*)
#define IME_IOC_WRITE_REG               _IOWR(IME_IOC_COMMON_TYPE, 4, void*)
#define IME_IOC_READ_REG_LIST           _IOWR(IME_IOC_COMMON_TYPE, 5, void*)
#define IME_IOC_WRITE_REG_LIST          _IOWR(IME_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
