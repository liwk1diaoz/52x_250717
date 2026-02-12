#ifndef __DISPOBJ_IOCTL_CMD_H_
#define __DISPOBJ_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int ui_addr;
	unsigned int ui_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int ui_count;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define DISPOBJ_IOC_COMMON_TYPE 'M'
#define DISPOBJ_IOC_START                   _IO(DISPOBJ_IOC_COMMON_TYPE, 1)
#define DISPOBJ_IOC_STOP                    _IO(DISPOBJ_IOC_COMMON_TYPE, 2)

#define DISPOBJ_IOC_READ_REG                _IOWR(DISPOBJ_IOC_COMMON_TYPE, 3, void*)
#define DISPOBJ_IOC_WRITE_REG               _IOWR(DISPOBJ_IOC_COMMON_TYPE, 4, void*)
#define DISPOBJ_IOC_READ_REG_LIST           _IOWR(DISPOBJ_IOC_COMMON_TYPE, 5, void*)
#define DISPOBJ_IOC_WRITE_REG_LIST          _IOWR(DISPOBJ_IOC_COMMON_TYPE, 6, void*)
#define DISPOBJ_IOC_WRITE_CFG               _IOWR(DISPOBJ_IOC_COMMON_TYPE, 7, char *)
#define DISPOBJ_IOC_READ_CFG                _IOWR(DISPOBJ_IOC_COMMON_TYPE, 8, char *)
#define DISPOBJ_1_IOC_WRITE_CFG             _IOWR(DISPOBJ_IOC_COMMON_TYPE, 9, char *)
#define DISPOBJ_1_IOC_READ_CFG              _IOWR(DISPOBJ_IOC_COMMON_TYPE, 10, char *)



/* Add other command ID here*/


#endif
