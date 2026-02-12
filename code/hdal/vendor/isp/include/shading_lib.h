#ifndef _SHADING_LIB_
#define _SHADING_LIB_

#include "hd_type.h"

//=============================================================================
// define
//=============================================================================
#define SHADING_DBG_TYPE_NULL        0x00000000
#define SHADING_DBG_TYPE_ECS         0x00000001
#define SHADING_DBG_TYPE_VIG         0x00000002
#define SHADING_DBG_TYPE_ALL         0xFFFFFFFF

#define ECS_TABLE_LEN 65*65

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _CHANNEL_INDEX {
	_R = 0,
	_Gr,
	_Gb,
	_B,
	ENUM_DUMMY4WORD(CHANNEL_INDEX)
} CHANNEL_INDEX;

typedef struct _ECS_CA {
	CHAR *src_buffer;                 ///< Input, must be unpack 12bit
	CHAR *out_buffer;                 ///< Output, result of ecs simulation
	UINT32 width;                     ///< Input, image width
	UINT32 height;                    ///< Input, image height
	UINT32 start_pix;                 ///< Input, using CHANNEL_INDEX
	UINT32 shad_tbl[ECS_TABLE_LEN];   ///< Output, ecs table
} ECS_CA;

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
} VIG_CA;

//=============================================================================
// extern functions
//=============================================================================
extern HD_RESULT shading_set_dbg_out(UINT32 type);
extern HD_RESULT shading_ecs_calibrate(ECS_CA *ecs_ca);
extern HD_RESULT shading_ecs_simulate(ECS_CA *ecs_ca);
extern HD_RESULT shading_vig_calibrate(VIG_CA *vig_ca);

#endif

