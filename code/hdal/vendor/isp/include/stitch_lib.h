#ifndef _STITCH_LIB_
#define _STITCH_LIB_

#include "hd_type.h"

//=============================================================================
// define
//=============================================================================
#define ECS_MAX_MAP_SZ     65
#define ECS_TABLE_LEN ECS_MAX_MAP_SZ*ECS_MAX_MAP_SZ

#define FALSE    0
#define TRUE     1

#define MAX_PATH_LENGTH    64

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _CHANNEL_INDEX {
	_R = 0,
	_GR,
	_GB,
	_B,
	ENUM_DUMMY4WORD(CHANNEL_INDEX)
} CHANNEL_INDEX;

typedef enum {
	AVERAGE_1X1_PIXEL,
	AVERAGE_3X3_PIXEL,
	AVERAGE_5X5_PIXEL,
	AVERAGE_7X7_PIXEL,
	AVERAGE_9X9_PIXEL,
} AVERAGE_PIXEL;

typedef struct {
	UINT32 low_bnd;
	UINT32 high_bnd;
} CALI_BOUND;
/*
typedef struct _VIG_CA{
	CHAR *src_buffer;                 ///< Input, must be unpack 12bit
	CHAR *out_buffer;                 ///< Output, result of vig simulation
	UINT32 width;                     ///< Input, image width
	UINT32 height;                    ///< Input, image height
	UINT32 start_pixel;               ///< Input, using CHANNEL_INDEX
	UINT32 vig_x;                     ///< Input, assign 0 for auto search center
	UINT32 vig_y;                     ///< Input, assign 0 for auto search center
	INT32 vig_weight;                 ///< Input, ratio of calibration (range: 0~100)
	INT32 vig_tap;                    ///< Output, vig number
	UINT32 vig_center_x;              ///< Output, vig center x
	UINT32 vig_center_y;              ///< Output, vig center y
	UINT32 vig_lut[17];               ///< Output, vig lut
	float vig_gain[17];               ///< Output, vig gain(float)
	UINT32 *dist_array;               ///< Output, dist
} VIG_CA;
*/
typedef struct _ECS_CA {
	CHAR *src_buffer0;                ///< Input0, must be unpack 12bit
	CHAR *src_buffer1;                ///< Input1, must be unpack 12bit
	CHAR *out_buffer0;                ///< Output0, result of ecs simulation
	CHAR *out_buffer1;                ///< Output1, result of ecs simulation
	UINT32 width;                     ///< Input, image width
	UINT32 height;                    ///< Input, image height
	UINT32 start_pix;                 ///< Input, using CHANNEL_INDEX
	UINT32 ecs_tbl[2][ECS_TABLE_LEN];
	AVERAGE_PIXEL avg_mode;
	CALI_BOUND target_lum;
} ECS_CA;

typedef struct {
	UINT32 top_blk;       // index: 0~64
	UINT32 bottom_blk;    // index: 0~64
	UINT32 left_blk;      // index: 0~64
	UINT32 right_blk;     // index: 0~64
} STITCH_OVERLAP_REGION;

typedef struct {
	UINT32 center_luma_ratio[ECS_MAX_MAP_SZ/2+1];    // unit: 0.1%
	UINT32 outer_luma_ratio[ECS_MAX_MAP_SZ/2+1];     // unit: 0.1%
} SHADING_COMPENSATE_RATIO;

typedef struct {
	STITCH_OVERLAP_REGION overlap_region;
	SHADING_COMPENSATE_RATIO comp;
} STITCH_PARAM;

typedef struct _AUTO_LUMA_RATIO {
	UINT32 frame_num;                           ///< Input, frame number
	UINT32 image_full_size_w;                   ///< Input, image full size width
	UINT32 image_full_size_h;                   ///< Input, image full size height
	UINT32 lut2d_scl_out_w[8];                  ///< Input, 2dlut output image width
	UINT32 lut2d_scl_out_h[8];                  ///< Input, 2dlut output image height
	UINT32 blend_width[8];						///< Input, over lap width
	UINT32 blend_height[8];						///< Input, over lap height
	UINT32 vpe_2dlut_size;						///< Input, 2dlut size
	UINT32 *lut2d_tbl_addr[8];  				///< Input, 2dlut
	UINT16 la[8][1024];							///< Input, la data
	UINT16 ratio_ub;							///< Input, ratio upper bound
	UINT32 luma_ratio[8];                   	///< Output, luma ratio
} AUTO_LUMA_RATIO;

//=============================================================================
// extern functions
//=============================================================================
extern HD_RESULT stitch_ecs_calibrate(ECS_CA *ecs_ca, STITCH_PARAM *stitch_param);
extern HD_RESULT stitch_ecs_simulate(ECS_CA *ecs_ca);
//extern HD_RESULT stitch_vig_calibrate(VIG_CA *vig_ca);
//extern HD_RESULT stitch_vig_simulate(VIG_CA *vig_ca);
extern HD_RESULT stitch_auto_luma_ratio_proces(AUTO_LUMA_RATIO *auto_ruma_ratio);
extern void stitch_set_dbg_out(int on);

#endif

