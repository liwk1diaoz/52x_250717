#ifndef __MODULE_IOCTL_CMD_H_
#define __MODULE_IOCTL_CMD_H_

#include "kwrap/type.h"
#include "kwrap/ioctl.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define GYRO_COMM_OPEN nvt_gyro_comm_openf
#define GYRO_COMM_IOCTL nvt_gyro_comm_ioctl
#define GYRO_COMM_CLOSE nvt_gyro_comm_closef
#endif
#if defined(__LINUX)
#define GYRO_COMM_OPEN  open
#define GYRO_COMM_IOCTL ioctl
#define GYRO_COMM_CLOSE close
#endif

#define GYRO_IOC_VERSION 0x220102510


typedef struct {
	INT32 gyro_x;
	INT32 gyro_y;
	INT32 gyro_z;
	INT32 ags_x;
	INT32 ags_y;
	INT32 ags_z;
} GYRO_COMM_DATA;

typedef enum
{
    GYRO_COMM_MODE_SINGLE_ACCESS,       ///< gyro in single access mode, read/write register one by one
    GYRO_COMM_MODE_FREE_RUN,            ///< gyro in free run mode, read register after periodic trigger automatically
    ENUM_DUMMY4WORD(GYRO_COMM_MODE)
} GYRO_COMM_MODE;

#if defined(__FREERTOS)

int nvt_gyro_comm_openf(char* file, int flag);
int nvt_gyro_comm_closef(int fd);
int nvt_gyro_comm_ioctl(int fd, unsigned int cmd, void *arg);
#endif
//============================================================================
// IOCTL command
//============================================================================
#define GYRO_IOC_COMMON_TYPE 'G'
#define GYRO_IOC_OPEN    _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 1, void*)

#define GYRO_IOC_CLOSE     _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 2, void*)
#define GYRO_IOC_MODE      _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 3, void*)
#define GYRO_IOC_FREERUN      _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 4, void*)

#define GYRO_IOC_GET_SDATA     _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 5, void*)
#define GYRO_IOC_SET_OFS        _VOS_IOWR(GYRO_IOC_COMMON_TYPE, 6, void*)

/* Add other command ID here*/


#endif
