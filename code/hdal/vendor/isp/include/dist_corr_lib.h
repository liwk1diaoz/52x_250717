#ifndef _DIST_CORR_LIB_H_
#define _DIST_CORR_LIB_H_

#ifdef __KERNEL__
#include    "kwrap/type.h"
#else
// NOTE: XXXXX
#include <kwrap/nvt_type.h>
//#include "Type.h"
//#include "ErrorNo.h"
#endif

void get_gdc_distorted_coord_of_input_image(
	UINT32 *gdc_lut,
	INT32 img_size_h,
	INT32 img_size_v,
	INT32 cent_x,
	INT32 cent_y,
	INT32 fovgain,
	INT32 coord_num,
	INT32 (*query_coord)[2],
	INT32 (*distorted_coord)[2]
);

#endif
