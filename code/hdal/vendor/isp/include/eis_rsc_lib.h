#ifndef _EIS_RSC_LIB_H_
#define _EIS_RSC_LIB_H_

#include "kwrap/type.h"

#define PATH_NUM 2
#define GYROSCOPE_XYZ 3
#define ACCELEROMETER_XYZ 3
#define DISTOR_CURVE_TABLE_NUM 257
#define UNDISTOR_CURVE_TABLE_NUM 65
#define QUAT_COEFF_NUM 4

// camera intrinsic parameters
typedef struct _CAMERA_INTRINSICS {
	double distor_center[2]; //Ex. distor_center[0] = 960.0, distor_center[1] = 540.0
	double distor_curve[DISTOR_CURVE_TABLE_NUM]; //provided by tool, for warpping image query points to apply distortion
	double undistor_curve[UNDISTOR_CURVE_TABLE_NUM]; //provided by tool, for warpping image query points to apply undistortion
	INT32 focal_length; //pixel, Ex. 1000
	ISIZE calib_img_size; //pixel, Ex. calib_img_size.w = 1920, calib_img_size.h = 1080
	double max_out_radius;
} CAMERA_INTRINSICS;

typedef enum _CAMERA_AXES {
	CAM_X = 0,
	CAM_Y = 1,
	CAM_Z = 2,
} CAMERA_AXIS;

typedef enum _SYNC_METHOD {
    VSYNC = 0,
    TIMESTAMP = 1,
} SYNC_METHOD;

typedef struct _CAMERA_AXIS_MAPPING {
	CAMERA_AXIS axis; //assign corresponding camera axis for each gyro axis
	INT32 sign; //assign axis direction for gyro axis to its corresponding camera axis, Ex. gyro X axis map to camera negative Z axis
} CAMERA_AXIS_MAPPING;

typedef enum _IMU_TYPE_SEL {
	GYROSCOPE = 0, //3-axes angular velocity
	GYROSCOPE_ACCELEROMETER = 1, //3-axes angular velocity and 3-axed acceleration
} IMU_TYPE;

typedef struct _GYROSCOPE_CONFIG {
    SYNC_METHOD sync_method;
	CAMERA_AXIS_MAPPING axes_mapping[GYROSCOPE_XYZ]; //Ex. axes_mapping[0].axis = CAM_Z, axes_mapping[0].sign = -1;
	INT32 sampling_rate; //Ex. 1000 Hz
	double unit_conv;    //Ex. gyro_value/unit_conv = rad/s
} GYROSCOPE_CONFIG;

typedef struct _ACCELEROMETER_CONFIG {
	CAMERA_AXIS_MAPPING axes_mapping[ACCELEROMETER_XYZ]; //Ex. axes_mapping[0].axis = CAM_Z, axes_mapping[0].sign = -1;
	INT32 sampling_rate; //Ex. 1000 Hz
	double unit_conv;    //not sure for now
	double accel_gain;
} ACCELEROMETER_CONFIG;

typedef enum _STABLE_PROFILE {
	SPORT_CAM = 0,
	IP_CAM = 1,
} STABLE_PROFILE;

typedef struct _EIS_RSC_OPEN_CFG {
	CAMERA_INTRINSICS cam_intrins; //camera intrinsic parameters acquired by camera calibration
	// IMU parameters
	IMU_TYPE imu_type; //Ex. imu_type = GYROSCOPE;
	GYROSCOPE_CONFIG gyro; //gyroscope configurations
	ACCELEROMETER_CONFIG accel; //accelerometer configurations
	// EIS effect parameters
	INT32 imu_sync_shift_exposure_time_threshold;
	INT32 imu_sync_shift_exposure_time_precent;
    INT32 imu_timing_shift_adjust;
	STABLE_PROFILE stable_profile;
} EIS_RSC_OPEN_CFG;

typedef enum _LUT_SIZE_SEL {
	LUT_9x9 = 0,
	LUT_65x65 = 3,
} LUT_SIZE_SEL;

