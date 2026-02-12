/**
	@brief Header file of vendor eis module.\n
	This file contains the functions which is related to EIS in the chip.

	@file vendor_eis.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_EIS_H_
#define _VENDOR_EIS_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include <kwrap/type.h>
/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
#define PATH_NUM 2
#define GYROSCOPE_XYZ 3
#define ACCELEROMETER_XYZ 3
#define DISTOR_CURVE_TABLE_NUM 257
#define UNDISTOR_CURVE_TABLE_NUM 65
#define QUAT_COEFF_NUM 4

/**
    Callback function prototype for user data
*/
typedef VOID (*VENDOR_EIS_USER_DATA_CB)(INT32 path_id, ULONG user_data_addr, UINT32 size);

#define MAX_GYRO_DATA_NUM 40
typedef struct _VENDOR_EIS_GYRO_INFO
{
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
} VENDOR_EIS_GYRO_INFO;//based on VDOCAP_EIS_GYRO_INFO


typedef struct _VENDOR_EIS_DBG_CTX
{
	UINT64 frame_count;
	UINT64 t_diff_crop;
	UINT64 t_diff_crp_end_to_vd;
	UINT32 exposure_time;
	UINT32 gyro_data_num;
	UINT32 vendor_eis_ver;
	UINT32 eis_rsc_ver;
} VENDOR_EIS_DBG_CTX;

// camera intrinsic parameters
typedef struct _VENDOR_EIS_CAMERA_INTRINSICS {
	double distor_center[2]; //Ex. distor_center[0] = 960, distor_center[1] = 540
	double distor_curve[DISTOR_CURVE_TABLE_NUM]; //provided by tool, for warpping image query points to apply distortion
	double undistor_curve[UNDISTOR_CURVE_TABLE_NUM]; //provided by tool, for warpping image query points to apply undistortion
	INT32 focal_length; //pixel, Ex. 1000
	ISIZE calib_img_size; //pixel, Ex. calib_img_size.w = 1920, calib_img_size.h = 1080
	double max_out_radius;
} VENDOR_EIS_CAMERA_INTRINSICS;

typedef enum _VENDOR_EIS_CAMERA_AXIS {
	VENDOR_EIS_CAM_X = 0,
	VENDOR_EIS_CAM_Y = 1,
	VENDOR_EIS_CAM_Z = 2,
} VENDOR_EIS_CAMERA_AXIS;


typedef struct _VENDOR_EIS_CAMERA_AXIS_MAPPING {
	VENDOR_EIS_CAMERA_AXIS axis; //assign corresponding camera axis for each gyro axis
	INT32 sign;	//assign axis direction for gyro axis to its corresponding camera axis, Ex. gyro X axis map to camera negative Z axis
} VENDOR_EIS_CAMERA_AXIS_MAPPING;

typedef enum _VENDOR_EIS_IMU_TYPE_SEL {
	VENDOR_EIS_GYROSCOPE = 0, //3-axes angular velocity
	VENDOR_EIS_GYROSCOPE_ACCELEROMETER = 1, //3-axes angular velocity and 3-axed acceleration
} VENDOR_EIS_IMU_TYPE;

typedef struct _VENDOR_EIS_GYROSCOPE_CONFIG {
	VENDOR_EIS_CAMERA_AXIS_MAPPING axes_mapping[GYROSCOPE_XYZ]; //Ex. axes_mapping[0].axis = CAM_Z, axes_mapping[0].sign = -1;
	INT32 sampling_rate; //Ex. 1000 Hz
	double unit_conv;	//Ex. gyro_value/unit_conv = rad/s
} VENDOR_EIS_GYROSCOPE_CONFIG;

typedef struct _VENDOR_EIS_ACCELEROMETER_CONFIG {
	VENDOR_EIS_CAMERA_AXIS_MAPPING axes_mapping[ACCELEROMETER_XYZ]; //Ex. axes_mapping[0].axis = CAM_Z, axes_mapping[0].sign = -1;
	INT32 sampling_rate; //Ex. 1000 Hz
	double unit_conv;	//not sure for now
	double accel_gain;
} VENDOR_EIS_ACCELEROMETER_CONFIG;

typedef enum _VENDOR_EIS_STABLE_PROFILE {
	VENDOR_EIS_SPORT_CAM = 0,
	VENDOR_EIS_IP_CAM = 1,
} VENDOR_EIS_STABLE_PROFILE;

