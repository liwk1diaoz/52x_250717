#ifndef _EIS_IOCTL_
#define _EIS_IOCTL_

#include "kwrap/ioctl.h"

//=============================================================================
// EIS IOCTL command definition
//=============================================================================
#define EIS_IOC_MAGIC 'E'

#define GYROSCOPE_XYZ 3
#define ACCELEROMETER_XYZ 3

#define MAX_LATENCY_NUM   5

/*
    error code define
*/
#define EIS_E_OK           (0)  // normal completion
#define EIS_E_TIMEOUT      (-1) // wait time out
#define EIS_E_ABORT        (-2) // wait abort
#define EIS_E_FAIL         (-3) // failed
#define EIS_E_STATE        (-4) // state machine error
#define EIS_E_DEBUG        (-5) // debug only
#define EIS_E_GYRO_LATENCY (-6) // enable gyro latency


#define MAX_GYRO_DATA_NUM 40
#define MAX_EIS_USER_DATA_SIZE 640


#define EIS_DEBUG_INFO 1

#if EIS_DEBUG_INFO
typedef struct
{
	UINT64 frame_count;
	UINT64 t_diff_crop;
	UINT64 t_diff_crp_end_to_vd;
	UINT32 exposure_time;
	UINT32 gyro_data_num;
	UINT32 vendor_eis_ver;
	UINT32 eis_rsc_ver;
} EIS_DBG_CTX;
#define EIS_DEBUG_SIZE ALIGN_CEIL_4(MAX_GYRO_DATA_NUM*6*sizeof(UINT32) + sizeof(EIS_DBG_CTX))//for gyro data
#else
#define EIS_DEBUG_SIZE 0
#endif

#define MAX_EIS_LUT2D_SIZE ((68*65)+(EIS_DEBUG_SIZE/sizeof(UINT32)))


typedef enum _EIS_STATE {
	EIS_STATE_CLOSE = 0,
	EIS_STATE_OPEN,
	EIS_STATE_IDLE, //abort wait_proc
	ENUM_DUMMY4WORD(EIS_STATE)
} EIS_STATE;

//should refer to VDOPRC_EIS_PROC_INFO
typedef struct _EIS_WAIT_PROC_INFO
{
	UINT64 frame_count;
	UINT64 t_diff_crop;
	UINT64 t_diff_crp_end_to_vd;
	UINT32 angular_rate_x[MAX_GYRO_DATA_NUM];
	UINT32 angular_rate_y[MAX_GYRO_DATA_NUM];
	UINT32 angular_rate_z[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_x[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_y[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_z[MAX_GYRO_DATA_NUM];
	UINT32 gyro_timestamp[MAX_GYRO_DATA_NUM];
	UINT32 data_num;
	UINT32 user_data_size;
	UINT8  user_data[MAX_EIS_USER_DATA_SIZE];
	INT32 wait_ms;
	INT32 result;
} EIS_WAIT_PROC_INFO;

typedef struct _EIS_PUSH_DATA
{
	UINT64 frame_count;
	UINT32 path_id;
	UINT32 *lut2d_buf;
	UINT32 buf_size;
	UINT32 frame_time;
} EIS_PUSH_DATA;

typedef struct _EIS_PATH_INFO {
	UINT32 path_id;
	UINT32 lut2d_buf_size;
	UINT32 lut2d_size_sel;
	UINT32 frame_latency;
} EIS_PATH_INFO;


#define EIS_IOC_SET_STATE        _VOS_IOWR(EIS_IOC_MAGIC, 0, UINT32)
#define EIS_IOC_WAIT_PROC        _VOS_IOWR(EIS_IOC_MAGIC, 1, EIS_WAIT_PROC_INFO)
#define EIS_IOC_PUSH_DATA        _VOS_IOWR(EIS_IOC_MAGIC, 2, EIS_PUSH_DATA)
#define EIS_IOC_PATH_INFO        _VOS_IOWR(EIS_IOC_MAGIC, 3, EIS_PATH_INFO)
#define EIS_IOC_DEBUG_SIZE       _VOS_IOWR(EIS_IOC_MAGIC, 4, UINT32)





#if defined(__FREERTOS)
extern int eis_open(char* file, int flag);
extern int eis_ioctl(int fd, unsigned int cmd, void *arg);
extern int eis_close(int fd);
#endif
extern ER eis_init_module(void);
extern ER eis_uninit_module(void);
#endif