typedef struct _EIS_RSC_PATH_INFO {
	INT32 path_id;
	INT32 frame_latency;
	ISIZE frame_size;
	LUT_SIZE_SEL lut2d_size_sel;
    ISIZE eis_crop_size;
} EIS_RSC_PATH_INFO;

typedef struct _EIS_RSC_BUF {
	UINT32 valid;
	UINT32 *lut2d_buf;
	UINT32 buf_size;
	UINT32 frame_first_line_timestamp;
} EIS_RSC_BUF;

typedef struct _EIS_RSC_PROC_INFO {
	INT32 path_id;
	UINT64 frame_count;
	INT32 frame_exposure_time; //us
	INT32 *angular_rate[GYROSCOPE_XYZ]; //3-axes angular velocity
	INT32 *acceleration_rate[ACCELEROMETER_XYZ]; //3-axes acceleration
	EIS_RSC_BUF lut2d;
} EIS_RSC_PROC_INFO;

typedef enum _IMAGE_ROTATION_SEL {
	//counterclockwise
	ROT_0 = 0,
	ROT_90 = 1, //= -270 clockwise
	ROT_180 = 2, //= -180 clockwise
	ROT_270 = 3, //= -90 clockwise
} IMG_ROTATE_SEL;

typedef struct _EIS_RSC_IMU_TIMESTAMP_INFO {
	INT32 imu_num_per_frm;
	UINT32 *timestamp;
	UINT32 time_precision;
	INT32 frm_imu_latency;
} EIS_RSC_IMU_TIMESTAMP_INFO;

typedef struct _EIS_RSC_FRAME_LINE_TIMESTAMP_INFO {
	UINT32 first_line_timestamp;
    UINT32 last_line_timestamp;
	UINT32 time_precision;
} EIS_RSC_FRAME_LINE_TIMESTAMP_INFO;