typedef struct _VENDOR_EIS_OPEN_CFG {
	VENDOR_EIS_CAMERA_INTRINSICS cam_intrins; //camera intrinsic parameters acquired by camera calibration
	// IMU parameters
	VENDOR_EIS_IMU_TYPE imu_type; //Ex. imu_type = GYROSCOPE;
	VENDOR_EIS_GYROSCOPE_CONFIG gyro; //gyroscope configurations
	VENDOR_EIS_ACCELEROMETER_CONFIG accel; //accelerometer configurations
	// EIS effect parameters
	INT32 imu_sync_shift_exposure_time_threshold;
	INT32 imu_sync_shift_exposure_time_precent;
	INT32 imu_timing_shift_adjust;
	VENDOR_EIS_STABLE_PROFILE stable_profile;
} VENDOR_EIS_OPEN_CFG;


typedef enum _VENDOR_EIS_LUT_SIZE_SEL
{
	VENDOR_EIS_LUT_9x9 = 0,
	VENDOR_EIS_LUT_65x65 = 3,
} VENDOR_EIS_LUT_SIZE_SEL;

typedef struct _VENDOR_EIS_PATH_INFO {
	INT32 path_id;
	INT32 frame_latency;
	ISIZE frame_size;
	VENDOR_EIS_LUT_SIZE_SEL lut2d_size_sel;
	ISIZE eis_crop_size;
} VENDOR_EIS_PATH_INFO;

typedef enum _VENDOR_EISIMAGE_ROTATION_SEL
{
	//counterclockwise
	VENDOR_EIS_ROT_0 = 0,
	VENDOR_EIS_ROT_90 = 1, //= -270 clockwise
	VENDOR_EIS_ROT_180 = 2, //= -180 clockwise
	VENDOR_EIS_ROT_270 = 3, //= -90 clockwise
} VENDOR_EIS_IMG_ROTATE_SEL;

typedef enum _VENDOR_EIS_EIS_PARAM_ITEM {
	VENDOR_EIS_PARAM_PATH_INFO,                   ///< set eis path, using VENDOR_EIS_PATH_INFO struct
	VENDOR_EIS_PARAM_FRAME_CNT_RESET,             ///< frame count overflow reset
	VENDOR_EIS_PARAM_FRAME_RATE,                  ///< change frame rate for AE, using INT32
	VENDOR_EIS_PARAM_IMU_SAMPLE_NUM_PER_FRAME,    ///< change IMU sample number of each frame, using INT32
	VENDOR_EIS_PARAM_ROTATE_TYPE,                 ///< set output image rotation type, using VENDOR_EIS_IMG_ROTATE_SEL enum
	VENDOR_EIS_PARAM_COMPEN_CROP_PERCENT,         ///< Obsolete item, do NOT use anymore
	VENDOR_EIS_PARAM_UNDISTORT_OUTPUT,            ///< undistort output, user setting
	VENDOR_EIS_PARAM_SIE_CROP_START_END_TIME,     ///< Obsolete item, do NOT use anymore
	VENDOR_EIS_PARAM_SIE_CROP_END_TO_VD_TIME,     ///< Obsolete item, do NOT use anymore
	VENDOR_EIS_PARAM_SHOW_LIB_PARAM,              ///< Enable printing library parameters
	VENDOR_EIS_PARAM_SHOW_FRAME_TIMESTAMP,        ///< Enable printing frame timestamp
	VENDOR_EIS_PARAM_SHOW_IMU_DATA,               ///< Enable printing gyro data
	VENDOR_EIS_PARAM_SHOW_CAM_BODY_RATE,          ///< Enable printing camera body rate(remapped gyro data)
	VENDOR_EIS_PARAM_SHOW_2DLUT_CHECKSUM,         ///< Enable printing 2dlut checksum
	VENDOR_EIS_PARAM_SHOW_EXPOSURE_TIME,          ///< Enable printing exposure time
	VENDOR_EIS_VERSION_DATE,                      ///< get version number
	VENDOR_EIS_PARAM_USER_DATA_CB,                ///< register callback for user data
	ENUM_DUMMY4WORD(VENDOR_EIS_PARAM_ID)
} VENDOR_EIS_PARAM_ID;
/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_eis_open(VENDOR_EIS_OPEN_CFG *p_open_cfg);
extern HD_RESULT vendor_eis_close(void);
extern BOOL vendor_eis_set(VENDOR_EIS_PARAM_ID param_id, VOID *p_param);
extern UINT32 vendor_eis_buf_query(VENDOR_EIS_LUT_SIZE_SEL lut2d_size_sel);
#endif


