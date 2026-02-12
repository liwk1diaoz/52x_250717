#ifndef __IPP_SIM_IOCTL_CMD_H_
#define __IPP_SIM_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define IPP_SIM_REG_LIST_NUM     1

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[IPP_SIM_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define IPP_SIM_IOC_COMMON_TYPE 'M'
#define IPP_SIM_IOC_START                   _IO(IPP_SIM_IOC_COMMON_TYPE, 1)
#define IPP_SIM_IOC_STOP                    _IO(IPP_SIM_IOC_COMMON_TYPE, 2)

#define IPP_SIM_IOC_READ_REG                _IOWR(IPP_SIM_IOC_COMMON_TYPE, 3, void*)
#define IPP_SIM_IOC_WRITE_REG               _IOWR(IPP_SIM_IOC_COMMON_TYPE, 4, void*)
#define IPP_SIM_IOC_READ_REG_LIST           _IOWR(IPP_SIM_IOC_COMMON_TYPE, 5, void*)
#define IPP_SIM_IOC_WRITE_REG_LIST          _IOWR(IPP_SIM_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
