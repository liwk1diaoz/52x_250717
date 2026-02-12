#ifndef _VPE_COLUMN_CAL_H_
#define _VPE_COLUMN_CAL_H_


#include "vpe_platform.h"
#include "vpe_config_base.h"


#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000




#define VPE_COL_MST_DBG   0

#define VPE_MAX_RES_NUM     4
#define VPE_MAX_COL_NUM     8

//========================================================

typedef enum {
	STPMODE_SST     = 0,  ///< single stripe mode
	STPMODE_MST     = 1,  ///< horizontal multiple stripe mode
} STP_MODE;


typedef struct _POS_INFO_ {
	uint32_t start_x;
	uint32_t end_x;
} POS_INFO;


typedef struct _VPE_RES_EN_PARAM_ {
	uint32_t res_en[VPE_MAX_RES_NUM];
	uint32_t res_out_size[VPE_MAX_RES_NUM];
} VPE_RES_EN_PARAM;


typedef struct _STRIPE_PARAM_ {
	uint32_t stp_hn;
	uint32_t stp_hl;
	uint32_t stp_hm;

	uint32_t stp_overlap;
} STRIPE_PARAM;

typedef struct _VPE_IMG_SCALE_PARAM_ {
	uint32_t sca_en;

	uint32_t in_size_h;
	uint32_t in_size_v;
	//uint32_t in_lofs;

	uint32_t out_size_h;
	uint32_t out_size_v;
	//uint32_t out_lofs;

	uint32_t stp_nums;
} VPE_IMG_SCALE_PARAM;



typedef struct _VPE_HW_OUT_COL_PARAM_ {
	VPE_RES_COL_SIZE_CFG  out_res0_col_info[VPE_MAX_RES_NUM][VPE_MAX_COL_NUM];
} VPE_HW_OUT_COL_PARAM;

typedef struct _VPE_IN_PATH_INFO_ { // view scene
	uint32_t src_width;
	uint32_t src_height;

	uint32_t dce_width;
	uint32_t dce_height;

	uint32_t proc_x_pos;
	uint32_t proc_y_pos;
	uint32_t proc_width;
	uint32_t proc_height;

	uint32_t dce_lens_radius;
} VPE_IN_PATH_INFO;

typedef struct _VPE_OUT_PATH_INFO_ {
	uint32_t sca_en;
	uint32_t sca_comps_en; // res0 and res1

	uint32_t sca_proc_crop_x_pos;  // target crop x position
	uint32_t sca_proc_crop_y_pos;  // target crop y position
	uint32_t sca_proc_crop_width;  // target crop width
	uint32_t sca_proc_crop_height;   // target crop height

	uint32_t sca_width;
	uint32_t sca_height;

	uint32_t sca_tag_crop_x_pos;   // crop x position after scaling
	uint32_t sca_tag_crop_y_pos;   // crop y position after scaling
	uint32_t sca_tag_crop_width;   // crop width after scaling
	uint32_t sca_tag_crop_height;  // crop height after scaling


	uint32_t des_width;
	uint32_t des_height;

	uint32_t out_x_pos;
	uint32_t out_y_pos;
	uint32_t out_width;
	uint32_t out_height;

	uint32_t rlt_x_pos;
	uint32_t rlt_y_pos;
	uint32_t rlt_width;
	uint32_t rlt_height;

	uint32_t pip_x_pos;
	uint32_t pip_y_pos;
	uint32_t pip_width;
	uint32_t pip_height;
} VPE_OUT_PATH_INFO;

typedef struct _VPE_SCALE_FACTORS_ {
	uint32_t sca_factor_h;
	uint32_t sca_factor_v;

	uint32_t sca_size_h;
	uint32_t sca_size_v;
	uint32_t sca_pos_x;
	uint32_t sca_pos_y;
} VPE_SCALE_FACTORS;

typedef struct _VPE_BGL_STRIPE_PARAM_ {
	VPE_IN_PATH_INFO    vpe_in_param;
	VPE_OUT_PATH_INFO   vpe_out_param[VPE_MAX_RES_NUM];
	VPE_SCALE_FACTORS   vpe_sca_factor_param[VPE_MAX_RES_NUM];
	uint32_t            vpe_dce_en;
	uint32_t            vpe_dce_stripe;
} VPE_BGL_STRIPE_PARAM;



