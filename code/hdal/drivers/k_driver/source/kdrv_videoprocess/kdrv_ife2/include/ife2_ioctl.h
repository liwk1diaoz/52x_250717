#ifndef __IFE2_IOCTL_CMD_H_
#define __IFE2_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define IFE2_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[IFE2_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define IFE2_IOC_COMMON_TYPE 'M'
#define IFE2_IOC_START                   _IO(IFE2_IOC_COMMON_TYPE, 1)
#define IFE2_IOC_STOP                    _IO(IFE2_IOC_COMMON_TYPE, 2)

#define IFE2_IOC_READ_REG                _IOWR(IFE2_IOC_COMMON_TYPE, 3, void*)
#define IFE2_IOC_WRITE_REG               _IOWR(IFE2_IOC_COMMON_TYPE, 4, void*)
#define IFE2_IOC_READ_REG_LIST           _IOWR(IFE2_IOC_COMMON_TYPE, 5, void*)
#define IFE2_IOC_WRITE_REG_LIST          _IOWR(IFE2_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
