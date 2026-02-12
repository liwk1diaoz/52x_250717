#ifndef __VPE_IOCTL_CMD_H__
#define __VPE_IOCTL_CMD_H__

#include <linux/ioctl.h>

#define VPE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[VPE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define VPE_IOC_COMMON_TYPE 'M'
#define VPE_IOC_START                   _IO(VPE_IOC_COMMON_TYPE, 1)
#define VPE_IOC_STOP                    _IO(VPE_IOC_COMMON_TYPE, 2)

#define VPE_IOC_READ_REG                _IOWR(VPE_IOC_COMMON_TYPE, 3, void*)
#define VPE_IOC_WRITE_REG               _IOWR(VPE_IOC_COMMON_TYPE, 4, void*)
#define VPE_IOC_READ_REG_LIST           _IOWR(VPE_IOC_COMMON_TYPE, 5, void*)
#define VPE_IOC_WRITE_REG_LIST          _IOWR(VPE_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
