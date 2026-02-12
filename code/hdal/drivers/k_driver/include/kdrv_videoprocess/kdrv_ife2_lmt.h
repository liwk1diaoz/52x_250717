
/**
 * @file kdrv_ife2_lmt.h
 * @brief parameter limitation of KDRV IFE2
 * @author ISP
 * @date in the year 2019
 */

#ifndef _KDRV_IFE2_LIMIT_H_
#define _KDRV_IFE2_LIMIT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

#define IFE2_SRCBUF_WMIN         40
#define IFE2_SRCBUF_HMIN         30

//=====================================================
// input / output limitation for Dram to Dram mode
#define IFE2_SRCBUF_D2D_WMIN         IFE2_SRCBUF_WMIN
#define IFE2_SRCBUF_D2D_WMAX         672
#define IFE2_SRCBUF_D2D_WALIGN       1

#define IFE2_SRCBUF_D2D_HMIN         IFE2_SRCBUF_HMIN
#define IFE2_SRCBUF_D2D_HMAX         16383
#define IFE2_SRCBUF_D2D_HALIGN       1

#define IFE2_SRCBUF_D2D_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IFE2_SRCBUF_D2D_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


//=====================================================
// input / output limitation for IPP mode
#define IFE2_SRCBUF_IPP_WMIN         IFE2_SRCBUF_WMIN
#define IFE2_SRCBUF_IPP_WMAX         660
#define IFE2_SRCBUF_IPP_WALIGN       1

#define IFE2_SRCBUF_IPP_HMIN         IFE2_SRCBUF_HMIN
#define IFE2_SRCBUF_IPP_HMAX         16383
#define IFE2_SRCBUF_IPP_HALIGN       1

#define IFE2_SRCBUF_IPP_LOFF_ALIGN    DRV_LIMIT_ALIGN_WORD
#define IFE2_SRCBUF_IPP_ADDR_ALIGN    DRV_LIMIT_ALIGN_BYTE


#endif
