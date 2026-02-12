/**
 * @file kdrv_videoenc_lmt.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2019
 */
#ifndef __KDRV_VIDEOENC_LMT_H__
#define __KDRV_VIDEOENC_LMT_H__

#include "comm/drv_lmt.h"

#define H264E_WIDTH_MIN			(320)	//min encode width	(after rotation)
#define H264E_HEIGHT_MIN		(240)	//min encode height	(after rotation)
#define H264E_WIDTH_MAX			(2688)
#define H264E_HEIGHT_MAX		(2688)
#define H264E_WIDTH_ALIGN		(DRV_LIMIT_ALIGN_4WORD)
#define H264E_HEIGHT_ALIGN		(DRV_LIMIT_ALIGN_2WORD)

#define H265E_WIDTH_MIN			(320)	//min encode width	(after rotation)
#define H265E_HEIGHT_MIN		(240)	//min encode height	(after rotation)
#define H265E_WIDTH_MAX			(5120)
#define H265E_HEIGHT_MAX		(2688)
#define H265E_WIDTH_ALIGN		(DRV_LIMIT_ALIGN_4WORD)
#define H265E_HEIGHT_ALIGN		(DRV_LIMIT_ALIGN_2WORD)
#define H265E_TILE_MAX			(5)

#define H265E_MAX_WIDTH_WITHOUT_TILE	(2048)

#define H264E_WIDTH_MIN_528		(240)	//min encode width
#define H264E_HEIGHT_MIN_528		(240)	//min encode height
#define H264E_WIDTH_MAX_528		(5120)
#define H264E_HEIGHT_MAX_528		(5120)

#define H265E_WIDTH_MIN_528		(240)	//min encode width
#define H265E_HEIGHT_MIN_528		(240)	//min encode height
#define H265E_WIDTH_MAX_528		(5120)
#define H265E_HEIGHT_MAX_528		(5120)
#define H265E_TILE_MAX_528		(4)
#define H265E_D2D_MAX_W_528			(4096)

#define H26XE_BUF_ADDR_ALIGN	(DRV_LIMIT_ALIGN_WORD)
#define H26XE_BS_BUF_ADDR_ALIGN	(DRV_LIMIT_ALIGN_WORD)

#define H26XE_ROTATE_90			(1)
#define H26XE_ROTATE_180		(1)
#define H26XE_ROTATE_270		(1)


#define H264E_ROTATE_MAX_HEIGHT		(1984)
#define H264E_ROTATE_MAX_HEIGHT_528	(2176)

#define H26XE_QP_MAP_UNIT		(16)

#define H26XE_ROI_WIN_MAX_NUM	(10)

#define H26XE_OSG_WIN_MAX_NUM	(32)
#define H26XE_OSG_PAL_MAX_NUM	(16)
#define H26XE_OSG_WIDTH_ALIGN	(2)		//should be multiple of 2
#define H26XE_OSG_HEIGHT_ALIGN	(2)		//should be multiple of 2
#define H26XE_OSG_POSITION_X_ALIGN	(2)		//should be multiple of 2
#define H26XE_OSG_POSITION_Y_ALIGN	(2)		//should be multiple of 2
#define H26XE_OSG_LOFS_ALIGN	(DRV_LIMIT_ALIGN_WORD)
#define H26XE_OSG_ADR_ALIGN	(DRV_LIMIT_ALIGN_2WORD)
#define H26XE_MOT_BUF_MAX_NUM	(3)

#define H26XE_SLICE_MAX_NUM		(176)

#endif	// __KDRV_VIDEOENC_LMT_H__
