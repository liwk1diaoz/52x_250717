#ifndef __VPE_CONFIG_BASE_H__
#define __VPE_CONFIG_BASE_H__

//#include "vpe_platform.h"
#include "vpe_ll_base.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

#define VPE_TOTAL_RES                4

#define VPE_TOTAL_COL                8

#define VPE_REG_OFFSET               4
#define VPE_REG_MAXBIT              32



typedef struct _VPE_REG_DEFINE_ {
	UINT32 nameIndex;
	UINT8  support_proc;
	UINT32 reg_add;
	UINT32 numofbits;
	UINT32 startBit;
	INT32 reg_min;
	INT32 reg_max;
	INT32 reg_val[VPE_TOTAL_RES];
} VPE_REG_DEFINE;

typedef struct _VPE_GLO_CTL_CFG_ {
	INT32 sharpen_en;
	INT32 dce_en;
	INT32 dctg_en;
	INT32 dctg_gamma_lut_en;
	INT32 dce_2d_lut_en;
	INT32 debug_type;
	INT32 col_num;
	INT32 src_format;
	INT32 sharpen_out_sel;
	INT32 src_drt;
	INT32 src_uv_swap;
} VPE_GLO_CTL_CFG;

typedef struct _VPE_GLO_SZ_CFG_ {
	INT32 src_width;
	INT32 src_height;
	INT32 proc_height;
	INT32 presca_merge_width;
	INT32 proc_y_start;
	INT32 dce_width;
	INT32 dce_height;
} VPE_GLO_SZ_CFG;

typedef struct _VPE_VCACHE_CFG_ {
	INT32 idx_sht;
	INT32 idx_set;
	INT32 idx_oft;
	INT32 oft_mode;
	INT32 oft_sht;
	INT32 slot_width;
	INT32 slot_height;
	INT32 oft_width;
	INT32 oft_height;
} VPE_VCACHE_CFG;

typedef struct _VPE_DCE_CFG_ {
	INT32 dce_mode;
	INT32 lut2d_sz;
	INT32 lsb_rand;
	INT32 fovbound;
	INT32 boundy;
	INT32 boundu;
	INT32 boundv;
	INT32 cent_x_s;
	INT32 cent_y_s;
	INT32 xdist_a1;
	INT32 ydist_a1;
	INT32 normfact;
	INT32 normbit;
	INT32 fovgain;
	UINT32 hfact;
	UINT32 vfact;
	INT32 xofs_i;
	UINT32 xofs_f;
	INT32 yofs_i;
	UINT32 yofs_f;
} VPE_DCE_CFG;

typedef struct _VPE_DCE_GEO_CFG_ {
	INT32 geo_lut[KDRV_VPE_GEO_LUT_NUMS];
} VPE_DCE_GEO_CFG;

typedef struct _VPE_SHARPEN_CFG_ {
	INT32 edge_weight_src_sel;
	INT32 edge_weight_th;
	INT32 edge_weight_gain;
	INT32 noise_level;
	INT32 edge_sharp_str1;
	INT32 edge_sharp_str2;
	INT32 flat_sharp_str;
	INT32 coring_th;
	INT32 blend_inv_gamma;
	INT32 bright_halo_clip;
	INT32 dark_halo_clip;
	INT32 noise_curve[KDRV_VPE_NOISE_CURVE_NUMS];
} VPE_SHARPEN_CFG;

typedef struct _VPE_PALETTE_CFG_ {
	INT32 pal_y;
	INT32 pal_cb;
	INT32 pal_cr;
} VPE_PALETTE_CFG;

typedef struct _VPE_DMA_CFG_ {
	UINT32 llc_addr;
	UINT32 src_addr;
	UINT32 dctg_gamma_addr;
	UINT32 dce_2dlut_addr;
	UINT32 des_res_dp_addr[4];
} VPE_DMA_CFG;

typedef struct _VPE_PIP_CTL_CFG_ {
	INT32 shp_ppi_idx;
	INT32 shp_ppo_idx;
	INT32 src_ppo_idx;
	INT32 sca_ppi_idx;
} VPE_PIP_CTL_CFG;

typedef struct _VPE_DCTG_CFG_ {
	UINT32 mount_type;
	UINT32 ra_en;
	UINT32 lut2d_sz;
	UINT32 lens_r;
	UINT32 lens_x_st;
	UINT32 lens_y_st;
	UINT32 theta_st;
	UINT32 theta_ed;
	UINT32 phi_st;
	UINT32 phi_ed;
	UINT32 rot_z;
	UINT32 rot_y;
} VPE_DCTG_CFG;

typedef struct _VPE_RES_CTL_CFG_ {
	uint8_t sca_en;
	uint8_t sca_bypass_en;
	uint8_t sca_crop_en;
	uint8_t tc_en;
	uint8_t sca_chroma_sel;
	uint8_t des_drt;
	uint8_t des_yuv420;
	uint8_t des_uv_swap;
	uint8_t out_bg_sel;
	uint8_t des_dp0_ycc_enc_en;
	uint8_t des_dp0_chrw;
	uint8_t des_dp0_format;
} VPE_RES_CTL_CFG;

typedef struct _VPE_RES_SIZE_CFG_ {
	uint32_t sca_height;
	uint32_t des_width;
	uint32_t des_height;
	uint32_t out_y_start;
	uint32_t out_height;
	uint32_t rlt_y_start;
	uint32_t rlt_height;
	uint32_t pip_y_start;
	uint32_t pip_height;
} VPE_RES_SIZE_CFG;