typedef enum _EIS_RSC_PARAM_ITEM {
	EIS_RSC_PARAM_PATH_INFO,        				///< set eis path, using EIS_RSC_PATH_INFO struct
	EIS_RSC_PARAM_FRAME_CNT_RESET,  				///< frame count overflow reset
	EIS_RSC_PARAM_FRAME_RATE,       				///< change frame rate for AE, using INT32
	EIS_RSC_PARAM_IMU_SAMPLE_NUM_PER_FRAME, 		///< change IMU sample number of each frame, using INT32
	EIS_RSC_PARAM_IMU_TIMESTAMP, 					///< set IMU timestamp information of each frame, using EIS_RSC_IMU_TIMESTAMP_INFO, available only when SYNC_METHOD = TIMPSTAMP
	EIS_RSC_PARAM_FRAME_LINE_TIMESTAMP, 			///< set timestamp information of start and end line, using EIS_RSC_FRAME_LINE_TIMESTAMP_INFO, available only when SYNC_METHOD = TIMPSTAMP
	EIS_RSC_PARAM_ROTATE_TYPE,      				///< set output image rotation type, using IMG_ROTATE_SEL enum
	//EIS_RSC_PARAM_COMPEN_CROP_PERCENT,      		///< compensation cropping percentage, user setting
	EIS_RSC_PARAM_UNDISTORT_OUTPUT,      			///< undistort output, user setting
	//EIS_RSC_PARAM_SIE_CROP_START_END_TIME,			///< set SIE crop window time
	//EIS_RSC_PARAM_SIE_CROP_END_TO_VD_TIME,			///< set SIE crop end to vd time
	EIS_RSC_PARAM_CHANGE_CAMERA_PARAM,				///< change camera parameter to support optical zoom lens

	//debug
	EIS_RSC_PARAM_SHOW_LIB_PARAM,					///< Enable printing library parameters
	EIS_RSC_PARAM_SHOW_FRAME_TIMESTAMP,				///< Enable printing frame timestamp
	EIS_RSC_PARAM_SHOW_IMU_DATA,          			///< Enable printing gyro data
	EIS_RSC_PARAM_SHOW_CAM_BODY_RATE,   			///< Enable printing camera body rate(remapped gyro data)
	EIS_RSC_PARAM_SHOW_ACTUAL_FRM_CENTER_QUAT,   	///< Enable printing actual frame center quaternion
	EIS_RSC_PARAM_SHOW_ACTUAL_LINE_QUAT,   			///< Enable printing actual line quaternion
	EIS_RSC_PARAM_SHOW_STABLE_FRM_CENTER_QUAT,   	///< Enable printing stable frame center quaternion
	EIS_RSC_PARAM_SHOW_LINE_COMPEN_QUAT,            ///< Enable printing line compensation quaternion
	EIS_RSC_PARAM_SHOW_WARPING_TABLE,				///< Enable printing warping table
	EIS_RSC_PARAM_SHOW_2DLUT_CHECKSUM,				///< Enable printing 2dlut checksum
	EIS_RSC_PARAM_SHOW_EXPOSURE_TIME,				///< Enable printing exposure time
	EIS_RSC_PARAM_COMPEN_VERIF_ENABLE,      		///< Enable compensation verification switch
	EIS_RSC_PARAM_DISABLE_ROLL_SHUT_CORRECT,   		///< Disable rolling shutter correction
	EIS_RSC_PARAM_DISABLE_STABILIZATION,  			///< Disable stabilization
	EIS_RSC_PARAM_DISABLE_WARPING_TABLE_CALCULATION,///< Disable warping table calculation
	EIS_RSC_PARAM_DISABLE_CAM_MODEL_UNDISTORT,      ///< Disable camera model undistortion
	EIS_RSC_PARAM_ADJUST_GYRO_TIME_SHIFT,   		///< Adjust gyro time shift with image sensor
	EIS_RSC_PARAM_ADJUST_ROLL_SHUT_READOUT_TIME, 	///< Adjust rolling shutter readout time
	EIS_RSC_PARAM_ADJUST_FOCAL_LENGTH, 				///< Adjust focal length
	EIS_RSC_PARAM_ADJUST_MAX_OUT_RADIUS,			///< Adjust max out radius
	EIS_RSC_PARAM_ADJUST_DISTORT_CENTER_X,			///< Adjust distort center X
	EIS_RSC_PARAM_ADJUST_DISTORT_CENTER_Y,			///< Adjust distort center Y
	EIS_RSC_PARAM_ADJUST_IMU_SYNC_SHIFT_EXPO_TIME_THRES,	///< Adjust IMU sync shift exposure time thres
	EIS_RSC_PARAM_ADJUST_IMU_SYNC_SHIFT_EXPO_TIME_PRECENT,	///< Adjust IMU sync shift exposure time ratio
	EIS_RSC_PARAM_ADJUST_ACCELEROMETER_GAIN,	    ///< Adjust accelerometer gain
	EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_1,			///< Set stable trajectory coeff 1
	EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_2,			///< Set stable trajectory coeff 2
	EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_3,			///< Set stable trajectory coeff 3
	EIS_RSC_PARAM_FIX_FRM_STABLE_QUAT,              ///< Fix stable frame center quaternion
    //get
    EIS_RSC_PARAM_VERSION_INFO,                           ///< get version number
	ENUM_DUMMY4WORD(EIS_RSC_PARAM_ID)
} EIS_RSC_PARAM_ID;

BOOL eis_rsc_open(EIS_RSC_OPEN_CFG *p_open_cfg);
BOOL eis_rsc_close(void);
BOOL eis_rsc_process(EIS_RSC_PROC_INFO *p_proc_info);
BOOL eis_rsc_set(EIS_RSC_PARAM_ID param_id, VOID *p_param);
BOOL eis_rsc_get(EIS_RSC_PARAM_ID param_id, VOID *p_param);

INT32 eis_rsc_get_2dlut_size(LUT_SIZE_SEL sz_sel);
UINT32 eis_rsc_get_2dlut_buffer_size(LUT_SIZE_SEL sz_sel);

VOID eis_rsc_show_configuration(VOID);

//BOOL generate_fusion_weight_map(UINT8 *p_wt_map, ISIZE map_size, INT32 smooth_length);

#endif
