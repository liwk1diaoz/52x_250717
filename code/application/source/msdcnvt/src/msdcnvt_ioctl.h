#ifndef __MSDCNVT_IOCTL_CMD_H
#define __MSDCNVT_IOCTL_CMD_H

#include <linux/ioctl.h>

//============================================================================
// IOCTL command
//============================================================================
#define MSDCNVT_IOC_COMMON_TYPE 'M'
#define MSDCNVT_IOC_CMD                     _IO(MSDCNVT_IOC_COMMON_TYPE, 1)

#endif