typedef struct _VPE_HW_IN_COL_PARAM_ {
	VPE_COL_SZ_CFG in_col_info[VPE_MAX_COL_NUM];
	uint32_t col_num;
} VPE_HW_IN_COL_PARAM;


typedef struct _VPE_OUT_MST_INFO_ {
	uint32_t sca_in_crop_size_h;
	uint32_t sca_in_crop_size_v;
	uint32_t sca_in_crop_pos_x;
	uint32_t sca_in_crop_pos_y;

	uint32_t sca_out_size_h;    // scaling size h
	uint32_t sca_out_size_v;    // scaling size v

	uint32_t sca_out_crop_size_h;  // target crop width
	uint32_t sca_out_crop_size_v;  // target crop height
	uint32_t sca_out_crop_pos_x;   // target crop start position x
	uint32_t sca_out_crop_pos_y;   // target crop start position y

	uint32_t rlt_pos_x;     // result output position x
	uint32_t rlt_pos_y;     // result output position y
	uint32_t rlt_size_h;    // result output width
	uint32_t rlt_size_v;    // result output height

	uint32_t out_pos_x;     // final output position x after target crop
	uint32_t out_pos_y;     // final output position y after target crop
	uint32_t out_size_h;    // final output width after target crop
	uint32_t out_size_v;    // final output height after target crop

	int32_t scl_h_init_ofs_int;
	int32_t scl_h_init_ofs_dec;
	int32_t vpe_scl_h_ftr_init;

	int32_t scl_v_init_ofs_int;
	int32_t scl_v_init_ofs_dec;
	int32_t vpe_scl_v_ftr_init;

	uint32_t skip_en;
} VPE_OUT_MST_INFO;

typedef struct _VPE_SCA_SRC_IN_STRIPE_PARAM_ {
	VPE_OUT_MST_INFO sca_stp_info[VPE_MAX_COL_NUM];  // sw output stripe parameters

	uint32_t sca_factor_h;
	uint32_t sca_factor_v;

	uint32_t sca_hsca_div;
	uint32_t sca_havg_pxl_msk;
	uint32_t sca_vsca_div;
	uint32_t sca_vavg_pxl_msk;

	uint32_t stp_overlap_size;
	uint32_t valid_stp_num;
	uint32_t valid_stp_start_idx;
	uint32_t valid_stp_end_idx;
} VPE_SCA_SRC_IN_STRIPE_PARAM;

typedef struct _RES_SCA_INFO_ {
	uint32_t sca_en;

	uint32_t sca_in_size_h;
	uint32_t sca_in_pos_start_x;
	uint32_t sca_in_pos_end_x;

	uint32_t sca_out_size_h;

	uint32_t sca_factor_h;

	bool sca_proc_done;
} RES_SCA_INFO;


#if 0
extern bool vpe_stripe_cal(STRIPE_PARAM *get_stp_info, STP_MODE stp_mode, uint32_t width, uint32_t stripe_size, uint32_t stp_overlap_size);

extern int32_t vpe_get_basic_stripe(VPE_IMG_SCALE_PARAM *p_scl_info, uint32_t path_num, uint32_t overlap_size);

//void vpe_user_to_hw_param_map(VPE_BGL_STRIPE_PARAM *p_in_src, VPE_BGL_STRIPE_PARAM *p_out_dst);

extern void vpe_cal_src_in_stripe(VPE_IMG_SCALE_PARAM *p_set_scl_info, STRIPE_PARAM *p_get_stp_info);

extern void vpe_get_in_column_info(VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, STRIPE_PARAM *p_stp_sw_info, VPE_HW_IN_COL_PARAM *p_stp_hw_info);

extern void vpe_refine_in_column_info(VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, VPE_HW_IN_COL_PARAM *p_stp_hw_info);

extern void vpe_cal_sca_in_stripe(uint32_t res_num, VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, STRIPE_PARAM *p_src_in_stp_info, VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info);

extern void vpe_get_out_column_info(uint32_t res_num, VPE_BGL_STRIPE_PARAM *p_stp_gbl_info, VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info);

extern uint32_t vpe_get_sca_div(uint32_t sca_factor);

extern uint32_t vpe_get_sca_avg_pxl_mask(uint32_t sca_factor);

extern void vpe_refine_out_column_info(VPE_SCA_SRC_IN_STRIPE_PARAM *p_sca_in_stp_info);
#endif

extern int vpe_cal_col_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);


#endif


