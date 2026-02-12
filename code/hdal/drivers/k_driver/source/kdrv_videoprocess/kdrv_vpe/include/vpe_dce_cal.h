#ifndef _VPE_DRV_DCE_INT_H_
#define _VPE_DRV_DCE_INT_H_
//#include "vpe_drv_ll_int.h"
//#include "vpe_drv_util_int.h"
//#include "vpe_drv_ctl_int.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

//#include "vpe_dce_base.h"

/*
#if (VPE_DRV_GEO_LUT_NUMS != KDRV_VPE_GEO_LUT_NUMS)
#error "GEO LUT size misatch"
#endif
*/
#if defined(__LINUX)
#define COHERENT_CACHE_BUF 1
#else
#define COHERENT_CACHE_BUF 1
#endif

#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define VPE_DRV_ALIGN_ROUNDDOWN ALIGN_FLOOR
#define VPE_DRV_ALIGN_ROUNDUP ALIGN_CEIL
#define VPE_DRV_ALIGN_ROUND ALIGN_ROUND


#define DCE_2DLUT_ROT_X_MAX 9
#define DCE_2DLUT_ROT_Y_MAX 9
#define DCE_2DLUT_TAB_IDX 0;//VPE_DCE_2DLUT_9_9_GRID
#define DCE_2DLUT_ROT_W_ALIGN VPE_DRV_ALIGN_ROUNDUP(DCE_2DLUT_ROT_X_MAX, 4)
#define DCE_2DLUT_TAB_TOTAL_NUM (DCE_2DLUT_ROT_W_ALIGN * DCE_2DLUT_ROT_Y_MAX)
#define DCE_2DLUT_BUF_SIZE (DCE_2DLUT_TAB_TOTAL_NUM << 2) 				    	//real buffer size
#if (COHERENT_CACHE_BUF == 1)
#define DCE_2DLUT_ALLOC_SIZE VPE_DRV_ALIGN_ROUNDUP(DCE_2DLUT_BUF_SIZE, 64)  	//for  buffer allocation used to meet address align 64
#else
#define DCE_2DLUT_ALLOC_SIZE VPE_DRV_ALIGN_ROUNDUP(DCE_2DLUT_BUF_SIZE, 16)  	//for  buffer allocation used to meet address align 16
#endif

extern int vpe_drv_dce_gen_rot_2dlut_fix(unsigned long *buf_va, unsigned int buf_size, struct KDRV_VPE_ROI roi, enum KDRV_VPE_DCE_ROT rot);
extern int vpe_drv_dce_gen_rot_2dlut_deg(unsigned long *buf_va, unsigned int buf_size, struct KDRV_VPE_ROI roi, struct KDRV_VPE_DCE_ROT_PARAM rot_param);

//int vpe_drv_dce_proc(VPE_ENG_HANDLE *p_eng_hdl, struct vpe_drv_job_cfg *job_cfg, struct vpe_drv_lut2d_info *lut2d);
//void vpe_drv_dce_dump_info(struct vpe_drv_dce_param *dce);
#endif //_VPE_DRV_TMNR_INT_H_
