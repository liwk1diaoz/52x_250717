/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_lca_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_LCA_REG_
#define _IME_LCA_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


//-------------------------------------------------------------------------------
// local chroma adaption
extern VOID ime_set_lca_enable_reg(UINT32 set_en);
extern VOID ime_set_lca_chroma_adj_enable_reg(UINT32 set_en);
extern VOID ime_set_lca_la_enable_reg(UINT32 set_en);
extern UINT32 ime_get_lca_enable_reg(VOID);
extern VOID ime_set_lca_bypass_enable_reg(UINT32 set_en);

extern VOID ime_set_lca_enable_ppb_reg(UINT32 set_en);

extern VOID ime_set_lca_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_get_lca_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);

extern VOID ime_set_lca_scale_factor_reg(UINT32 factor_h, UINT32 factor_v);

extern VOID ime_set_lca_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_c);
extern VOID ime_get_lca_lineoffset_reg(UINT32 *p_get_lofs_y, UINT32 *p_get_lofs_c);

extern VOID ime_set_lca_dma_addr0_reg(UINT32 addr_y, UINT32 addr_c);
extern VOID ime_get_lca_dma_addr0_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c);

extern VOID ime_set_lca_dma_addr1_reg(UINT32 addr_y, UINT32 addr_c);
extern VOID ime_get_lca_dma_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c);

//extern UINT32 ime_get_lca_active_ppb_id_reg(VOID);

extern VOID ime_set_lca_format_reg(UINT32 set_fmt);
extern UINT32 ime_get_lca_format_reg(VOID);

extern VOID ime_set_lca_source_reg(UINT32 set_src);

extern VOID ime_set_lca_ca_adjust_center_reg(UINT32 u, UINT32 v);
extern VOID ime_set_lca_ca_range_reg(UINT32 rng, UINT32 wet_prc);
extern VOID ime_set_lca_ca_weight_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);

extern VOID ime_set_lca_chroma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 ref_c_wt, UINT32 out_wt);
extern VOID ime_set_lca_luma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 out_wt);

extern VOID ime_set_lca_chroma_range_y_reg(UINT32 rng, UINT32 wet_prc);
extern VOID ime_set_lca_chroma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);

extern VOID ime_set_lca_chroma_range_uv_reg(UINT32 rng, UINT32 wt_prc);
extern VOID ime_set_lca_chroma_weight_uv_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);

extern VOID ime_set_lca_luma_range_y_reg(UINT32 rng, UINT32 wt_prc);
extern VOID ime_set_lca_luma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);

//-------------------------------------------------------------------------------
// LCA-Subout
extern VOID ime_set_lca_subout_enable_reg(UINT32 set_en);
extern UINT32 ime_get_lca_subout_enable_reg(VOID);

extern VOID ime_set_lca_subout_source_reg(UINT32 src);


extern VOID ime_set_lca_subout_scale_factor_h_reg(UINT32 scl_rate, UINT32 scl_factor);
extern VOID ime_set_lca_subout_scale_factor_v_reg(UINT32 scl_rate, UINT32 scl_factor);

extern VOID ime_set_lca_subout_isd_scale_factor_base_reg(UINT32 scl_base_factor_h, UINT32 scl_base_factor_v);
extern VOID ime_set_lca_subout_isd_scale_factor0_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_set_lca_subout_isd_scale_factor1_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_set_lca_subout_isd_scale_factor2_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_set_lca_subout_isd_scale_coef_number_reg(UINT32 coef_num_h, UINT32 coef_num_v);

extern VOID ime_set_lca_subout_lineoffset_reg(UINT32 lofs);
extern UINT32 ime_get_lca_subout_lineoffset_reg(VOID);
extern VOID ime_set_lca_subout_dma_addr_reg(UINT32 addr);
extern UINT32 ime_get_lca_subout_dma_addr_reg(VOID);



#ifdef __cplusplus
}
#endif


#endif // _IME_LCA_REG_