#if 0
typedef struct _VPE_COL_SZ_CFG_ {
	INT32 proc_width;
	INT32 proc_x_start;
	INT32 col_x_start;
} VPE_COL_SZ_CFG;
#else
typedef struct _VPE_COL_SZ_CFG_ {
	uint32_t proc_width;        // each column width
	uint32_t proc_x_start;      // each column start position from src(_buf)_width
	uint32_t col_x_start;       // each column processing start position from proc_width(origional image width)
} VPE_COL_SZ_CFG;

#endif

typedef struct _VPE_RES_SCA_CROP_CFG_ {
	uint32_t sca_crop_y_start;
	uint32_t sca_crop_height;
} VPE_RES_SCA_CROP_CFG;

typedef struct _VPE_RES_SCA_CFG_ {
	uint8_t  sca_iq_en;
	uint8_t sca_y_luma_algo_en;
	uint8_t sca_x_luma_algo_en;
	uint8_t sca_y_chroma_algo_en;
	uint8_t sca_x_chroma_algo_en;
	uint8_t sca_map_sel;
	uint32_t sca_hstep;
	uint32_t sca_vstep;
	uint32_t sca_hsca_divisor;
	uint32_t sca_havg_pxl_msk;
	uint32_t sca_vsca_divisor;
	uint32_t sca_vavg_pxl_msk;
	uint32_t coeffHorizontal[4];
	uint32_t coeffVertical[4];
} VPE_RES_SCA_CFG;

typedef struct _VPE_RES_TC_CFG_ {
	uint32_t tc_crop_y_start;
	uint32_t tc_crop_height;
} VPE_RES_TC_CFG;

#if 0
typedef struct _VPE_RES_COL_SIZE_CFG_ {
	INT32 sca_width;
	INT32 sca_hstep_oft;
	INT32 out_x_start;
	INT32 out_width;
	INT32 rlt_x_start;
	INT32 rlt_width;
	INT32 pip_x_start;
	INT32 pip_width;
	INT32 sca_crop_x_start;
	INT32 sca_crop_width;
	INT32 tc_crop_x_start;
	INT32 tc_crop_width;
	INT32 tc_crop_skip;
} VPE_RES_COL_SIZE_CFG;
#else
typedef struct _VPE_RES_COL_SIZE_CFG_ {
	uint32_t sca_crop_x_start;      // horizontal crop position on the desried processed image
	uint32_t sca_crop_width;        // horizontal crop width on the desried processed image

	uint32_t sca_width;             // scaing width from sca_crop_width

	uint32_t sca_hstep_oft;         // scaling initinal start position (factor)

	uint32_t tc_crop_x_start;
	uint32_t tc_crop_width;

	uint32_t out_x_start;           // horizontal output position on image destination image
	uint32_t out_width;             // horizontal output width on image destination image

	uint32_t rlt_x_start;
	uint32_t rlt_width;

	uint32_t pip_x_start;
	uint32_t pip_width;

	uint32_t tc_crop_skip;
} VPE_RES_COL_SIZE_CFG;

#endif

typedef struct _VPE_RES_COMPRESSION_CTL_CFG_ {
	INT32 res0_enc_smode_en;
	INT32 res1_enc_smode_en;
} VPE_RES_COMPRESSION_CTL_CFG;

typedef struct _VPE_ENCODER_Q_TABLE_CFG_ {
	INT32 enc_p[3];
	INT32 dct_qtbl_dcmax;
	INT32 dec_p[3];
} VPE_ENCODER_Q_TABLE_CFG;

typedef struct _VPE_ENCODER_Q_DC_CFG_ {
	INT32 yrc_lncnt_lfn[6]; //debug register
	INT32 dct_maxdist; //debug register
} VPE_ENCODER_Q_DC_CFG;

typedef struct _VPE_ENCODER_COMPRESSION_CFG_ {
	VPE_RES_COMPRESSION_CTL_CFG     res_compression_ctl;
	VPE_ENCODER_Q_TABLE_CFG         encoder_q_table[16];
	VPE_ENCODER_Q_DC_CFG            encoder_q_dc;
} VPE_ENCODER_COMPRESSION_CFG;


typedef struct _VPE_RES_CFG_ {
	VPE_RES_CTL_CFG       res_ctl;
	VPE_RES_SIZE_CFG      res_size;
	VPE_RES_COL_SIZE_CFG  res_col_size[8];
	VPE_RES_SCA_CROP_CFG  res_sca_crop;
	VPE_RES_SCA_CFG       res_sca;
	VPE_RES_TC_CFG        res_tc;
} VPE_RES_CFG;

typedef struct VPE_DRV_CFG {
	VPE_GLO_CTL_CFG             glo_ctl;
	VPE_GLO_SZ_CFG              glo_sz;
	VPE_COL_SZ_CFG              col_sz[8];
	VPE_DCE_CFG                 dce;
	VPE_DCE_GEO_CFG             dce_geo;
	VPE_DCTG_CFG                dctg;
	VPE_SHARPEN_CFG             sharpen;
	VPE_PALETTE_CFG             palette[8];
	VPE_DMA_CFG                 dma;
	VPE_PIP_CTL_CFG             pip_ctl;
	VPE_VCACHE_CFG              vcache;
	VPE_RES_CFG                 res[4];
	VPE_ENCODER_COMPRESSION_CFG encoder_compression;
} VPE_DRV_CFG;

typedef struct _VPE_ALL_JOB_LIST_ {
	KDRV_VPE_LIST_HEAD        job_list;
	vk_spinlock_t              job_lock;

	UINT32                     ll_frm_done;
	KDRV_VPE_JOB_LIST *ll_process;
	void (*callback)(void *param);
	UINT16                     llcmd_id[VPE_LLCMD_NUM];
	UINT16                     llcmd_count;

} VPE_ALL_JOB_LIST;

#define VPE_MAX_RES              4

extern int vpe_drv_sca_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);
extern int vpe_drv_col_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);

//extern int vpe_drv_execute(void);



#endif
