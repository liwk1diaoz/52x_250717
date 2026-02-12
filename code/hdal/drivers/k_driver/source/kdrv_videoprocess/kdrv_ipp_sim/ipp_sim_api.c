#include <kwrap/file.h>
#include "kwrap/type.h"

//#include <frammap/frammap_if.h>

#include "ipp_sim_drv.h"
#include "ipp_sim_dbg.h"
#include "ipp_sim_api.h"

#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"

//#include <kwrap/file.h>
#include <kwrap/task.h>
#include <kwrap/cpu.h>

#if (defined __UITRON || defined __ECOS)
#elif defined(__FREERTOS)

#include <stdlib.h>
#include "malloc.h"

//SC
#include "string.h"
#include "rcw_macro.h"
#include "io_address.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "kwrap/type.h"
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
//end

#if defined(_BSP_NA51055_)
#include "nvt-sramctl.h"
#endif

#else

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <comm/nvtmem.h>
#include <mach/fmem.h>

#endif

#include "kdrv_videocapture/kdrv_sie.h"
#include "kdrv_videoprocess/kdrv_ife.h"
#include "kdrv_videoprocess/kdrv_dce.h"
#include "kdrv_videoprocess/kdrv_ipe.h"
#include "kdrv_videoprocess/kdrv_ife2.h"
#include "kdrv_videoprocess/kdrv_ime.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"

#include "kdrv_type.h"



#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define ALIGN_ROUND_32(a)       ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define ALIGN_ROUND_16(a)       ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define ALIGN_ROUND_8(a)        ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define ALIGN_ROUND_4(a)        ALIGN_ROUND(a, 4)   ///< Round Off to 4

#define ALIGN_CEIL_32(a)        ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define ALIGN_CEIL_16(a)        ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define ALIGN_CEIL_8(a)         ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define ALIGN_CEIL_4(a)         ALIGN_CEIL(a, 4)    ///< Round Up to 4

#define ALIGN_FLOOR_32(a)       ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define ALIGN_FLOOR_16(a)       ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define ALIGN_FLOOR_8(a)        ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define ALIGN_FLOOR_4(a)        ALIGN_FLOOR(a, 4)   ///< Round down to 4

#define KDRV_IPP_CEIL_64(n) ((((n) + 63) >> 6) << 6)

#define IFE_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))

#if defined (__FREERTOS)
unsigned int ipp_sim_debug_level = NVT_DBG_IND;
#endif

static BOOL ime_lca_set_param_enable = FALSE;

UINT32 g_ife_fend_cnt = 0;
UINT32 g_dce_fend_cnt = 0;
UINT32 g_ipe_fend_cnt = 0;
UINT32 g_ime_fend_cnt = 0;

UINT32 g_ife_fstart_cnt = 0;
UINT32 g_dce_fstart_cnt = 0;
UINT32 g_ipe_fstart_cnt = 0;
UINT32 g_ime_fstart_cnt = 0;

typedef struct {
	BOOL enable;
	UINT32 *vaddr;
	UINT32 *paddr;
	UINT32 size;
	void *handle;
} IPP_BUFF_INFO;

IPP_BUFF_INFO ife_in_buf_a = {0};
#if (KDRV_IPP_NEW_INIT_FLOW == ENABLE)
IPP_BUFF_INFO init_ipp_buff = {0};
#endif
//-------------------------------------------------




typedef struct _nvt_kdrv_ipp_sie_info_ {
	UINT32 crop_out_width;
	UINT32 crop_out_height;
} nvt_kdrv_ipp_sie_info;

typedef struct _nvt_kdrv_ipp_ife_info_ {
	KDRV_IFE_IN_MODE ife_op_mode;

	UINT32 in_bit;
	UINT32 out_bit;

	UINT32 in_raw_width;
	UINT32 in_raw_height;
	UINT32 in_raw_lineoffset;

	UINT32 in_raw_crop_width;
	UINT32 in_raw_crop_height;
	UINT32 in_raw_crop_pos_x;
	UINT32 in_raw_crop_pos_y;

	UINT32 in_hn;
	UINT32 in_hl;
	UINT32 in_hm;

	UINT32 in_raw_addr_a;
	UINT32 in_raw_addr_b;
} nvt_kdrv_ipp_ife_info;

typedef struct _nvt_kdrv_ipp_dce_info_ {
	KDRV_DCE_IN_SRC dce_op_mode;
	KDRV_DCE_STRIPE_TYPE dce_strp_type;
	UINT32 in_width;
	UINT32 in_height;
	UINT32 addr_y;
	UINT32 addr_uv;
	UINT32 new_disto_rate;
} nvt_kdrv_ipp_dce_info;

typedef struct _nvt_kdrv_ipp_ipe_info_ {
	KDRV_IPE_IN_SRC ipe_op_mode;

	UINT32 in_width;
	UINT32 in_height;
} nvt_kdrv_ipp_ipe_info;

typedef struct _nvt_kdrv_ipp_ime_info_ {
	KDRV_IME_IN_SRC ime_op_mode;

	UINT32 in_width;
	UINT32 in_height;

	UINT32 out_p1_size_h;
	UINT32 out_p1_size_v;
	UINT32 out_p1_lofs_y;
	UINT32 out_p1_lofs_uv;

	UINT32 out_p2_size_h;
	UINT32 out_p2_size_v;
	UINT32 out_p2_lofs_y;
	UINT32 out_p2_lofs_uv;

	UINT32 out_ref_size_h;
	UINT32 out_ref_size_v;
	UINT32 out_ref_lofs_y;
	UINT32 out_ref_lofs_uv;

	UINT32 out_lca_size_h;
	UINT32 out_lca_size_v;
	UINT32 out_lca_lofs_y;
	KDRV_IME_LCA_IN_SRC lca_in_src;

} nvt_kdrv_ipp_ime_info;

typedef struct _nvt_kdrv_ipp_ife2_info_ {
	KDRV_IFE2_OUT_DST ife2_op_mode;

	UINT32 in_width;
	UINT32 in_height;
	UINT32 in_lofs;
	UINT32 in_addr;
	UINT32 out_addr;
} nvt_kdrv_ipp_ife2_info;

nvt_kdrv_ipp_sie_info g_set_sie_param;
nvt_kdrv_ipp_ife_info g_set_ife_param;
nvt_kdrv_ipp_dce_info g_set_dce_param;
nvt_kdrv_ipp_ipe_info g_set_ipe_param;
nvt_kdrv_ipp_ime_info g_set_ime_param;
nvt_kdrv_ipp_ife2_info g_set_ife2_param;


typedef enum _nvt_kdrv_ipp_id_ {
	nvt_kdrv_id_sie  = 0,
	nvt_kdrv_id_ife,
	nvt_kdrv_id_dce,
	nvt_kdrv_id_ipe,
	nvt_kdrv_id_ime,
	nvt_kdrv_id_ife2,
	nvt_kdrv_id_max,
	ENUM_DUMMY4WORD(nvt_kdrv_ipp_id)
} nvt_kdrv_ipp_id;





UINT32 set_kdrv_id[nvt_kdrv_id_max] = {0};

static UINT64 kdrv_ipp_sim_do_64b_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif
	return dividend;
}

static void nvt_kdrv_ipp_set_id(void)
{
	set_kdrv_id[nvt_kdrv_id_sie]  = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VDOCAP_ENGINE0, 0);
	set_kdrv_id[nvt_kdrv_id_ife]  = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE_ENGINE0, 0);
	set_kdrv_id[nvt_kdrv_id_dce]  = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_DCE_ENGINE0, 0);
	set_kdrv_id[nvt_kdrv_id_ipe]  = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IPE_ENGINE0, 0);
	set_kdrv_id[nvt_kdrv_id_ime]  = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0, 0);
	set_kdrv_id[nvt_kdrv_id_ife2] = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0, 0);
}

static int nvt_kdrv_ipp_load_file(char *file, UINT32 file_load_addr, UINT32 file_load_size)
{
	VOS_FILE fp;
	int len;

	// load y file
	fp = vos_file_open(file, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file);
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		return -E_NOMEM;
#else
		return -EFAULT;
#endif
	}
	//vk_printk("load file: %s,  to addr: %08x, size: %08x\r\n", file, file_load_addr, file_load_size);
	len = vos_file_read(fp, (void *)file_load_addr, file_load_size);
	DBG_IND("ipp: read file lenght: %d\r\n", len);
	vos_file_close(fp);

	vos_cpu_dcache_sync(file_load_addr, ALIGN_CEIL_32(file_load_size), VOS_DMA_TO_DEVICE);

	return 0;
}

KDRV_SIE_OPENCFG sie_opencfg;
KDRV_SIE_MCLK_INFO sie_mclk_info;
//SIE_MCLKSRC_SEL sie_mclk_src = SIE_MCLKSRC_480;

KDRV_IFE_OPENCFG ife_opencfg;
KDRV_DCE_OPENCFG dce_opencfg;
KDRV_IPE_OPENCFG ipe_opencfg;
KDRV_IME_OPENCFG ime_opencfg;
KDRV_IFE2_OPENCFG ife2_opencfg;


static UINT32 nvt_kdrv_ife_fend_flag = 0;
static UINT32 nvt_kdrv_ife_fstart_flag = 0;
static INT32 nvt_kdrv_ipp_ife_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IFE_INT_FMD) {
		nvt_kdrv_ife_fend_flag = 1;
		g_ife_fend_cnt += 1;
	}

	if (sts & KDRV_IFE_INT_SIE_FRAME_START) {
		nvt_kdrv_ife_fstart_flag = 1;
		g_ife_fstart_cnt += 1;
	}
	return 0;
}

static UINT32 nvt_kdrv_dce_fend_flag = 0;
static UINT32 nvt_kdrv_dce_fstart_flag = 0;
static INT32 nvt_kdrv_ipp_dce_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_DCE_INT_FMD) {
		nvt_kdrv_dce_fend_flag = 1;
		g_dce_fend_cnt += 1;
	}

	if (sts & KDRV_DCE_INT_FST) {
		nvt_kdrv_dce_fstart_flag = 1;
		g_dce_fstart_cnt += 1;
	}
	return 0;
}

static UINT32 nvt_kdrv_ipe_fend_flag = 0;
static UINT32 nvt_kdrv_ipe_fstart_flag = 0;
static INT32 nvt_kdrv_ipp_ipe_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IPE_INT_FMD) {
		nvt_kdrv_ipe_fend_flag = 1;
		g_ipe_fend_cnt += 1;
	}

	if (sts & KDRV_IPE_INT_FMS) {
		nvt_kdrv_ipe_fstart_flag = 1;
		g_ipe_fstart_cnt += 1;
	}
	return 0;
}

static UINT32 nvt_kdrv_ime_fend_flag = 0;
static UINT32 nvt_kdrv_ime_fstart_flag = 0;
static INT32 nvt_kdrv_ipp_ime_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IME_INT_FRM_END) {
		nvt_kdrv_ime_fend_flag = 1;

		g_ime_fend_cnt += 1;
	}

	if (sts & KDRV_IME_INT_FRM_START) {
		nvt_kdrv_ime_fstart_flag = 1;

		g_ime_fstart_cnt += 1;
	}

	return 0;
}


static UINT32 nvt_kdrv_ife2_fend_flag = 0;
static INT32 nvt_kdrv_ipp_ife2_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IFE2_INT_FMD) {
		nvt_kdrv_ife2_fend_flag = 1;
	}
	return 0;
}


//--------------------------------------------------------------------

ER kdrv_ipp_alloc_buff(IPP_BUFF_INFO *buff, unsigned int req_size)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	buff->size = KDRV_IPP_CEIL_64(req_size);
	buff->vaddr = (UINT32 *)malloc(buff->size * sizeof(UINT8));
	if (buff->vaddr == NULL) {
		DBG_ERR("get buffer fail, size = %u->%u\n", req_size, (unsigned int)buff->size);
		return -E_NOMEM;
	}
#else
	int ret = 0;
	struct nvt_fmem_mem_info_t linux_buff = {0};
	//buff->size = ((req_size + 3) / 4) * 4;
	buff->size = KDRV_IPP_CEIL_64(req_size);
	DBG_IND("buff size 0x%x\r\n", buff->size);
	ret = nvt_fmem_mem_info_init(&linux_buff, NVT_FMEM_ALLOC_CACHE, buff->size, NULL);
	if (ret >= 0) {
		buff->handle = nvtmem_alloc_buffer(&linux_buff);//save handle to release it
		buff->vaddr = (UINT32 *)linux_buff.vaddr;
		buff->paddr = (UINT32 *)linux_buff.paddr;
	} else {
		DBG_ERR("get buffer fail, size = %u\n", (unsigned int)buff->size);
		return -EFAULT;
	}
#endif
	/*
	 *DBG_IND("vaddr 0x%08x\r\n", (unsigned int)buff->vaddr);
	 *DBG_IND("paddr 0x%08x\r\n", (unsigned int)buff->paddr);
	 *DBG_IND("size 0x%08x\r\n", (unsigned int)buff->size);
	 */

	return E_OK;
}

void kdrv_ipp_rel_buff(IPP_BUFF_INFO *buff)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	free(buff->vaddr);
	buff->vaddr = NULL;
#else
	int ret = 0;
	ret = nvtmem_release_buffer(buff->handle);
	buff->handle = NULL;
	if (ret < 0) {
		pr_info("line %d: error releasing buffer\n", __LINE__);
	}
#endif
}

#if 1
IPP_BUFF_INFO sie_out_buf0 = {0};
static int sie_out_size_h = 0, sie_out_size_v = 0;
static int sie_out_lofs = 0;
static UINT32 sie_out_addr;
static UINT32 sie_out_buf_size;
#endif



static KDRV_SIE_DATA_FMT sie_data_fmt;
//KDRV_SIE_SIGNAL signal = {KDRV_SIE_PHASE_RISING, KDRV_SIE_PHASE_RISING, KDRV_SIE_PHASE_RISING, FALSE, FALSE};
static KDRV_SIE_INT sie_inte = KDRV_SIE_INT_VD | KDRV_SIE_INT_BP1 | KDRV_SIE_INT_BP2 | KDRV_SIE_INT_BP3 | KDRV_SIE_INT_DRAM_OUT0_END;
//KDRV_SIE_CCIR_INFO ccir_info = {0};
static KDRV_SIE_PIX sie_cfa = KDRV_SIE_RGGB_PIX_R;
static URECT sie_act_win;
static URECT sie_crp_win;
//static UINT32 sie_lofs[6] = {0};
static KDRV_SIE_PATGEN_INFO sie_patgen;
static KDRV_SIE_OUTPUT_MODE_TYPE sie_out_mode;
static KDRV_SIE_OUT_DEST sie_out_dst = KDRV_SIE_OUT_DEST_DIRECT;
//static KDRV_SIE_BP1 sie_bp1;
//static KDRV_SIE_BP2 sie_bp2;
//static KDRV_SIE_BP3 sie_bp3;
static KDRV_SIE_TRIG_INFO sie_trig_cfg;


int nvt_kdrv_ipp_sie_set_param(UINT32 id, nvt_kdrv_ipp_sie_info *p_set_param)
{
	sie_data_fmt = KDRV_SIE_BAYER_12;
	kdrv_sie_set(id, KDRV_SIE_ITEM_DATA_FMT, (void *)&sie_data_fmt);

	sie_inte = (
				   KDRV_SIE_INT_VD |
				   KDRV_SIE_INT_BP1 |
				   KDRV_SIE_INT_BP2 |
				   KDRV_SIE_INT_BP3 |
				   KDRV_SIE_INT_DRAM_OUT0_END |
				   KDRV_SIE_INT_CRPST |
				   KDRV_SIE_INT_CROPEND
			   );
	//kdrv_sie_set(id, KDRV_SIE_PARAM_IPL_ISRCB, (void*)DAL_SIE_ISR_CB);KDRV_SIE_ISRCB
	kdrv_sie_set(id, KDRV_SIE_ITEM_INTE, (void *)&sie_inte);

	sie_patgen.mode = KDRV_SIE_PAT_COLORBAR;
	sie_patgen.val = 0x20;
	sie_patgen.src_win.w = p_set_param->crop_out_width + 256;
	sie_patgen.src_win.h = p_set_param->crop_out_height + 32;
	kdrv_sie_set(id, KDRV_SIE_ITEM_PATGEN_INFO, (void *)&sie_patgen);

	sie_out_mode = KDRV_SIE_NORMAL_OUT;
	kdrv_sie_set(id, KDRV_SIE_ITEM_OUTPUT_MODE, (void *)&sie_out_mode); // 520


	sie_act_win.x = 128;
	sie_act_win.y = 16;
	sie_act_win.w = p_set_param->crop_out_width;
	sie_act_win.h = p_set_param->crop_out_height;
	kdrv_sie_set(id, KDRV_SIE_ITEM_ACT_WIN, (void *)&sie_act_win);

	sie_crp_win.x = 0;
	sie_crp_win.y = 0;
	sie_crp_win.w = p_set_param->crop_out_width;
	sie_crp_win.h = p_set_param->crop_out_height;
	kdrv_sie_set(id, KDRV_SIE_ITEM_CROP_WIN, (void *)&sie_crp_win);

	DBG_IND("sie_crp_win %dx%d\r\n", (int)sie_crp_win.w, (int)sie_crp_win.h);


	sie_cfa = KDRV_SIE_RGGB_PIX_R;
	sie_out_dst = KDRV_SIE_OUT_DEST_DIRECT;
	//kdrv_sie_set(id, KDRV_SIE_PARAM_IPL_RAWCFA, (void *)&sie_cfa);  //jarkko tmp mark for new sie drv
	kdrv_sie_set(id, KDRV_SIE_ITEM_OUT_DEST, (void *)&sie_out_dst);


	if ((sie_out_dst == KDRV_SIE_OUT_DEST_BOTH) || (sie_out_dst == KDRV_SIE_OUT_DEST_DRAM)) {
		sie_out_size_h = sie_crp_win.w;
		sie_out_size_v = sie_crp_win.h;
		sie_out_lofs = ALIGN_CEIL_4(sie_out_size_h * 12 / 8);
		sie_out_buf_size = (sie_out_lofs * sie_out_size_v);

		if (kdrv_ipp_alloc_buff(&sie_out_buf0, sie_out_buf_size) != E_OK) {
			return 1;
		};
		DBG_IND("sie_out_buf0.vaddr = 0x%08x\r\n", (unsigned int)sie_out_buf0.vaddr);

		kdrv_sie_set(id, KDRV_SIE_ITEM_CH0_LOF, (void *)&sie_out_lofs);
		kdrv_sie_set(id, KDRV_SIE_ITEM_CH0_ADDR, (void *)&sie_out_addr);
		//kdrv_sie_set(id, KDRV_SIE_PARAM_IPL_CH1_ADDR, (void*)&ch1_addr);
		//kdrv_sie_set(id, KDRV_SIE_PARAM_IPL_CH2_ADDR, (void*)&ch2_addr);
	}

	return 0;
}

int nvt_kdrv_ipp_sie_set_load_param(UINT32 id, nvt_kdrv_ipp_sie_info *p_set_param, UINT32 fnum)
{
	sie_crp_win.x = 0;
	sie_crp_win.y = 0;
	sie_crp_win.w = p_set_param->crop_out_width;
	sie_crp_win.h = p_set_param->crop_out_height;
	kdrv_sie_set(id, KDRV_SIE_ITEM_CROP_WIN, (void *)&sie_crp_win);


	kdrv_sie_set(id, KDRV_SIE_ITEM_LOAD, NULL);

	DBG_IND("SIE set load done!\r\n");

	return 0;
}
//--------------------------------------------------------------------

static KDRV_IFE_NRS_PARAM     g_kdrv_ife_nrs;
//static KDRV_IFE_FUSION_PARAM  g_kdrv_ife_fusion;
static KDRV_IFE_FUSION_INFO   g_kdrv_ife_fusion_info;
static KDRV_IFE_FCURVE_PARAM  g_kdrv_ife_fcurve;

static KDRV_IFE_OUTL_PARAM   g_kdrv_ife_outl;
static KDRV_IFE_FILTER_PARAM g_kdrv_ife_2DNR;
static KDRV_IFE_CGAIN_PARAM  g_kdrv_ife_cgain;
static KDRV_IFE_VIG_PARAM    g_kdrv_ife_vig;
static KDRV_IFE_VIG_CENTER   g_kdrv_ife_vig_center;
static KDRV_IFE_GBAL_PARAM   g_kdrv_ife_gbal;

static KDRV_IFE_IN_IMG_INFO ife_in_img_info = {0};
static KDRV_IFE_IN_OFS      ife_in_img_lofs = {0};
static KDRV_IFE_IN_ADDR     ife_in_img_addr = {0};
static KDRV_IFE_MIRROR      mirror = {0};
static KDRV_IFE_RDE_INFO    rde_info = {0};
//KDRV_IFE_IN_ADDR     in_img_addr = {0};
//UINT32 in_addr;
static KDRV_IFE_OUT_IMG_INFO ife_out_img_info = {0};
static KDRV_IFE_INTE ife_inte_en;

static KDRV_IFE_PARAM_IPL_FUNC_EN ife_ipl_func_ctrl = {0};



#if 0
static int nvt_kdrv_ife_wait_frame_start(BOOL clear_flag)
{
	/*wait end*/
	while (!nvt_kdrv_ife_fstart_flag) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		vos_task_delay_ms(1);
#else
		msleep(1);
#endif
	}

	if (clear_flag == TRUE) {
		nvt_kdrv_ife_fstart_flag = 0;
	}

	return 0;
}
#endif


char ife_in_raw[260] = "/mnt/sd/ipp/input/";

int nvt_kdrv_ipp_ife_set_param(UINT32 id, nvt_kdrv_ipp_ife_info *p_set_param)
{
	UINT32 in_w, in_h, in_bit, out_bit;//, hn, hm, hl;
	UINT16 crop_w, crop_h, crop_hpos, crop_vpos;
	//UINT8  h_shift;
	//UINT32 FNUM;
	UINT32 ife_in_addr;
	UINT32 ife_in_buf_size;

	in_w = p_set_param->in_raw_width;//1920;//simple_strtoul(pargv[1], NULL, 0);
	in_h = p_set_param->in_raw_height;//1080;//simple_strtoul(pargv[2], NULL, 0);
	in_bit = p_set_param->in_bit;//simple_strtoul(pargv[3], NULL, 0);
	out_bit = p_set_param->out_bit;//simple_strtoul(pargv[5], NULL, 0);

	crop_w  = p_set_param->in_raw_crop_width;
	crop_h  = p_set_param->in_raw_crop_height;
	crop_hpos = p_set_param->in_raw_crop_pos_x;
	crop_vpos = p_set_param->in_raw_crop_pos_y;
	/*  h_shift = 0;

	    hn = p_set_param->in_hn;
	    hl = p_set_param->in_hl;
	    hm = p_set_param->in_hm;*/

	//FNUM = 0;


	if (p_set_param->ife_op_mode == KDRV_IFE_IN_MODE_IPP) {
		ife_in_buf_size = (p_set_param->in_raw_lineoffset * p_set_param->in_raw_height);
		if (kdrv_ipp_alloc_buff(&ife_in_buf_a, ife_in_buf_size)) {
			return 1;
		}
		DBG_IND("ife_in_buf_a.vaddr = 0x%08x\r\n", (unsigned int)ife_in_buf_a.vaddr);

		ife_in_addr = (UINT32)ife_in_buf_a.vaddr;
		nvt_kdrv_ipp_load_file(ife_in_raw, ife_in_addr, ife_in_buf_size);

		p_set_param->in_raw_addr_a = ife_in_addr;
		p_set_param->in_raw_addr_b = ife_in_addr;
	}

	//---------NRS--------------------//
	g_kdrv_ife_nrs.enable = 0;
	/*
	    g_kdrv_ife_nrs.order.enable     = 1;
	    g_kdrv_ife_nrs.order.rng_bri    = 7
	    g_kdrv_ife_nrs.order.rng_dark   = 1
	    g_kdrv_ife_nrs.order.diff_th    = 591;
	    for(i = 0; i < 8; i++){
	        g_kdrv_ife_nrs.order.dark_w[i]  = IFE_F_NRS_ORD_DARK_WLUT[i];
	        g_kdrv_ife_nrs.order.bri_w[i]  = IFE_F_NRS_ORD_BRI_WLUT[i];
	    }

	    g_kdrv_ife_nrs.bilat.b_enable   = 1;
	    g_kdrv_ife_nrs.bilat.b_str      = 4;
	    for(i = 0; i < 6; i++){
	        g_kdrv_ife_nrs.bilat.b_ofs[i]      = NRS_B_OFS[i];
	        g_kdrv_ife_nrs.bilat.b_weight[i]   = NRS_B_W[i];
	    }
	    for(i = 0; i < 5; i++){
	        g_kdrv_ife_nrs.bilat.b_rng[i]      = NRS_B_RNG[i];
	        g_kdrv_ife_nrs.bilat.b_th[i]       = NRS_B_TH[i];
	    }

	    g_kdrv_ife_nrs.bilat.gb_enable  = 1;
	    g_kdrv_ife_nrs.bilat.gb_str     = 4;
	    g_kdrv_ife_nrs.bilat.gb_blend_w = 9;
	    for(i = 0; i < 6; i++){
	        g_kdrv_ife_nrs.bilat.gb_ofs[i]     = NRS_GB_OFS[i];
	        g_kdrv_ife_nrs.bilat.gb_weight[i]  = NRS_GB_W[i];
	    }
	    for(i = 0; i < 5; i++){
	        g_kdrv_ife_nrs.bilat.gb_rng[i]     = NRS_GB_RNG[i];
	        g_kdrv_ife_nrs.bilat.gb_th[i]      = NRS_GB_TH[i];
	    }
	*/
	//---------Fusion--------------------//
	g_kdrv_ife_fusion_info.enable = 0;
	/*
	    g_kdrv_ife_fusion.fnum = FNUM;

	    g_kdrv_ife_fusion.fu_ctrl.mode = 1;
	    g_kdrv_ife_fusion.fu_ctrl.y_mean_sel = 2;
	    g_kdrv_ife_fusion.fu_ctrl.ev_ratio = 231;

	    g_kdrv_ife_fusion.bld_cur.nor_sel= 0;
	    g_kdrv_ife_fusion.bld_cur.dif_sel= 2;

	    g_kdrv_ife_fusion.bld_cur.l_dif_knee[0] = 953;
	    g_kdrv_ife_fusion.bld_cur.l_dif_knee[1] = 2636;
	    g_kdrv_ife_fusion.bld_cur.l_dif_range   = 12;
	    g_kdrv_ife_fusion.bld_cur.l_dif_w_edge  = 1;
	    g_kdrv_ife_fusion.bld_cur.l_nor_knee[0] = 1783;
	    g_kdrv_ife_fusion.bld_cur.l_nor_knee[1] = 3345;
	    g_kdrv_ife_fusion.bld_cur.l_nor_range   = 3;
	    g_kdrv_ife_fusion.bld_cur.l_nor_w_edge  = 1;

	    g_kdrv_ife_fusion.bld_cur.s_dif_knee[0] = 220;
	    g_kdrv_ife_fusion.bld_cur.s_dif_knee[1] = 1577;
	    g_kdrv_ife_fusion.bld_cur.s_dif_range   = 1;
	    g_kdrv_ife_fusion.bld_cur.s_dif_w_edge  = 1;
	    g_kdrv_ife_fusion.bld_cur.s_nor_knee[0] = 1450;
	    g_kdrv_ife_fusion.bld_cur.s_nor_knee[1] = 2299;
	    g_kdrv_ife_fusion.bld_cur.s_nor_range   = 7;
	    g_kdrv_ife_fusion.bld_cur.s_nor_w_edge  = 0;

	    g_kdrv_ife_fusion.dk_sat.th[0]         = 2749;
	    g_kdrv_ife_fusion.dk_sat.th[1]         = 3013;
	    g_kdrv_ife_fusion.dk_sat.step[0]       = 215;
	    g_kdrv_ife_fusion.dk_sat.step[1]       = 12;
	    g_kdrv_ife_fusion.dk_sat.low_bound[0]  = 230;
	    g_kdrv_ife_fusion.dk_sat.low_bound[1]  = 74;

	    g_kdrv_ife_fusion.mc_para.diff_ratio   = 0;
	    g_kdrv_ife_fusion.mc_para.lum_th       = 3903;
	    g_kdrv_ife_fusion.mc_para.dwd          = 9;

	    for (i = 0; i < 16; i++) {
	        g_kdrv_ife_fusion.mc_para.neg_diff_w[i]   = MC_NEG_DIFF_W[i];
	        g_kdrv_ife_fusion.mc_para.pos_diff_w[i]   = MC_POS_DIFF_W[i];
	    }

	    g_kdrv_ife_fusion.s_comp.enable      = 1;
	    g_kdrv_ife_fusion.s_comp.knee[0]     = 103;
	    g_kdrv_ife_fusion.s_comp.knee[1]     = 651;
	    g_kdrv_ife_fusion.s_comp.knee[2]     = 1086;
	    for (i = 0; i < 4; i++) {
	        g_kdrv_ife_fusion.s_comp.shift[i]       = SCOMP_SUB[i];
	        g_kdrv_ife_fusion.s_comp.sub_point[i]   = SCOMP_SHIFT[i];
	    }
	    */
	//---------Fcurve--------------------//
	g_kdrv_ife_fcurve.enable = 0;

	//---------Outlier--------------------//
	g_kdrv_ife_outl.enable = 0;
	/*  g_kdrv_ife_info.bright_th[0] = 189;
	    g_kdrv_ife_info.bright_th[1] = 1035;
	    g_kdrv_ife_info.bright_th[2] = 1828;
	    g_kdrv_ife_info.bright_th[3] = 2720;
	    g_kdrv_ife_info.bright_th[4] = 3092;
	    g_kdrv_ife_info.dark_th[0] = 473;
	    g_kdrv_ife_info.dark_th[1] = 1359;
	    g_kdrv_ife_info.dark_th[2] = 1484;
	    g_kdrv_ife_info.dark_th[3] = 2448;
	    g_kdrv_ife_info.dark_th[4] = 2483;
	    g_kdrv_ife_info.outl_bright_ofs = 225;
	    g_kdrv_ife_info.outl_dark_ofs = 161;
	    g_kdrv_ife_info.outl_weight = 144;
	    g_kdrv_ife_info.outl_crs_chan_enable = 1;
	    g_kdrv_ife_info.outl_cnt[0] = 15;
	    g_kdrv_ife_info.outl_cnt[1] = 5;
	    */
	//------------2DNR------------------//

	g_kdrv_ife_2DNR.enable = 0;
	/*
	    g_kdrv_ife_2DNR.nlm_ker.locw_enable    = 1; //0x138
	    g_kdrv_ife_2DNR.nlm_ker.nlm_ker_enable = 1; //0x138
	    g_kdrv_ife_2DNR.nlm_ker.bilat_wd1      = 6; //0x138
	    g_kdrv_ife_2DNR.nlm_ker.bilat_wd2      = 3; //0x138
	    g_kdrv_ife_2DNR.nlm_ker.ker_slope0 = 10; //0x138
	    g_kdrv_ife_2DNR.nlm_ker.ker_slope1 = 67; //0x138
	    for (tmp_i = 0; tmp_i < 8; tmp_i++) {
	        g_kdrv_ife_2DNR.nlm_ker.ker_radius[tmp_i] = radius[tmp_i]; //0x13c~0x140
	    }

	    g_kdrv_ife_2DNR.nlm_lut.mwth[0] = 161; //0x148
	    g_kdrv_ife_2DNR.nlm_lut.mwth[1] = 37;  //0x148
	    for (tmp_i = 0; tmp_i < 4; tmp_i++) {
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wb[tmp_i]  = wb[tmp_i]; //0x148
	    }
	    for (tmp_i = 0; tmp_i < 8; tmp_i++) {
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wa [tmp_i] = wa[tmp_i] ; //0x144
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wbh[tmp_i] = wbh[tmp_i];
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wbl[tmp_i] = wbl[tmp_i];
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wbm[tmp_i] = wbm[tmp_i];
	    }
	    for (tmp_i = 0; tmp_i < 7; tmp_i++) {
	        g_kdrv_ife_2DNR.nlm_lut.bilat_wc[tmp_i] = wc[tmp_i];
	    }
	    for (tmp_i = 0; tmp_i < 17; tmp_i++) {
	        g_kdrv_ife_2DNR.range.range_a_lut[tmp_i] = range_a_lut[tmp_i];
	        g_kdrv_ife_2DNR.range.range_b_lut[tmp_i] = range_b_lut[tmp_i];
	    }
	    for (tmp_i = 0; tmp_i < 6; tmp_i++) {
	        g_kdrv_ife_2DNR.range.range_a_th[tmp_i] = rth_a[tmp_i];
	        g_kdrv_ife_2DNR.range.range_b_th[tmp_i] = rth_b[tmp_i];
	    }
	    g_kdrv_ife_2DNR.range.rng_bilat.enable  = 1;   //0x44
	    g_kdrv_ife_2DNR.range.rng_bilat.th1     = 65;  //0x44
	    g_kdrv_ife_2DNR.range.rng_bilat.th2     = 745; //0x44

	    g_kdrv_ife_2DNR.spatial.s_only_enable   = 0;
	    g_kdrv_ife_2DNR.spatial.s_only_filt_len = 0;
	    for (tmp_i = 0; tmp_i < 10; tmp_i++) {
	        g_kdrv_ife_2DNR.spatial.weight[tmp_i] = s_weight[tmp_i];
	    }

	    g_kdrv_ife_2DNR.filt_w     = 5; //<- RTH_W
	    g_kdrv_ife_2DNR.bit_dither = 0;
	    g_kdrv_ife_2DNR.bin        = 0;

	    g_kdrv_ife_2DNR.clamp.dlt  = 302;
	    g_kdrv_ife_2DNR.clamp.mul  = 69;
	    g_kdrv_ife_2DNR.clamp.th   = 3741;
	    */
	//------------Color gain------------//
	g_kdrv_ife_cgain.enable = 1;

	g_kdrv_ife_cgain.hinv   = 0;
	g_kdrv_ife_cgain.inv    = 0;
	g_kdrv_ife_cgain.mask   = 4095;

	g_kdrv_ife_cgain.cgain_r  = 443;
	g_kdrv_ife_cgain.cgain_gr = 256;
	g_kdrv_ife_cgain.cgain_gb = 256;
	g_kdrv_ife_cgain.cgain_b  = 421;

	g_kdrv_ife_cgain.cofs_r   = 0;
	g_kdrv_ife_cgain.cofs_gr  = 0;
	g_kdrv_ife_cgain.cofs_gb  = 0;
	g_kdrv_ife_cgain.cofs_b   = 0;

	g_kdrv_ife_cgain.bit_field = 0;

	//------------- Vig ---------------//
	g_kdrv_ife_vig.enable = 0;
	/*
	    g_kdrv_ife_vig.t = 109;
	    g_kdrv_ife_vig.tab_gain = 2;
	    g_kdrv_ife_vig.x_div = 3882;
	    g_kdrv_ife_vig.y_div = 3995;
	    g_kdrv_ife_vig.dither_enable = 1;
	    g_kdrv_ife_vig.dither_rst_enable = 1;
	    for (tmp_i = 0; tmp_i < 17; tmp_i++) {
	        g_kdrv_ife_vig.ch0_lut[tmp_i] = vig_ch0_lut[tmp_i];
	        g_kdrv_ife_vig.ch1_lut[tmp_i] = vig_ch1_lut[tmp_i];
	        g_kdrv_ife_vig.ch2_lut[tmp_i] = vig_ch2_lut[tmp_i];
	        g_kdrv_ife_vig.ch3_lut[tmp_i] = vig_ch3_lut[tmp_i];
	    }
	    */
	g_kdrv_ife_vig_center.ch0.x = 240;
	g_kdrv_ife_vig_center.ch1.x = 240;
	g_kdrv_ife_vig_center.ch2.x = 327;
	g_kdrv_ife_vig_center.ch3.x = 367;

	g_kdrv_ife_vig_center.ch0.y = 43;
	g_kdrv_ife_vig_center.ch1.y = 87;
	g_kdrv_ife_vig_center.ch2.y = 51;
	g_kdrv_ife_vig_center.ch3.y = 55;

	//------------ Gbal ----------------//
	g_kdrv_ife_gbal.enable    = 0;
	/*
	    for (tmp_i = 0; tmp_i < 17; tmp_i++) {
	        g_kdrv_ife_gbal.gbal_dtab[tmp_i] = gbal_dtab[tmp_i];
	        g_kdrv_ife_gbal.gbal_stab[tmp_i] = gbal_stab[tmp_i];
	    }
	    for (tmp_i = 0; tmp_i < 4; tmp_i++) {
	        g_kdrv_ife_gbal.ir_sub[tmp_i] = ir_sub[tmp_i];
	    }

	    g_kdrv_ife_gbal.gbal_th = 594;
	    g_kdrv_ife_gbal.ir_sub_ctc_gain = 0;
	    */
	//----------------------------------//

	/*set input img parameters*/
	ife_in_img_info.in_src = p_set_param->ife_op_mode;
	ife_in_img_info.type = KDRV_IFE_IN_BAYER;
	ife_in_img_info.img_info.width = in_w;
	ife_in_img_info.img_info.height = in_h;
	/*ife_in_img_info.img_info.stripe_hn = hn;
	ife_in_img_info.img_info.stripe_hl = hl;
	ife_in_img_info.img_info.stripe_hm = hm;*/
	ife_in_img_info.img_info.st_pix = KDRV_IPP_RGGB_PIX_R;
	//ife_in_img_info.img_info.cfa_pat = cfa_pat;
	ife_in_img_info.img_info.bit = in_bit;

	ife_in_img_info.img_info.crop_width  = crop_w;
	ife_in_img_info.img_info.crop_height = crop_h;
	ife_in_img_info.img_info.crop_hpos   = crop_hpos;
	ife_in_img_info.img_info.crop_vpos   = crop_vpos;
	//ife_in_img_info.img_info.h_shift     = h_shift;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&ife_in_img_info);

	ife_in_img_lofs.line_ofs_1 = p_set_param->in_raw_lineoffset;
	ife_in_img_lofs.line_ofs_2 = 0;
	/*
	if (FNUM == 0) {
	    ife_in_img_lofs.line_ofs_2 = 0;
	} else {
	    ife_in_img_lofs.line_ofs_2 = p_set_param->in_raw_lineoffset;
	}
	*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_1, (void *)&ife_in_img_lofs);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_OFS_2, (void *)&ife_in_img_lofs);

	if (ife_in_img_info.in_src !=  KDRV_IFE_IN_MODE_DIRECT) {
		/*set input1 address*/
		ife_in_img_addr.ife_in_addr_1 = (UINT32)p_set_param->in_raw_addr_a;
		kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_1, (void *)&ife_in_img_addr);
		DBG_IND("kdrv_ife_Set In Addr1: 0x%08x !!\r\n", (unsigned int)ife_in_img_addr.ife_in_addr_1);
		/*set input2 address*/
		ife_in_img_addr.ife_in_addr_2 = (UINT32)p_set_param->in_raw_addr_b;//in2_buf_info.vaddr;
		kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_ADDR_2, (void *)&ife_in_img_addr);
		DBG_IND("kdrv_ife_Set In Addr2: 0x%08x !!\r\n", (unsigned int)ife_in_img_addr.ife_in_addr_2);
	}


	/*set output img parameters*/
	ife_out_img_info.line_ofs = ALIGN_CEIL_4((in_w * out_bit) >> 3);
	ife_out_img_info.bit = out_bit;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_IMG, (void *)&ife_out_img_info);

	/*set output address*/
	//out_addr = (UINT32)out_buf_info.vaddr;
	//kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OUT_ADDR, (void *)&out_addr);

	mirror.enable = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_MIRROR, (void *)&mirror);

	rde_info.enable = 0;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_RDE_INFO, (void *)&rde_info);

	/*set interrupt enable*/
	ife_inte_en = KDRV_IFE_INTE_ALL;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_INTER, (void *)&ife_inte_en);


	/* set outlier para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_OUTL, (void *)&g_kdrv_ife_outl);

	/* set 2dnr para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_FILTER, (void *)&g_kdrv_ife_2DNR);

	/* set color gain para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_CGAIN, (void *)&g_kdrv_ife_cgain);

	/* set Vig para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_VIG, (void *)&g_kdrv_ife_vig);
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_VIG_CENTER, (void *)&g_kdrv_ife_vig_center);

	/* set gbal para*/
	kdrv_ife_set(id, KDRV_IFE_PARAM_IQ_GBAL, (void *)&g_kdrv_ife_gbal);

	ife_ipl_func_ctrl.ipl_ctrl_func = (KDRV_IFE_IQ_FUNC_CGAIN);
	ife_ipl_func_ctrl.enable = TRUE;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)&ife_ipl_func_ctrl);


	DBG_IND("kdrv_ife_set done\r\n");

	return 0;
}

int nvt_kdrv_ipp_ife_set_load_param(UINT32 id, nvt_kdrv_ipp_ife_info *p_set_param, UINT32 fnum)
{
	//UINT32 i;

	ife_ipl_func_ctrl.enable = TRUE;
	ife_ipl_func_ctrl.ipl_ctrl_func = 0x000001ff;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)(&ife_ipl_func_ctrl));

	ife_in_img_info.type                 = 0;
	ife_in_img_info.img_info.bit         = 12;
	ife_in_img_info.img_info.width       = p_set_param->in_raw_width;
	ife_in_img_info.img_info.height      = p_set_param->in_raw_height;
	ife_in_img_info.img_info.crop_width  = p_set_param->in_raw_crop_width;
	ife_in_img_info.img_info.crop_height = p_set_param->in_raw_crop_height;
	ife_in_img_info.img_info.crop_hpos   = p_set_param->in_raw_crop_pos_x;
	ife_in_img_info.img_info.crop_vpos   = p_set_param->in_raw_crop_pos_y;
	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&ife_in_img_info);


	kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_LOAD, NULL);

	DBG_IND("IFE set load done!\r\n");

	return 0;
}
//-------------------------------------------------------------------------

static void *handle_wdrin0_buff = NULL;
static void *handle_wdrout0_buff = NULL;
static void *handle_wdrin1_buff = NULL;
static void *handle_wdrout1_buff = NULL;
static void *handle_cfaout_buff = NULL;
/*
 *struct nvt_fmem_mem_info_t wdrin0_buf_info = {0};
 *struct nvt_fmem_mem_info_t wdrout0_buf_info = {0};
 *struct nvt_fmem_mem_info_t wdrin1_buf_info = {0};
 *struct nvt_fmem_mem_info_t wdrout1_buf_info = {0};
 *struct nvt_fmem_mem_info_t cfaout_buf_info = {0};
 */
IPP_BUFF_INFO wdrin0_buf_info = {0};
IPP_BUFF_INFO wdrout0_buf_info = {0};
IPP_BUFF_INFO wdrin1_buf_info = {0};
IPP_BUFF_INFO wdrout1_buf_info = {0};
IPP_BUFF_INFO cfaout_buf_info = {0};


UINT32 dce_gdc_lut[65] = {
	//31.29%
	/*
	 *65535, 64828, 64235, 63662, 63107, 62569, 62047, 61540, 61046, 60566,
	 *60099, 59644, 59201, 58768, 58346, 57934, 57532, 57139, 56755, 56380,
	 *56013, 55653, 55302, 54958, 54621, 54290, 53967, 53650, 53339, 53034,
	 *52734, 52441, 52152, 51869, 51592, 51319, 51050, 50787, 50528, 50273,
	 *50023, 49776, 49534, 49296, 49061, 48830, 48603, 48379, 48158, 47941,
	 *47727, 47517, 47309, 47104, 46902, 46704, 46507, 46314, 46123, 45935,
	 *45749, 45566, 45385, 45207, 45031
	 */
	//32.15%
	65535, 64823, 64195, 63599, 63029, 62480, 61949, 61435,
	60937, 60452, 59981, 59521, 59073, 58635, 58208, 57791,
	57383, 56983, 56593, 56210, 55836, 55469, 55109, 54757,
	54411, 54072, 53740, 53413, 53093, 52779, 52470, 52167,
	51869, 51576, 51288, 51006, 50728, 50454, 50186, 49921,
	49661, 49405, 49153, 48906, 48661, 48421, 48185, 47952,
	47722, 47496, 47273, 47053, 46837, 46624, 46413, 46206,
	46002, 45800, 45601, 45405, 45211, 45020, 44832, 44646,
	44462,
};
UINT32 mod_dce_gdc_lut[65] = {0};

//0.5%, (max - min) / 65536 <= 0.005
INT32 dce_cac_lut_r[65] = {
	-125, -97, -27, -97, -138, -110, -41, -110, -110, -55,
	-83, 169, -125, -83, -13, -98, -70, -127, -14, -14,
	-42, -14, -128, -28, -143, -129, 169, -130, -29, -58,
	-14, -29, -44, -73, -73, -59, -104, -119, -89, -149,
	-90, -30, -105, -90, -75, -60, -91, -137, -107, -92,
	-138, 169, -46, -62, -109, -15, -141, -157, -78, -94,
	-158, -63, -95, -111, -47
};
INT32 dce_cac_lut_b[65] = {
	-111, -41, 169, -13, -69, -97, -41, -27, -110, -137,
	-124, -27, -41, -139, -97, -126, -98, -56, -42, 169,
	169, -128, -143, -114, -28, -129, -130, -130, -116, -145,
	-43, -29, -29, -132, -59, 169, -148, -104, -104, -59,
	169, -30, -75, -106, -151, -121, -122, 169, -138, -46,
	169, 169, -62, -62, -124, 169, -125, -15, -31, -31,
	-158, -111, -127, -15, 169
};


static KDRV_DCE_INTE dce_inte_en = {0};
static KDRV_DCE_IN_INFO in_info = {0};
static KDRV_DCE_IMG_IN_DMA_INFO in_dma_info = {0};
//static KDRV_DCE_IMG_OUT_DMA_INFO out_dma_info = {0};
//static KDRV_DCE_STRIPE_INFO ipl_stripe = {0};
//static KDRV_DCE_EXTEND_INFO extend_info = {0};
static KDRV_DCE_SUBIMG_IN_ADDR wdr_subin_addr = {0};
static KDRV_DCE_SUBIMG_OUT_ADDR wdr_subout_addr = {0};
static KDRV_DCE_WDR_SUBIMG_PARAM wdr_subimg_info = {0};
static KDRV_DCE_DC_CAC_PARAM dc_cac_info = {0};
static KDRV_DCE_FOV_PARAM fov_info = {0};
static KDRV_DCE_TRIG_TYPE_INFO dce_trig_info = {0};
static KDRV_DCE_STRIPE_PARAM dce_stripe = {0};
static KDRV_DCE_GDC_CENTER_INFO gdc_cent = {0};
static KDRV_DCE_DC_SRAM_SEL dc_sram_sel = {0};
static KDRV_DCE_CFA_PARAM cfa_info = {0};
//static KDRV_DCE_2DLUT_PARAM lut2d_info = {0};
//static KDRV_DCE_FOV_PARAM fov_info = {0};
static KDRV_DCE_WDR_PARAM wdr_param = {0};
static KDRV_DCE_HIST_PARAM hist_param = {0};
static KDRV_DCE_CFA_SUBOUT_INFO cfa_subout_param = {0};
static KDRV_DCE_PARAM_IPL_FUNC_EN ipl_all_func_en = {0};

KDRV_DCE_IN_IMG_INFO dce_in_img_info = {0};
KDRV_DCE_OUT_IMG_INFO dce_out_img_info = {0};

#if 0
static int nvt_kdrv_dce_wait_frame_start(BOOL clear_flag)
{
	/*wait end*/
	while (!nvt_kdrv_dce_fstart_flag) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		vos_task_delay_ms(1);
#else
		msleep(1);
#endif
	}

	if (clear_flag == TRUE) {
		nvt_kdrv_dce_fstart_flag = 0;
	}

	return 0;
}
#endif


int nvt_kdrv_ipp_dce_set_param(UINT32 id, nvt_kdrv_ipp_dce_info *p_set_param)
{
	UINT32 size = 0;
	INT32 i = 0;
	//UINT32 *p_2dlut = NULL;

	/*get wdr in buffer*/
	size = 16 * 9 * 2 * 4;
	if (kdrv_ipp_alloc_buff(&wdrin0_buf_info, size) != E_OK) {
		return 1;
	}
	handle_wdrin0_buff = wdrin0_buf_info.handle;
	if (kdrv_ipp_alloc_buff(&wdrin1_buf_info, size) != E_OK) {
		return 1;
	}
	handle_wdrin1_buff = wdrin1_buf_info.handle;

	/*get wdr out buffer*/
	size = 16 * 9 * 2 * 4;
	if (kdrv_ipp_alloc_buff(&wdrout0_buf_info, size) != E_OK) {
		return 1;
	}
	handle_wdrout0_buff = wdrout0_buf_info.handle;
	if (kdrv_ipp_alloc_buff(&wdrout1_buf_info, size) != E_OK) {
		return 1;
	}
	handle_wdrout1_buff = wdrout1_buf_info.handle;

	/*get cfa out buffer*/
	size = (((p_set_param->in_width * p_set_param->in_height) + 3) / 8) * 4;
	if (kdrv_ipp_alloc_buff(&cfaout_buf_info, size) != E_OK) {
		return 1;
	}
	handle_cfaout_buff = cfaout_buf_info.handle;
	DBG_IND("subimg input : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (unsigned int)wdrin0_buf_info.vaddr, (unsigned int)wdrin0_buf_info.paddr);
	DBG_IND("subimg input : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (unsigned int)wdrin1_buf_info.vaddr, (unsigned int)wdrin1_buf_info.paddr);
	DBG_IND("subimg output : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (unsigned int)wdrout0_buf_info.vaddr, (unsigned int)wdrout0_buf_info.paddr);
	DBG_IND("subimg output : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (unsigned int)wdrout1_buf_info.vaddr, (unsigned int)wdrout1_buf_info.paddr);
	DBG_IND("cfa output : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (unsigned int)cfaout_buf_info.vaddr, (unsigned int)cfaout_buf_info.paddr);

	/*set input information*/
	in_info.in_src = p_set_param->dce_op_mode;
	in_info.type = KDRV_DCE_D2D_FMT_Y_PACK_UV422;
	in_info.yuvfmt = KDRV_DCE_YUV2RGB_FMT_FULL;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&in_info);
	//DBG_IND("set KDRV_DCE_PARAM_IPL_IN_INFO\r\n");

#if 0
	dce_stripe.stripe_type = KDRV_DCE_STRIPE_CUSTOMER;
	for (i = 0; i < 16; i++) {
		dce_stripe.hstp[i] = 64;
	}
#else
	dce_stripe.stripe_type = p_set_param->dce_strp_type;
#endif
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_STRIPE_PARAM, (void *)&dce_stripe);
	//DBG_IND("set KDRV_DCE_PARAM_IQ_STRIPE_PARAM\r\n");

	/*set input img parameters*/
	dce_in_img_info.img_size_h = p_set_param->in_width;
	dce_in_img_info.img_size_v = p_set_param->in_height;
	dce_in_img_info.lineofst[KDRV_DCE_YUV_Y] = ((p_set_param->in_width + 3) / 4) * 4;
	dce_in_img_info.lineofst[KDRV_DCE_YUV_UV] = ((p_set_param->in_width + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&dce_in_img_info);
	//DBG_IND("set KDRV_DCE_PARAM_IPL_IN_IMG\r\n");


	/*set input address*/
	if (p_set_param->dce_op_mode == KDRV_DCE_IN_SRC_DCE_IME) {
		in_dma_info.addr_y = (UINT32)(p_set_param->addr_y);
		in_dma_info.addr_cbcr = (UINT32)(p_set_param->addr_uv);
		kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IMG_DMA_IN, (void *)&in_dma_info);
		//DBG_IND("set KDRV_DCE_PARAM_IPL_IMG_DMA_IN\r\n");
	}

	/*set output img parameters*/
	dce_out_img_info.crop_start_h = 0;
	dce_out_img_info.crop_start_v = 0;
	dce_out_img_info.crop_size_h = p_set_param->in_width;
	dce_out_img_info.crop_size_v = p_set_param->in_height;
	dce_out_img_info.lineofst[KDRV_DCE_YUV_Y] = ((dce_out_img_info.crop_size_h + 3) / 4) * 4;
	dce_out_img_info.lineofst[KDRV_DCE_YUV_UV] = ((dce_out_img_info.crop_size_h + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&dce_out_img_info);
	//DBG_IND("KDRV_DCE_PARAM_IPL_OUT_IMG\r\n");

	/*set output address*/
	//out_dma_info.addr_y = (UINT32)yout_buf_info.vaddr;
	//out_dma_info.addr_cbcr = (UINT32)uvout_buf_info.vaddr;
	//kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IMG_DMA_OUT, (void *)&out_dma_info);
	////DBG_IND("KDRV_DCE_PARAM_IPL_IMG_DMA_OUT\r\n");

	/*set wdr subimg input address*/
	wdr_subin_addr.addr = (UINT32)wdrin0_buf_info.vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBIN, (void *)&wdr_subin_addr);
	DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBIN\r\n");

	/*set wdr subimg output address*/
	wdr_subout_addr.enable = FALSE;
	wdr_subout_addr.addr = (UINT32)wdrout0_buf_info.vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBOUT, (void *)&wdr_subout_addr);
	DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBOUT\r\n");

	/*set CFA parameteres*/
	if (p_set_param->dce_op_mode == KDRV_DCE_IN_SRC_DIRECT) {
		cfa_info.cfa_enable = TRUE;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_CFA, (void *)&cfa_info);
		DBG_IND("KDRV_DCE_PARAM_IQ_CFA\r\n");
	}

	/*set Cfa subout parameteres*/
	cfa_subout_param.cfa_subout_enable = FALSE;
	cfa_subout_param.cfa_subout_flip_enable = FALSE;
	cfa_subout_param.cfa_addr = (UINT32)cfaout_buf_info.vaddr;
	cfa_subout_param.cfa_lofs = ((p_set_param->in_width + 3) / 8) * 4;
	cfa_subout_param.subout_ch_sel = KDRV_DCE_CFA_SUBOUT_CH1;
	cfa_subout_param.subout_byte = KDRV_DCE_CFA_SUBOUT_1BYTE;
	cfa_subout_param.subout_shiftbit = 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_CFAOUT, (void *)&cfa_subout_param);

	//WDR
	kdrv_dce_get(id, KDRV_DCE_PARAM_IQ_WDR, (void *)&wdr_param);
	wdr_param.wdr_enable = FALSE;
	wdr_param.tonecurve_enable = FALSE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR, (void *)&wdr_param);

	/*set WDR parameteres*/
	ipl_all_func_en.enable = wdr_param.wdr_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_WDR;
	ipl_all_func_en.enable = wdr_param.tonecurve_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func |= KDRV_DCE_IQ_FUNC_TONECURVE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set WDR subimg parameteres*/
	wdr_subimg_info.subimg_size_h = 16;
	wdr_subimg_info.subimg_size_v = 9;
	wdr_subimg_info.subimg_lofs_in = 16 * 4 * 2;
	wdr_subimg_info.subimg_lofs_out = 16 * 4 * 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR_SUBIMG, (void *)&wdr_subimg_info);

	/*set Histogram parameteres*/
	hist_param.hist_sel = KDRV_AFTER_WDR;
	hist_param.step_h = 0;
	hist_param.step_v = 0;
	ipl_all_func_en.enable = hist_param.hist_enable = FALSE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_HIST, (void *)&hist_param);
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_HISTOGRAM;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set GDC center*/
	gdc_cent.geo_center_x = p_set_param->in_width / 2;
	gdc_cent.geo_center_y = p_set_param->in_height / 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_GDC_CENTER, (void *)&gdc_cent);
	//DBG_IND("KDRV_DCE_PARAM_IPL_GDC_CENTER\r\n");

	/*set DC parameters*/
	dc_sram_sel.sram_mode = KDRV_DCE_SRAM_CNN;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DC_SRAM_MODE, (void *)&dc_sram_sel);

	ipl_all_func_en.enable = dc_cac_info.dc_enable = TRUE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_DC;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	dc_cac_info.dc.geo_dist_x = 4095;
	dc_cac_info.dc.geo_dist_y = 4095;
	for (i = 0; i < 65; i++) {
		mod_dce_gdc_lut[i] = 65535 - (UINT32)(kdrv_ipp_sim_do_64b_div(((UINT64)(dce_gdc_lut[0] - dce_gdc_lut[i]) * (UINT64)p_set_param->new_disto_rate * (UINT64)dce_gdc_lut[0]), ((UINT64)(dce_gdc_lut[0] - dce_gdc_lut[64]) * 100)));
	}
	memcpy(dc_cac_info.dc.geo_lut_g, mod_dce_gdc_lut, GDC_TABLE_NUM * 4);
	/*memcpy(dc_cac_info.dc.geo_lut_g, dce_gdc_lut, GDC_TABLE_NUM * 4);*/
	DBG_IND("GDC distortion rate %d\r\n", (int)p_set_param->new_disto_rate);

	/*set CAC parameters*/
	ipl_all_func_en.enable = dc_cac_info.cac.cac_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_CAC;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	dc_cac_info.cac.cac_sel = 1;
	if (dc_cac_info.cac.cac_sel == 0) {
		dc_cac_info.cac.geo_r_lut_gain = 4000;
		dc_cac_info.cac.geo_g_lut_gain = 4096;
		dc_cac_info.cac.geo_b_lut_gain = 4200;
	} else {
		memcpy(dc_cac_info.cac.geo_lut_r, dce_cac_lut_r, GDC_TABLE_NUM * 4);
		memcpy(dc_cac_info.cac.geo_lut_b, dce_cac_lut_b, GDC_TABLE_NUM * 4);
	}
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_DC_CAC, (void *)&dc_cac_info);
	//DBG_IND("KDRV_DCE_PARAM_IQ_DC_CAC\r\n");

	/*set FOV gain*/
	fov_info.fov_bnd_sel = KDRV_DCE_FOV_BND_REG_RGB;
	fov_info.fov_gain = 1228;
	fov_info.fov_bnd_b = 0;
	fov_info.fov_bnd_r = 0;
	fov_info.fov_bnd_g = 0;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_FOV, (void *)&fov_info);
	ipl_all_func_en.enable = TRUE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_FOV;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set interrupt enable*/
	dce_inte_en = KDRV_DCE_INTE_ALL;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_INTER, (void *)&dce_inte_en);
	//DBG_IND("set KDRV_DCE_PARAM_IPL_INTER\r\n");

#if 0
	/*set 2DLUT parameters*/
	ipl_all_func_en.enable = lut2d_info.lut_2d_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_2DLUT;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	lut2d_info.lut_num_select = KDRV_DCE_2DLUT_65_65_GRID;
	lut2d_info.lut_2d_y_dist_off = 0;
	for (i = 0; i < 65 * 65; i++) {
		lut2d_info.lut_2d_value[i] = p_2dlut[i];
	}
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_2DLUT, (void *)&lut2d_info);

	/*set FOV parameters*/
	fov_info.fov_bnd_sel = KDRV_DCE_FOV_BND_DUPLICATE;
	fov_info.fov_gain = 1190;
	fov_info.fov_bnd_b = 1023;
	fov_info.fov_bnd_r = 1023;
	fov_info.fov_bnd_g = 1023;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_FOV, (void *)&fov_info);
#endif

	if (p_set_param->dce_op_mode == KDRV_DCE_IN_SRC_DCE_IME) {
		ipl_all_func_en.ipl_ctrl_func = (KDRV_DCE_IQ_FUNC_WDR | KDRV_DCE_IQ_FUNC_TONECURVE);
	} else {
		ipl_all_func_en.ipl_ctrl_func = (KDRV_DCE_IQ_FUNC_CFA);
	}
	ipl_all_func_en.enable = TRUE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	return 0;
}

int nvt_kdrv_ipp_dce_set_load_param(UINT32 id, nvt_kdrv_ipp_dce_info *p_set_param, UINT32 fnum)
{
	/*set input img parameters*/
	dce_in_img_info.img_size_h = p_set_param->in_width;
	dce_in_img_info.img_size_v = p_set_param->in_height;
	dce_in_img_info.lineofst[KDRV_DCE_YUV_Y] = ((p_set_param->in_width + 3) / 4) * 4;
	dce_in_img_info.lineofst[KDRV_DCE_YUV_UV] = ((p_set_param->in_width + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&dce_in_img_info);
	//DBG_IND("set KDRV_DCE_PARAM_IPL_IN_IMG\r\n");


	/*set output img parameters*/
	dce_out_img_info.crop_start_h = 0;
	dce_out_img_info.crop_start_v = 0;
	dce_out_img_info.crop_size_h = p_set_param->in_width;
	dce_out_img_info.crop_size_v = p_set_param->in_height;
	dce_out_img_info.lineofst[KDRV_DCE_YUV_Y] = ((dce_out_img_info.crop_size_h + 3) / 4) * 4;
	dce_out_img_info.lineofst[KDRV_DCE_YUV_UV] = ((dce_out_img_info.crop_size_h + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&dce_out_img_info);

	/*set wdr subimg output address*/
	if (fnum % 2 == 0) {
		wdr_subout_addr.enable = TRUE;
		wdr_subout_addr.addr = (UINT32)wdrout0_buf_info.vaddr;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBOUT, (void *)&wdr_subout_addr);
		DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBOUT, use addr 0\r\n");
	} else {
		wdr_subout_addr.enable = TRUE;
		wdr_subout_addr.addr = (UINT32)wdrout1_buf_info.vaddr;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBOUT, (void *)&wdr_subout_addr);
		DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBOUT, use addr 1\r\n");
	}

	/*set wdr subimg input address*/
	if (fnum % 2 == 0) {
		wdr_subin_addr.addr = (UINT32)wdrout1_buf_info.vaddr;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBIN, (void *)&wdr_subin_addr);
		DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBIN, use addr 0\r\n");

	} else {
		wdr_subin_addr.addr = (UINT32)wdrout0_buf_info.vaddr;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBIN, (void *)&wdr_subin_addr);
		DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBIN, use addr 1\r\n");
	}

	if (fnum == 1) {
		wdr_param.wdr_enable = TRUE;
		wdr_param.tonecurve_enable = TRUE;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR, (void *)&wdr_param);
	}

	if (fnum == 7) {
		wdr_subimg_info.subimg_size_h = 8;
		wdr_subimg_info.subimg_size_v = 5;
		wdr_subimg_info.subimg_lofs_in = 8 * 4 * 2;
		wdr_subimg_info.subimg_lofs_out = 8 * 4 * 2;
		kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR_SUBIMG, (void *)&wdr_subimg_info);
	}

	/*if ((fnum == 3)) {
	    UINT32 i = 0;
	    KDRV_DCE_HIST_RSLT hist_rslt = {0};
	    kdrv_dce_get(id, KDRV_DCE_PARAM_IPL_HIST_RSLT, (void *)&hist_rslt);

	    for (i = 0; i < HISTOGRAM_STCS_NUM; i++) {
	        DBG_USER("hist[%d] = %d \r\n", i,hist_rslt.hist_stcs[i]);
	    }
	}*/

	//set load
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_LOAD, NULL);

	/*if ((fnum == 7)) {
	    UINT32 i = 0;
	    KDRV_DCE_HIST_RSLT hist_rslt = {0};
	    kdrv_dce_get(id, KDRV_DCE_PARAM_IPL_HIST_RSLT, (void *)&hist_rslt);
	    DBG_USER("histogram = \r\n");
	    for (i = 0; i < HISTOGRAM_STCS_NUM; i++) {
	        DBG_USER("hist[%d] = %d \r\n", i,hist_rslt.hist_stcs[i]);
	    }
	}*/


	return 0;
}

static int nvt_kdrv_ipp_dce_save_output(nvt_kdrv_ipp_dce_info *g_set_dce_param)
{
	VOS_FILE fp;
	int len = 0, size = 0;
	char file_path[256] = "";

	if (wdr_subout_addr.enable == 1) {
		//size = (wdr_subimg_info.subimg_size_h * wdr_subimg_info.subimg_size_v * 8);
		sprintf(file_path, "/mnt/sd/ipp/wdr_outsub0.raw");
		fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return 1;
		}
		len = vos_file_write(fp, (void *)wdrout0_buf_info.vaddr, wdrout0_buf_info.size);
		DBG_IND("write length %d\r\n", (int)len);
		vos_file_close(fp);

		//size = (wdr_subimg_info.subimg_size_h * wdr_subimg_info.subimg_size_v * 8);
		sprintf(file_path, "/mnt/sd/ipp/wdr_outsub1.raw");
		fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return 1;
		}
		len = vos_file_write(fp, (void *)wdrout1_buf_info.vaddr, wdrout1_buf_info.size);
		DBG_IND("write length %d\r\n", (int)len);
		vos_file_close(fp);
	}

	if (cfa_subout_param.cfa_subout_enable == 1) {
		size = (((g_set_dce_param->in_width * g_set_dce_param->in_height) + 3) / 8) * 4;
		sprintf(file_path, "/mnt/sd/ipp/cfa_outsub.raw");
		fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return 1;
		}
		len = vos_file_write(fp, (void *)cfaout_buf_info.vaddr, size);
		DBG_IND("write length %d\r\n", (int)len);
		vos_file_close(fp);
	}

	kdrv_ipp_rel_buff(&wdrin0_buf_info);
	kdrv_ipp_rel_buff(&wdrout0_buf_info);
	kdrv_ipp_rel_buff(&wdrin1_buf_info);
	kdrv_ipp_rel_buff(&wdrout1_buf_info);

	return 0;
}
//--------------------------------------------------------------------------------
IPP_BUFF_INFO subin0_buf_info = {0};
IPP_BUFF_INFO subout0_buf_info = {0};
IPP_BUFF_INFO subin1_buf_info = {0};
IPP_BUFF_INFO subout1_buf_info = {0};

static KDRV_IPE_IN_IMG_INFO ipe_in_img_info = {0};
static KDRV_IPE_OUT_IMG_INFO ipe_out_img_info = {0};
//KDRV_IPE_DMA_ADDR_INFO in_addr_info = {0};
//KDRV_IPE_DMA_ADDR_INFO out_addr_info = {0};
static KDRV_IPE_SUBIMG_PARAM subimg_info = {0};
static KDRV_IPE_DMA_SUBIN_ADDR subin_addr_info = {0};
static KDRV_IPE_DMA_SUBOUT_ADDR subout_addr_info = {0};
static KDRV_IPE_INTE  ipe_inte_en = {0};
static KDRV_IPE_ETH_INFO eth_info = {0};
static KDRV_IPE_EEXT_PARAM eext_info = {0};
static KDRV_IPE_EDGE_OVERSHOOT_PARAM overshoot_info = {0};
static KDRV_IPE_EEXT_TONEMAP_PARAM eext_tonemap_info = {0};
//static KDRV_IPE_EPROC_PARAM eproc_info = {0};//
static KDRV_IPE_RGBLPF_PARAM rgblpf_info = {0};
static KDRV_IPE_PFR_PARAM  pfr_info = {0};
static KDRV_IPE_CC_EN_PARAM cc_en_info = {0};
static KDRV_IPE_CCTRL_EN_PARAM cctrl_en_info = {0};
static KDRV_IPE_CADJ_EE_PARAM cadj_ee_info = {0};
static KDRV_IPE_CADJ_YCCON_PARAM cadj_yccon_info = {0};
static KDRV_IPE_CADJ_COFS_PARAM cadj_cofs_info = {0};
static KDRV_IPE_CADJ_RAND_PARAM cadj_rand_info = {0};
static KDRV_IPE_CADJ_HUE_PARAM cadj_hue_info = {0};
static KDRV_IPE_CADJ_FIXTH_PARAM cadj_fixth_info = {0};
static KDRV_IPE_CADJ_MASK_PARAM cadj_mask_info = {0};
//static KDRV_IPE_CST_PARAM cst_info = {0};
static KDRV_IPE_CSTP_PARAM cstp_info = {0};
static KDRV_IPE_GAMYRAND_PARAM gamyrand_info = {0};
static KDRV_IPE_GAMMA_PARAM gamma_info = {0};
//static KDRV_IPE_YCURVE_PARAM ycurve_info = {0};
//static KDRV_IPE_3DCC_PARAM iq_3dcc_info = {0};
static KDRV_IPE_DEFOG_PARAM dfg_info  = {0};
static KDRV_IPE_LCE_PARAM lce_info  = {0};
static KDRV_IPE_EDGEDBG_PARAM edgdbg_info  = {0};
static KDRV_IPE_VA_PARAM va_param  = {0};
static KDRV_IPE_VA_WIN_INFO va_win_info = {0};
static KDRV_IPE_PARAM_IPL_FUNC_EN ipl_en_info = {0};
UINT32 ipe_gamma_day[129] = {
	0, 37, 73, 107, 139, 169, 198, 225, 241, 257,
	273, 289, 304, 318, 333, 347, 360, 373, 386, 399,
	411, 422, 433, 444, 455, 465, 475, 484, 493, 502,
	510, 518, 525, 532, 539, 546, 553, 560, 566, 573,
	580, 587, 594, 601, 607, 614, 621, 627, 634, 640,
	647, 653, 659, 666, 672, 678, 684, 691, 697, 703,
	709, 715, 721, 727, 733, 739, 744, 750, 756, 762,
	767, 772, 777, 782, 787, 792, 797, 801, 806, 811,
	816, 821, 826, 830, 835, 840, 845, 849, 854, 859,
	863, 868, 872, 877, 882, 886, 891, 895, 900, 904,
	908, 913, 917, 921, 926, 930, 934, 939, 943, 947,
	951, 955, 959, 964, 968, 972, 976, 980, 984, 988,
	992, 996, 1000, 1003, 1007, 1011, 1015, 1019, 1023
};

#if 0
static int nvt_kdrv_ipe_wait_frame_start(BOOL clear_flag)
{
	/*wait end*/
	while (!nvt_kdrv_ipe_fstart_flag) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		vos_task_delay_ms(1);
#else
		msleep(1);
#endif
	}

	if (clear_flag == TRUE) {
		nvt_kdrv_ipe_fstart_flag = 0;
	}

	return 0;
}
#endif


int nvt_kdrv_ipp_ipe_set_param(UINT32 id, nvt_kdrv_ipp_ipe_info *p_set_param)
{
	UINT32 in_w, in_h, size, i;

	/*get parametr*/
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EEXT, (void *)&eext_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT, (void *)&overshoot_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EEXT_TONEMAP, (void *)&eext_tonemap_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_RGBLPF, (void *)&rgblpf_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_PFR, (void *)&pfr_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CC_EN, (void *)&cc_en_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CCTRL_EN, (void *)&cctrl_en_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_EE, (void *)&cadj_ee_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_YCCON, (void *)&cadj_yccon_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_COFS, (void *)&cadj_cofs_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_RAND, (void *)&cadj_rand_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_HUE, (void *)&cadj_hue_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_FIXTH, (void *)&cadj_fixth_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_MASK, (void *)&cadj_mask_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CSTP, (void *)&cstp_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_GAMYRAND, (void *)&gamyrand_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_3DCC, (void *)&iq_3dcc_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_DEFOG, (void *)&dfg_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_LCE, (void *)&lce_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGEDBG, (void *)&edgdbg_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_VA_PARAM, (void *)&va_param);

	//edge
	rgblpf_info.enable = 0;
	pfr_info.enable = 0;
	cc_en_info.enable = 0;
	cctrl_en_info.enable = 0;
	cadj_ee_info.enable = 0;
	cadj_yccon_info.enable = 0;
	cadj_cofs_info.enable = 1;
	cadj_cofs_info.cb_ofs = 128;
	cadj_cofs_info.cr_ofs = 128;
	cadj_rand_info.enable = 0;
	cadj_hue_info.enable = 0;
	cadj_fixth_info.enable = 0;
	cadj_mask_info.enable = 1;
	cadj_mask_info.y_mask = 255;
	cadj_mask_info.cb_mask = 255;
	cadj_mask_info.cr_mask = 255;
	//cst_info.enable = 1;
	//cst_info.cst_off_sel = KDRV_IPE_CST_MINUS128;
	cstp_info.enable = 0;
	gamyrand_info.enable = 0;

	subimg_info.subimg_size.h_size = 16;
	subimg_info.subimg_size.v_size = 9;
	subimg_info.subimg_ftrcoef[0] = 4;
	subimg_info.subimg_ftrcoef[1] = 2;
	subimg_info.subimg_ftrcoef[2] = 1;
	subimg_info.subimg_lofs_in = 16 << 2;
	subimg_info.subimg_lofs_out = 16 << 2;

	dfg_info.enable = 0;
	lce_info.enable = 0;
	lce_info.diff_wt_avg = 128;
	lce_info.diff_wt_pos = 0;
	lce_info.diff_wt_neg = 0;
	lce_info.lum_wt_lut[0] = 64;
	lce_info.lum_wt_lut[1] = 68;
	lce_info.lum_wt_lut[2] = 72;
	lce_info.lum_wt_lut[3] = 85;
	lce_info.lum_wt_lut[4] = 85;
	lce_info.lum_wt_lut[5] = 85;
	lce_info.lum_wt_lut[6] = 85;
	lce_info.lum_wt_lut[7] = 72;
	lce_info.lum_wt_lut[8] = 68;
	edgdbg_info.enable = 0;
	va_param.enable = 0;
	va_param.indep_va_enable = 0;
	va_param.indep_win[0].enable = 0;
	va_param.indep_win[1].enable = 0;
	va_param.indep_win[2].enable = 0;
	va_param.indep_win[3].enable = 0;
	va_param.indep_win[4].enable = 0;
	va_param.indep_win[0].linemax_g1 = 0;
	va_param.indep_win[1].linemax_g1 = 0;
	va_param.indep_win[2].linemax_g1 = 0;
	va_param.indep_win[3].linemax_g1 = 0;
	va_param.indep_win[4].linemax_g1 = 0;
	va_param.indep_win[0].linemax_g2 = 0;
	va_param.indep_win[1].linemax_g2 = 0;
	va_param.indep_win[2].linemax_g2 = 0;
	va_param.indep_win[3].linemax_g2 = 0;
	va_param.indep_win[4].linemax_g2 = 0;
	va_param.win_num.w = 8;
	va_param.win_num.h = 5;
	va_param.va_out_grp1_2 = 1;
	va_param.va_lofs = 8 * 4 * 4;
	va_param.group_1.h_filt.symmetry = 0;
	va_param.group_1.h_filt.filter_size = 1;
	va_param.group_1.h_filt.tap_a = 2;
	va_param.group_1.h_filt.tap_b = -1;
	va_param.group_1.h_filt.tap_c = 0;
	va_param.group_1.h_filt.tap_d = 0;
	va_param.group_1.h_filt.div = 3;
	va_param.group_1.h_filt.th_l = 21;
	va_param.group_1.h_filt.th_u = 26;
	va_param.group_1.v_filt.symmetry = 0;
	va_param.group_1.v_filt.filter_size = 1;
	va_param.group_1.v_filt.tap_a = 2;
	va_param.group_1.v_filt.tap_b = -1;
	va_param.group_1.v_filt.tap_c = 0;
	va_param.group_1.v_filt.tap_d = 0;
	va_param.group_1.v_filt.div = 3;
	va_param.group_1.v_filt.th_l = 21;
	va_param.group_1.v_filt.th_u = 26;
	va_param.group_1.linemax_mode = 0;
	va_param.group_1.count_enable = 1;
	va_param.group_2.h_filt.symmetry = 0;
	va_param.group_2.h_filt.filter_size = 1;
	va_param.group_2.h_filt.tap_a = 2;
	va_param.group_2.h_filt.tap_b = -1;
	va_param.group_2.h_filt.tap_c = 0;
	va_param.group_2.h_filt.tap_d = 0;
	va_param.group_2.h_filt.div = 3;
	va_param.group_2.h_filt.th_l = 21;
	va_param.group_2.h_filt.th_u = 26;
	va_param.group_2.v_filt.symmetry = 0;
	va_param.group_2.v_filt.filter_size = 1;
	va_param.group_2.v_filt.tap_a = 2;
	va_param.group_2.v_filt.tap_b = -1;
	va_param.group_2.v_filt.tap_c = 0;
	va_param.group_2.v_filt.tap_d = 0;
	va_param.group_2.v_filt.div = 3;
	va_param.group_2.v_filt.th_l = 21;
	va_param.group_2.v_filt.th_u = 26;
	va_param.group_2.linemax_mode = 0;
	va_param.group_2.count_enable = 1;

	va_win_info.ratio_base = 1000;
	va_win_info.winsz_ratio.w = 1000;
	va_win_info.winsz_ratio.h = 1000;
	for (i = 0; i < 5; i++) {
		va_win_info.indep_roi_ratio[i].x = 1000;
		va_win_info.indep_roi_ratio[i].y = 1000;
		va_win_info.indep_roi_ratio[i].w = 1000;
		va_win_info.indep_roi_ratio[i].h = 1000;
	}

	//iq_3dcc_info.enable =  0;
	//iq_3dcc_info.cc3d_rnd.round_opt = KDRV_IPE_ROUNDING;
	//iq_3dcc_info.cc3d_rnd.rst_en = 0;

	in_w = p_set_param->in_width;
	in_h = p_set_param->in_height;

	/*subin addr */
	size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
	if (kdrv_ipp_alloc_buff(&subin0_buf_info, size) != E_OK) {
		return 1;
	}
	DBG_IND("subin0_buf_info.vaddr = 0x%08x\r\n", (unsigned int)subin0_buf_info.vaddr);

	if (kdrv_ipp_alloc_buff(&subin1_buf_info, size) != E_OK) {
		return 1;
	}
	DBG_IND("subin1_buf_info.vaddr = 0x%08x\r\n", (unsigned int)subin1_buf_info.vaddr);

	/*subout addr */
	if (kdrv_ipp_alloc_buff(&subout0_buf_info, size) != E_OK) {
		return 1;
	}
	DBG_IND("subout0_buf_info.vaddr = 0x%08x\r\n", (unsigned int)subout0_buf_info.vaddr);

	if (kdrv_ipp_alloc_buff(&subout1_buf_info, size) != E_OK) {
		return 1;
	}
	DBG_IND("subout1_buf_info.vaddr = 0x%08x\r\n", (unsigned int)subout1_buf_info.vaddr);

	/*set input img parameters*/
	ipe_in_img_info.in_src = p_set_param->ipe_op_mode;
	ipe_in_img_info.type = KDRV_IPE_IN_FMT_Y_PACK_UV444;
	ipe_in_img_info.ch.y_width = in_w;
	ipe_in_img_info.ch.y_height = in_h;
	ipe_in_img_info.ch.y_line_ofs = in_w;
	ipe_in_img_info.ch.uv_line_ofs = in_w * 2;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_IN_IMG, (void *)&ipe_in_img_info);

	/*set input address*/
	//in_addr_info.addr_y = (UINT32)y_in_buf_info.vaddr;
	//in_addr_info.addr_uv = (UINT32)uv_in_buf_info.vaddr;
	//kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_IN_ADDR, (void *)&in_addr_info);

	/*set output img parameters*/
	ipe_out_img_info.type = KDRV_IPE_IN_FMT_Y_PACK_UV444;
	ipe_out_img_info.dram_out = FALSE;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OUT_IMG, (void *)&ipe_out_img_info);

	/*set output address*/
	//out_addr_info.addr_y = (UINT32)y_out_buf_info.vaddr;
	//out_addr_info.addr_uv = (UINT32)uv_out_buf_info.vaddr;
	//kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OUT_ADDR, (void *)&out_addr_info);

	/*set subimg input/output dma address*/
	subin_addr_info.addr = (UINT32)subin0_buf_info.vaddr;
	subout_addr_info.enable = TRUE;
	subout_addr_info.addr = (UINT32)subout0_buf_info.vaddr;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBIN_ADDR, (void *)&subin_addr_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, (void *)&subout_addr_info);

	/*set eth parametr*/
	eth_info.enable = 0;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ETH, (void *)&eth_info);


#if 1//(VERF_IQ_ALL != 1) //normal case

	/*set edge eext parametr*/
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EEXT, (void *)&eext_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT, (void *)&overshoot_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EEXT_TONEMAP, (void *)&eext_tonemap_info);

	/*set parametr*/
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_RGBLPF, (void *)&rgblpf_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_PFR, (void *)&pfr_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CC_EN, (void *)&cc_en_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CCTRL_EN, (void *)&cctrl_en_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_EE, (void *)&cadj_ee_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_YCCON, (void *)&cadj_yccon_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_COFS, (void *)&cadj_cofs_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_RAND, (void *)&cadj_rand_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_HUE, (void *)&cadj_hue_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_FIXTH, (void *)&cadj_fixth_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_MASK, (void *)&cadj_mask_info);
	//kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CST, (void *)&cst_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CSTP, (void *)&cstp_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_GAMYRAND, (void *)&gamyrand_info);
	//kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_3DCC, (void *)&iq_3dcc_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_SUBIMG, (void *)&subimg_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_DEFOG, (void *)&dfg_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_LCE, (void *)&lce_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EDGEDBG, (void *)&edgdbg_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_VA_PARAM, (void *)&va_param);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_VA_WIN_INFO, (void *)&va_win_info);


	/*set gamma parametr*/;
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_GAMMA, (void *)&gamma_info);
	gamma_info.enable = 1;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_GAMMA, (void *)&gamma_info);
	DBG_IND("set KDRV_IPE_PARAM_IQ_GAMMA\r\n");

	/*set ycurve parametr*/
#if 0
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_YCURVE, (void *)&ycurve_info);
	ycurve_info.enable = 1;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_YCURVE, (void *)&ycurve_info);
	DBG_IND("set KDRV_IPE_PARAM_IQ_YCURVE\r\n");
#endif

#else // use KDRV_IPE_PARAM_IQ_ALL item

	iq_param.p_eext         = &eext_info;
	iq_param.p_eext_tonemap = &eext_tonemap_info;
	iq_param.p_eproc        = NULL;
	iq_param.p_rgb_lpf      = &rgblpf_info;
	iq_param.p_cc_en        = &cc_en_info;
	iq_param.p_cc           = NULL;
	iq_param.p_ccm          = NULL;
	iq_param.p_cctrl_en     = &cctrl_en_info;
	iq_param.p_cctrl        = NULL;
	iq_param.p_cctrl_ct     = NULL;
	iq_param.p_cadj_ee      = &cadj_ee_info;
	iq_param.p_cadj_yccon   = &cadj_yccon_info;
	iq_param.p_cadj_cofs    = &cadj_cofs_info;
	iq_param.p_cadj_rand    = &cadj_rand_info;
	iq_param.p_cadj_hue     = &cadj_hue_info;
	iq_param.p_cadj_fixth   = &cadj_fixth_info;
	iq_param.p_cadj_mask    = &cadj_mask_info;
	iq_param.p_cst          = &cst_info;
	iq_param.p_cstp         = &cstp_info;
	iq_param.p_gamy_rand    = &gamyrand_info;
	iq_param.p_gamma        = &gamma_info;
	iq_param.p_y_curve      = &ycurve_info;
	iq_param.p_3dcc         = &iq_3dcc_info;
	iq_param.p_defog        = &dfg_info;
	iq_param.p_lce          = &lce_info;
	iq_param.p_subimg       = &subimg_info;
	iq_param.p_edgedbg      = &edgdbg_info;
	iq_param.p_va           = &va_param;

	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_ALL, (void *)&iq_param);
	DBG_IND("set KDRV_IPE_PARAM_IQ_ALL\r\n");
#endif


	ipl_en_info.ipl_ctrl_func = (KDRV_IPE_IQ_FUNC_CST | KDRV_IPE_IQ_FUNC_GAMMA);
	ipl_en_info.enable = TRUE;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_en_info);

	//kdrv_ipe_fmd_flag = 0;
	//kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)&KDRV_IPE_ISR_CB);

	/*set interrupt enable*/
	ipe_inte_en = KDRV_IPE_INTE_ALL;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_INTER, (void *)&ipe_inte_en);
	DBG_IND("set KDRV_IPE_PARAM_IPL_INTER\r\n");

	return 0;
}

int nvt_kdrv_ipp_ipe_set_load_param(UINT32 id, nvt_kdrv_ipp_ipe_info *p_set_param, UINT32 fnum)
{

	/*set input img parameters*/
	ipe_in_img_info.in_src = p_set_param->ipe_op_mode;
	ipe_in_img_info.type = KDRV_IPE_IN_FMT_Y_PACK_UV444;
	ipe_in_img_info.ch.y_width = p_set_param->in_width;
	ipe_in_img_info.ch.y_height = p_set_param->in_height;
	ipe_in_img_info.ch.y_line_ofs = p_set_param->in_width;
	ipe_in_img_info.ch.uv_line_ofs = p_set_param->in_width * 2;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_IN_IMG, (void *)&ipe_in_img_info);

	if (fnum % 2 == 0) {
		subout_addr_info.enable = TRUE;
		subout_addr_info.addr = (UINT32)subout0_buf_info.vaddr;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, (void *)&subout_addr_info);
		DBG_IND("KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, use 0\r\n");
	} else {
		subout_addr_info.enable = TRUE;
		subout_addr_info.addr = (UINT32)subout1_buf_info.vaddr;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, (void *)&subout_addr_info);
		DBG_IND("KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, use 1\r\n");
	}

	if (fnum % 2 == 0) {
		subin_addr_info.addr = (UINT32)subout1_buf_info.vaddr;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBIN_ADDR, (void *)&subin_addr_info);
		DBG_IND("KDRV_IPE_PARAM_IPL_SUBIN_ADDR, use 0\r\n");
	} else {
		subin_addr_info.addr = (UINT32)subout0_buf_info.vaddr;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBIN_ADDR, (void *)&subin_addr_info);
		DBG_IND("KDRV_IPE_PARAM_IPL_SUBIN_ADDR, use 1\r\n");
	}

	if (fnum == 1) {
		dfg_info.enable = TRUE;
		//lce_info.enable = TRUE;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_DEFOG, (void *)&dfg_info);
		//kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_LCE, (void *)&lce_info);
		ipl_en_info.ipl_ctrl_func = (KDRV_IPE_IQ_FUNC_DEFOG);//| KDRV_IPE_IQ_FUNC_LCE);
		ipl_en_info.enable = TRUE;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_en_info);
	}

	/*
	{
	    KDRV_IPE_EDGE_STCS edg_stcs  = {0};
	    KDRV_IPE_EDGE_THRES_AUTO edg_th_auto  = {0};
	    if (fnum  % 2 == 1) {
	        kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGE_STCS_RSLT, (void *)&edg_stcs);

	        DBG_USER("edge stcs localmax_max =  %d \r\n", edg_stcs.localmax_max);
	        DBG_USER("edge stcs coneng_max =    %d \r\n", edg_stcs.coneng_max);
	        DBG_USER("edge stcs coneng_avg =    %d \r\n", edg_stcs.coneng_avg);

	        edg_th_auto.stcs.localmax_max = edg_stcs.localmax_max;
	        edg_th_auto.stcs.coneng_avg = edg_stcs.coneng_avg;
	        edg_th_auto.stcs.coneng_max = edg_stcs.coneng_max;
	        edg_th_auto.flat_ratio = 255;
	        edg_th_auto.edge_ratio = 64;
	        edg_th_auto.luma_ratio = 205;
	        kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGE_THRES_RSLT, (void *)&edg_th_auto);

	        DBG_USER("edge stcs th_flat = %d \r\n", edg_th_auto.region_th_auto.th_flat);
	        DBG_USER("edge stcs th_edge = %d \r\n", edg_th_auto.region_th_auto.th_edge);
	        DBG_USER("edge stcs th_luma = %d \r\n", edg_th_auto.region_th_auto.th_lum_hld);
	    }

	    if (fnum % 2 == 0) {
	        eext_info.eext_region.reg_th.th_flat = edg_th_auto.region_th_auto.th_flat;
	        eext_info.eext_region.reg_th.th_edge = edg_th_auto.region_th_auto.th_edge;
	        eext_info.eext_region.reg_th.th_flat_hld = edg_th_auto.region_th_auto.th_flat_hld;
	        eext_info.eext_region.reg_th.th_edge_hld = edg_th_auto.region_th_auto.th_edge_hld;
	        eext_info.eext_region.reg_th.th_lum_hld = edg_th_auto.region_th_auto.th_lum_hld;
	        kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EEXT, (void *)&eext_info);
	    }
	}*/

	/*
	if (fnum == 1) {
	    UINT32 i = 0;
	    gamma_info.enable = 1;
	    gamma_info.option = KDRV_IPE_GAMMA_RGB_COMBINE;
	    for (i = 0; i < 129 ; i++) {
	        gamma_info.gamma_lut[0][i] = ipe_gamma_day[128 - i];
	        gamma_info.gamma_lut[1][i] = ipe_gamma_day[128 - i];
	        gamma_info.gamma_lut[2][i] = ipe_gamma_day[128 - i];
	    }
	}*/

	if (fnum == 7) {
		subimg_info.subimg_size.h_size = 8;
		subimg_info.subimg_size.v_size = 5;
		subimg_info.subimg_lofs_in = 8 << 2;
		subimg_info.subimg_lofs_out = 8 << 2;
		kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_SUBIMG, (void *)&subimg_info);

		//kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_GAMMA, (void *)&gamma_info);
	}

	//set load
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_LOAD, NULL);

	return 0;
}

static int nvt_kdrv_ipp_ipe_save_output(void)
{
	VOS_FILE fp;
	int len;//, size;
	char file_path[256] = "";

	if (subout_addr_info.enable) {
		sprintf(file_path, "/mnt/sd/ipp/ipe_outsub0.raw");
		fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return FALSE;
		}
		//size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
		len = vos_file_write(fp, (void *)subout0_buf_info.vaddr, subout0_buf_info.size);
		DBG_IND("write length %d\r\n", (int)len);
		vos_file_close(fp);

		sprintf(file_path, "/mnt/sd/ipp/ipe_outsub1.raw");
		fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return FALSE;
		}
		//size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
		len = vos_file_write(fp, (void *)subout1_buf_info.vaddr, subout1_buf_info.size);
		DBG_IND("write length %d\r\n", (int)len);
		vos_file_close(fp);
	}

	kdrv_ipp_rel_buff(&subin0_buf_info);
	kdrv_ipp_rel_buff(&subout0_buf_info);
	kdrv_ipp_rel_buff(&subin1_buf_info);
	kdrv_ipp_rel_buff(&subout1_buf_info);

	return 0;
}
//----------------------------------------------------------------------------------
static KDRV_IFE2_IN_INFO ife2_in_img_info;
static KDRV_IFE2_OUT_IMG ife2_out_img_info;
//UINT32 in_addr = 0, out_addr = 0;

static KDRV_IFE2_INTE ife2_interrupt_en;
static KDRV_IFE2_GRAY_STATIST ife2_gray_stl;
static KDRV_IFE2_REFCENT_PARAM ife2_refcen_info;
static KDRV_IFE2_FILTER_PARAM ife2_filter_info;



int nvt_kdrv_ipp_ife2_set_param(UINT32 id, nvt_kdrv_ipp_ife2_info *p_set_param)
{
	/* set interrupt */
	ife2_interrupt_en = KDRV_IFE2_INTE_FMD;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_INTER, (void *)&ife2_interrupt_en);

	ife2_in_img_info.ch.width = p_set_param->in_width;
	ife2_in_img_info.ch.height = p_set_param->in_height;
	ife2_in_img_info.ch.line_ofs = p_set_param->in_lofs;
	ife2_in_img_info.type = KDRV_IFE2_IN_FMT_PACK_YUV444;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_IMG_INFO, (void *)&ife2_in_img_info);
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_ADDR, (void *)&p_set_param->in_addr);

	ife2_out_img_info.out_dst = p_set_param->ife2_op_mode;
	ife2_out_img_info.line_ofs = p_set_param->in_lofs;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO, (void *)&ife2_out_img_info);

	if (ife2_out_img_info.out_dst == KDRV_IFE2_OUT_DST_DRAM) {
		kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_ADDR, (void *)&p_set_param->out_addr);
	}

	ife2_gray_stl.u_th0 = 125;
	ife2_gray_stl.u_th1 = 130;
	ife2_gray_stl.v_th0 = 125;
	ife2_gray_stl.v_th1 = 130;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_GRAY_STATIST, (void *)&ife2_gray_stl);


	ife2_refcen_info.refcent_y.rng_th[0] = 6;
	ife2_refcen_info.refcent_y.rng_th[1] = 12;
	ife2_refcen_info.refcent_y.rng_th[2] = 18;
	ife2_refcen_info.refcent_y.rng_wt[0] = 8;
	ife2_refcen_info.refcent_y.rng_wt[1] = 4;
	ife2_refcen_info.refcent_y.rng_wt[2] = 2;
	ife2_refcen_info.refcent_y.rng_wt[3] = 1;
	ife2_refcen_info.refcent_y.cent_wt = 31;    ///< Reference center weighting
	ife2_refcen_info.refcent_y.outl_dth = 255;  ///< Outlier difference threshold
	ife2_refcen_info.refcent_y.outl_th = 7;   ///< Reference center outlier threshold

	ife2_refcen_info.refcent_uv.rng_th[0] = 5;
	ife2_refcen_info.refcent_uv.rng_th[1] = 10;
	ife2_refcen_info.refcent_uv.rng_th[2] = 15;
	ife2_refcen_info.refcent_uv.rng_wt[0] = 8;
	ife2_refcen_info.refcent_uv.rng_wt[1] = 4;
	ife2_refcen_info.refcent_uv.rng_wt[2] = 2;
	ife2_refcen_info.refcent_uv.rng_wt[3] = 1;
	ife2_refcen_info.refcent_uv.cent_wt = 31;
	ife2_refcen_info.refcent_uv.outl_dth = 255;
	ife2_refcen_info.refcent_uv.outl_th = 7;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_REFCENT, (void *)&ife2_refcen_info);


	ife2_filter_info.set_y.filt_th[0] = 6;
	ife2_filter_info.set_y.filt_th[1] = 12;
	ife2_filter_info.set_y.filt_th[2] = 18;
	ife2_filter_info.set_y.filt_th[3] = 24;
	ife2_filter_info.set_y.filt_th[4] = 30;

	ife2_filter_info.set_y.filt_wt[0] = 16;
	ife2_filter_info.set_y.filt_wt[1] = 8;
	ife2_filter_info.set_y.filt_wt[2] = 4;
	ife2_filter_info.set_y.filt_wt[3] = 2;
	ife2_filter_info.set_y.filt_wt[4] = 1;
	ife2_filter_info.set_y.filt_wt[5] = 0;

	ife2_filter_info.set_u.filt_th[0] = 5;
	ife2_filter_info.set_u.filt_th[1] = 10;
	ife2_filter_info.set_u.filt_th[2] = 15;
	ife2_filter_info.set_u.filt_th[3] = 20;
	ife2_filter_info.set_u.filt_th[4] = 25;

	ife2_filter_info.set_u.filt_wt[0] = 16;
	ife2_filter_info.set_u.filt_wt[1] = 8;
	ife2_filter_info.set_u.filt_wt[2] = 4;
	ife2_filter_info.set_u.filt_wt[3] = 2;
	ife2_filter_info.set_u.filt_wt[4] = 1;
	ife2_filter_info.set_u.filt_wt[5] = 0;

	ife2_filter_info.set_v.filt_th[0] = 5;
	ife2_filter_info.set_v.filt_th[1] = 10;
	ife2_filter_info.set_v.filt_th[2] = 15;
	ife2_filter_info.set_v.filt_th[3] = 20;
	ife2_filter_info.set_v.filt_th[4] = 25;

	ife2_filter_info.set_v.filt_wt[0] = 16;
	ife2_filter_info.set_v.filt_wt[1] = 8;
	ife2_filter_info.set_v.filt_wt[2] = 4;
	ife2_filter_info.set_v.filt_wt[3] = 2;
	ife2_filter_info.set_v.filt_wt[4] = 1;
	ife2_filter_info.set_v.filt_wt[5] = 0;

	ife2_filter_info.enable = FALSE;
	ife2_filter_info.size = KDRV_IFE2_FLTR_SIZE_9x9;
	ife2_filter_info.edg_dir.hv_th = 24;
	ife2_filter_info.edg_dir.pn_th = 24;
	ife2_filter_info.edg_ker_size = KDRV_IFE2_EKNL_SIZE_3x3;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_FILTER, (void *)&ife2_filter_info);

	return 0;
}


int nvt_kdrv_ipp_ife2_set_load_param(UINT32 id, nvt_kdrv_ipp_ife2_info *p_set_param, UINT32 fnum)
{
	if (fnum == 4) {
		ife2_in_img_info.ch.width = p_set_param->in_width;
		ife2_in_img_info.ch.height = p_set_param->in_height;
		ife2_in_img_info.ch.line_ofs = p_set_param->in_lofs;
		ife2_in_img_info.type = KDRV_IFE2_IN_FMT_PACK_YUV444;
		kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_IMG_INFO, (void *)&ife2_in_img_info);

		kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_LOAD, NULL);
	}



	return 0;
}



//----------------------------------------------------------------------------------

static UINT32 ime_interrupt_en_info;
static KDRV_IME_IN_INFO ime_in_img_info;
//static KDRV_IME_DMA_ADDR_INFO ime_in_addr;
static KDRV_IME_OUT_PATH_IMG_INFO ime_out_img_info[2];
static KDRV_IME_OUT_PATH_ADDR_INFO ime_out_addr;
static KDRV_IME_EXTEND_INFO ime_extend_info = {0};
static KDRV_IME_TMNR_REF_OUT_IMG_INFO ime_refout_img_info;
static KDRV_IME_LCA_SUBOUT_INFO ime_lcaout_img_info;

static KDRV_IME_LCA_IMG_INFO ime_lcain_img_info;
static KDRV_IME_LCA_PARAM ime_lca_set_param;
static KDRV_IME_LCA_CTRL_PARAM ime_lca_ctrl_param;
static KDRV_IME_PARAM_IPL_FUNC_EN ime_ipl_func_param;

static KDRV_IME_TMNR_BUF_SIZE_INFO ime_tmnr_buf_info;


static KDRV_IME_TMNR_REF_IN_IMG_INFO ime_tmnr_set_in_img_info;
static KDRV_IME_TMNR_MOTION_STATUS_INFO ime_tmnr_set_ms;
static KDRV_IME_TMNR_MOTION_VECTOR_INFO ime_tmnr_set_mv;
static KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO ime_tmnr_set_ms_roi;
static KDRV_IME_TMNR_STATISTIC_DATA_INFO ime_tmnr_set_sta;

static KDRV_IME_TMNR_PARAM ime_tmnr_set_iq_param = {
	TRUE,
	{
		// KDRV_IME_TMNR_ME_PARAM          me_param
		KDRV_ME_UPDATE_RAND, TRUE, KDRV_MV_DOWN_SAMPLE_AVERAGE, KDRV_ME_SDA_TYPE_COMPENSATED,
		0, 3, 3, 16, 2048,
		{0x80, 0x80, 0x0, 0x100, 0x100, 0x100, 0x200, 0x200},
		{0xA, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF},
		0x8c,
		{0x4, 0x4, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4},
		{0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0},
	},
	{
		// KDRV_IME_TMNR_MD_PARAM          md_param
		{0x18, 0x18, 0x16, 0x17, 0x16, 0x16, 0x16, 0x16},
		{0x26, 0x10, 0x38, 0x12, 0x1C, 0x25, 0x25, 0x25},
		{0x3D, 0x4F, 0x53, 0x5A, 0x4D, 0x52, 0x52, 0x52},
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MD_ROI_PARAM      md_roi_param
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MC_PARAM          mc_param
		{0x16E, 0x20C, 0x2EE, 0x314, 0x2E5, 0x2F2, 0x2F2, 0x2F2},
		{0x18, 0x18, 0x16, 0x17, 0x16, 0x16, 0x16, 0x16},
		{0x26, 0x10, 0x38, 0x12, 0x1C, 0x25, 0x25, 0x25},
		{0x3D, 0x4F, 0x53, 0x5A, 0x4D, 0x52, 0x52, 0x52},
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MC_ROI_PARAM      mc_roi_param
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_PS_PARAM          ps_param
		TRUE, TRUE, TRUE, KDRV_PS_MODE_0, KDRV_MV_INFO_MODE_AVERAGE,
		0x8, 0x8,
		{0xb4, 0xe6},
		{0x200, 0x400},
		//{0x168, 0x96},//removed because of PQ_NEW definition
		0x6, 0x6, 0x0,
		{0xc0, 0x180},
		//0x154, //removed because of PQ_NEW definition
		0x800,
	},
	{
		// KDRV_IME_TMNR_NR_PARAM          nr_param
		TRUE, TRUE,
		0x1, 0x0, 0x80, 0x1, 0x1,
		{0x10, 0x10, 0x10, 0x10},
		{0x30, 0x20, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
		KDRV_PRE_FILTER_TYPE_B,
		{0x50, 0x5a, 0x64, 0x96},
		{0x40, 0x20},
		{0x8, 0x20, 0x20},
		{0xff, 0x20, 0x10},
		0x1f4, 0x640,
		{0x8, 0x10, 0x18, 0x20},
		{0x0, 0x4, 0xA, 0x14, 0x20, 0x2E, 0x40, 0x5A},
		{0x0, 0x4, 0xA, 0x14, 0x20, 0x2E, 0x40, 0x5A},
		{0x80, 0x40},
		FALSE,
	},
	//{
	//  // KDRV_IME_TMNR_DBG_PARAM   dbg_param
	//  FALSE, 0
	//},
	//{
	//  // KDRV_IME_TMNR_STATISTIC_PARAM   sta_param
	//  0x8, 0x4, 0x3c, 0x44, 0x0, 0x0
	//},
};

static KDRV_IME_TMNR_DBG_PARAM ime_tmnr_set_dbg_param = {
	FALSE, 0
};

IPP_BUFF_INFO ime_out_buf_p1_y = {0}, ime_out_buf_p1_u = {0}, ime_out_buf_p1_v = {0};
IPP_BUFF_INFO ime_out_buf_p2_y = {0}, ime_out_buf_p2_u = {0}, ime_out_buf_p2_v = {0};


static int out_size_p1_h = 0, out_size_p1_v = 0;
static int out_lofs_p1_y = 0, out_lofs_p1_uv = 0;
static UINT32 out_addr_p1_y, out_addr_p1_u, out_addr_p1_v;
static UINT32 out_buf_p1_size_y = 0, out_buf_p1_size_u = 0, out_buf_p1_size_v = 0;

static int out_size_p2_h = 0, out_size_p2_v = 0;
static int out_lofs_p2_y = 0, out_lofs_p2_uv = 0;
static UINT32 out_addr_p2_y, out_addr_p2_u, out_addr_p2_v;
static UINT32 out_buf_p2_size_y = 0, out_buf_p2_size_u = 0, out_buf_p2_size_v = 0;


IPP_BUFF_INFO out_buf_refout_y0 = {0}, out_buf_refout_u0 = {0};
IPP_BUFF_INFO out_buf_refout_y1 = {0}, out_buf_refout_u1 = {0};


IPP_BUFF_INFO out_buf_tmnr_mv0 = {0}, out_buf_tmnr_mv1 = {0};
IPP_BUFF_INFO out_buf_tmnr_ms0 = {0}, out_buf_tmnr_ms1 = {0};
IPP_BUFF_INFO out_buf_tmnr_ms_roi0 = {0}, out_buf_tmnr_ms_roi1 = {0};
IPP_BUFF_INFO out_buf_tmnr_sta0 = {0}, out_buf_tmnr_sta1 = {0};


//static int out_size_refout_h = 0, out_size_refout_v = 0;
static UINT32 out_lofs_refout_y, out_lofs_refout_uv;
static UINT32 out_addr_refout_y0, out_addr_refout_u0;
static UINT32 out_addr_refout_y1, out_addr_refout_u1;
static UINT32 out_buf_refout_size_y, out_buf_refout_size_u;

static UINT32 out_lofs_mv, out_lofs_ms, out_lofs_ms_roi, out_lofs_sta;
static UINT32 out_size_mv, out_size_ms, out_size_ms_roi, out_size_sta;
static UINT32 out_addr_mv0, out_addr_mv1;
static UINT32 out_addr_ms0, out_addr_ms1;
static UINT32 out_addr_ms_roi0, out_addr_ms_roi1;
static UINT32 out_addr_sta0, out_addr_sta1;
//static int in_lofs_refin_y = 0, in_lofs_refin_uv = 0;
//static UINT32 in_addr_refin_y0, in_addr_refin_u0;
//static UINT32 in_addr_refin_y1, in_addr_refin_u1;
//static UINT32 in_buf_refin_size_y = 0, in_buf_refin_size_u = 0;

IPP_BUFF_INFO out_buf0_lca_y = {0}, out_buf1_lca_y = {0};
void *hdl_out_buf0_lca_y = NULL, *hdl_out_buf1_lca_y = NULL;

static int out_lofs_lca_y = 0;
static UINT32 out_addr0_lca_y, out_addr1_lca_y;
static UINT32 out_buf_lca_size_y = 0;




char ime_out_p1_raw_file_y[260] = "/mnt/sd/ime/output/img_out_p1_y.raw";
char ime_out_p1_raw_file_u[260] = "/mnt/sd/ime/output/img_out_p1_u.raw";
char ime_out_p1_raw_file_v[260] = "/mnt/sd/ime/output/img_out_p1_v.raw";

char ime_out_p2_raw_file_y[260] = "/mnt/sd/ime/output/img_out_p2_y.raw";
char ime_out_p2_raw_file_u[260] = "/mnt/sd/ime/output/img_out_p2_u.raw";
char ime_out_p2_raw_file_v[260] = "/mnt/sd/ime/output/img_out_p2_v.raw";

char ime_out_refout_raw_file_y[260] = "/mnt/sd/ime/output/img_out_refout_y.raw";
char ime_out_refout_raw_file_u[260] = "/mnt/sd/ime/output/img_out_refout_u.raw";

char ime_out_lcaout_raw_file_y[260] = "/mnt/sd/ime/output/img_out_lcaout_y.raw";


#if 0
static int nvt_kdrv_ime_wait_frame_start(BOOL clear_flag)
{
	/*wait end*/
	while (!nvt_kdrv_ime_fstart_flag) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		vos_task_delay_ms(1);
#else
		msleep(1);
#endif

		//DBG_IND("nvt_kdrv_ife_fend_flag: %d\r\n", nvt_kdrv_ife_fend_flag);
	}

	if (clear_flag == TRUE) {
		nvt_kdrv_ime_fstart_flag = 0;
	}

	return 0;
}
#endif



int nvt_kdrv_ipp_ime_set_param(UINT32 id, nvt_kdrv_ipp_ime_info *p_set_param)
{
	out_size_p1_h = p_set_param->out_p1_size_h;
	out_size_p1_v = p_set_param->out_p1_size_v;
	out_lofs_p1_y = p_set_param->out_p1_lofs_y;
	out_lofs_p1_uv = p_set_param->out_p1_lofs_uv;

	DBG_IND("out_size_p1_h %d\r\n", out_size_p1_h);
	DBG_IND("out_size_p1_v %d\r\n", out_size_p1_v);
	DBG_IND("out_lofs_p1_y %d\r\n", out_lofs_p1_y);
	DBG_IND("out_lofs_p1_uv %d\r\n", out_lofs_p1_uv);

#if 0
	out_size_p2_h = p_set_param->out_p2_size_h;
	out_size_p2_v = p_set_param->out_p2_size_v;
	out_lofs_p2_y = p_set_param->out_p2_lofs_y;
	out_lofs_p2_uv = p_set_param->out_p2_lofs_uv;

	DBG_IND("out_size_p2_h %d\r\n", out_size_p2_h);
	DBG_IND("out_size_p2_v %d\r\n", out_size_p2_v);
	DBG_IND("out_lofs_p2_y %d\r\n", out_lofs_p2_y);
	DBG_IND("out_lofs_p2_uv %d\r\n", out_lofs_p2_uv);
#endif

	// set interrupt
	ime_interrupt_en_info = (KDRV_IME_INTE_FRM_START | KDRV_IME_INTE_FRM_END);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_INTER, (void *)&ime_interrupt_en_info);


	//set input img parameters
	ime_in_img_info.in_src = p_set_param->ime_op_mode;
	ime_in_img_info.type = KDRV_IME_IN_FMT_YUV420;
	ime_in_img_info.ch[0].width = p_set_param->in_width;
	ime_in_img_info.ch[0].height = p_set_param->in_height;
	ime_in_img_info.ch[0].line_ofs = p_set_param->in_width;
	ime_in_img_info.ch[1].line_ofs = p_set_param->in_width;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&ime_in_img_info);



	ime_extend_info.stripe_num = 1;     ///<stripe window number
	ime_extend_info.stripe_h_max = p_set_param->in_width;   ///<max horizontal stripe size
	ime_extend_info.tmnr_en = 0;       ///<3dnr enable
	ime_extend_info.p1_enc_en = 0;      ///<output path1 encoder enable
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_extend_info);


	//set input address
	//in_addr.addr_y  = (UINT32)in_buf_y.vaddr;
	//in_addr.addr_cb = (UINT32)in_buf_u.vaddr;
	//in_addr.addr_cr = (UINT32)in_buf_v.vaddr;
	//kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_ADDR, (void *)&in_addr);

	//set output path parameters
	//for (i = 0; i < IME_EXAM_OUT_MAX; i ++) {
	{
		ime_out_img_info[0].path_id = KDRV_IME_PATH_ID_1;
		ime_out_img_info[0].path_en = TRUE;
		ime_out_img_info[0].path_flip_en = FALSE;
		ime_out_img_info[0].type = KDRV_IME_OUT_FMT_Y_PACK_UV420;//Out_Data[i].fmt;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].width = out_size_p1_h;//Out_Data[i].SizeH;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].height = out_size_p1_v;//Out_Data[i].SizeV;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].line_ofs = out_lofs_p1_y;//Out_Data[i].LineOfs;

		//ime_out_img_info[0].ch[1] = IPL_UTI_Y_INFO_CONV2_UV(Out_Data[i].fmt, ime_out_img_info[0].ch[0]);

		ime_out_img_info[0].ch[KDRV_IPP_YUV_U].width = out_size_p1_h;//Out_Data[i].SizeH;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_U].height = out_size_p1_v;//Out_Data[i].SizeV;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_U].line_ofs = out_lofs_p1_uv;//Out_Data[i].LineOfs;

		ime_out_img_info[0].ch[KDRV_IPP_YUV_V].width = out_size_p1_h;//Out_Data[i].SizeH;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_V].height = out_size_p1_v;//Out_Data[i].SizeV;
		ime_out_img_info[0].ch[KDRV_IPP_YUV_V].line_ofs = out_lofs_p1_uv;//Out_Data[i].LineOfs;

		ime_out_img_info[0].crp_window.x = 0;
		ime_out_img_info[0].crp_window.y = 0;
		ime_out_img_info[0].crp_window.w = out_size_p1_h;//Out_Data[i].SizeH;
		ime_out_img_info[0].crp_window.h = out_size_p1_v;//Out_Data[i].SizeV;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&ime_out_img_info[0]);

		//set output path1 address
		// output buffer
		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_RGB:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = (out_lofs_p1_uv * out_size_p1_v);

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)ime_out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = (out_lofs_p1_uv * out_size_p1_v);

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)ime_out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = (out_lofs_p1_uv * (out_size_p1_v >> 1));

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)ime_out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)ime_out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p1_size_y = (out_lofs_p1_y * (out_size_p1_v >> 1));
			out_buf_p1_size_u = 0;
			out_buf_p1_size_v = 0;

			//out_addr_p1_y = (UINT32)ime_out_buf_p1_y.va_addr;
			//out_addr_p1_u = out_addr_p1_y;
			//out_addr_p1_v = out_addr_p1_y;
			break;

		default:
			break;
		}

		ime_out_addr.path_id = KDRV_IME_PATH_ID_1;

		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p1_size_y != 0) {
				//
#if 0
				ime_out_buf_p1_y.size = out_buf_p1_size_y;
				ime_out_buf_p1_y.align = 64;     ///< address alignment
				ime_out_buf_p1_y.name = "ime_out_buf_p1_y";
				ime_out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_y, out_buf_p1_size_y) == E_OK) {
					out_addr_p1_y = (UINT32)ime_out_buf_p1_y.vaddr;
				}
				DBG_IND("ime_out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_y.vaddr);
			}

			if (out_buf_p1_size_u != 0) {
				//
#if 0
				ime_out_buf_p1_u.size = out_buf_p1_size_u;
				ime_out_buf_p1_u.align = 64;     ///< address alignment
				ime_out_buf_p1_u.name = "ime_out_buf_p1_u";
				ime_out_buf_p1_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_u);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_u, out_buf_p1_size_u) == E_OK) {
					out_addr_p1_u = (UINT32)ime_out_buf_p1_u.vaddr;
				}
				DBG_IND("ime_out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_u.vaddr);
			}

			if (out_buf_p1_size_v != 0) {
				//
#if 0
				ime_out_buf_p1_v.size = out_buf_p1_size_v;
				ime_out_buf_p1_v.align = 64;     ///< address alignment
				ime_out_buf_p1_v.name = "ime_out_buf_p1_v";
				ime_out_buf_p1_v.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_v);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_v, out_buf_p1_size_v) == E_OK) {
					out_addr_p1_v = (UINT32)ime_out_buf_p1_v.vaddr;
				}
				DBG_IND("ime_out_buf_p1_v.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_v.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p1_size_y != 0) {
				//
#if 0
				ime_out_buf_p1_y.size = out_buf_p1_size_y;
				ime_out_buf_p1_y.align = 64;     ///< address alignment
				ime_out_buf_p1_y.name = "ime_out_buf_p1_y";
				ime_out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_y, out_buf_p1_size_y) == E_OK) {
					out_addr_p1_y = (UINT32)ime_out_buf_p1_y.vaddr;
				}
				DBG_IND("ime_out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_y.vaddr);
			}

			if (out_buf_p1_size_u != 0) {
				//
#if 0
				ime_out_buf_p1_u.size = out_buf_p1_size_u;
				ime_out_buf_p1_u.align = 64;     ///< address alignment
				ime_out_buf_p1_u.name = "ime_out_buf_p1_u";
				ime_out_buf_p1_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_u);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_u, out_buf_p1_size_u) == E_OK) {
					out_addr_p1_u = (UINT32)ime_out_buf_p1_u.vaddr;
				}
				DBG_IND("ime_out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_u.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p1_size_y != 0) {
				//
#if 0
				ime_out_buf_p1_y.size = out_buf_p1_size_y;
				ime_out_buf_p1_y.align = 64;     ///< address alignment
				ime_out_buf_p1_y.name = "ime_out_buf_p1_y";
				ime_out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p1_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p1_y, out_buf_p1_size_y) == E_OK) {
					out_addr_p1_y = (UINT32)ime_out_buf_p1_y.vaddr;
				}
				DBG_IND("ime_out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p1_y.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_y;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&ime_out_addr);
	}

#if 0
	{
		ime_out_img_info[1].path_id = KDRV_IME_PATH_ID_2;
		ime_out_img_info[1].path_en = TRUE;
		ime_out_img_info[1].path_flip_en = FALSE;
		ime_out_img_info[1].type = KDRV_IME_OUT_FMT_Y_PACK_UV420;//Out_Data[i].fmt;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].width = out_size_p2_h;//Out_Data[i].SizeH;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].height = out_size_p2_v;//Out_Data[i].SizeV;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].line_ofs = out_lofs_p2_y;//Out_Data[i].LineOfs;

		//ime_out_img_info[1].ch[1] = IPL_UTI_Y_INFO_CONV2_UV(Out_Data[i].fmt, ime_out_img_info[1].ch[0]);

		ime_out_img_info[1].ch[KDRV_IPP_YUV_U].width = out_size_p2_h;//Out_Data[i].SizeH;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_U].height = out_size_p2_v;//Out_Data[i].SizeV;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_U].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

		ime_out_img_info[1].ch[KDRV_IPP_YUV_V].width = out_size_p2_h;//Out_Data[i].SizeH;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_V].height = out_size_p2_v;//Out_Data[i].SizeV;
		ime_out_img_info[1].ch[KDRV_IPP_YUV_V].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

		ime_out_img_info[1].crp_window.x = 0;
		ime_out_img_info[1].crp_window.y = 0;
		ime_out_img_info[1].crp_window.w = out_size_p2_h;//Out_Data[i].SizeH;
		ime_out_img_info[1].crp_window.h = out_size_p2_v;//Out_Data[i].SizeV;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&ime_out_img_info[1]);

		//set output path2 address
		// output buffer
		switch (ime_out_img_info[1].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_RGB:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = (out_lofs_p2_uv * out_size_p2_v);

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = (out_lofs_p2_uv * out_size_p2_v);

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = (out_lofs_p2_uv * (out_size_p2_v >> 1));

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p2_size_y = (out_lofs_p2_y * (out_size_p2_v >> 1));
			out_buf_p2_size_u = 0;
			out_buf_p2_size_v = 0;

			//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
			//out_addr_p2_u = out_addr_p2_y;
			//out_addr_p2_v = out_addr_p2_y;
			break;

		default:
			break;
		}

		ime_out_addr.path_id = KDRV_IME_PATH_ID_2;

		switch (ime_out_img_info[1].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p2_size_y != 0) {
				//
#if 0
				ime_out_buf_p2_y.size = out_buf_p2_size_y;
				ime_out_buf_p2_y.align = 64;     ///< address alignment
				ime_out_buf_p2_y.name = "ime_out_buf_p2_y";
				ime_out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_y, out_buf_p2_size_y) == E_OK) {
					out_addr_p2_y = (UINT32)ime_out_buf_p2_y.vaddr;
				}
				DBG_IND("ime_out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_y.vaddr);
			}

			if (out_buf_p2_size_u != 0) {
				//
#if 0
				ime_out_buf_p2_u.size = out_buf_p2_size_u;
				ime_out_buf_p2_u.align = 64;     ///< address alignment
				ime_out_buf_p2_u.name = "ime_out_buf_p2_u";
				ime_out_buf_p2_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_u);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_u, out_buf_p2_size_u) == E_OK) {
					out_addr_p2_u = (UINT32)ime_out_buf_p2_u.vaddr;
				}
				DBG_IND("ime_out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_u.vaddr);
			}

			if (out_buf_p2_size_v != 0) {
				//
#if 0
				ime_out_buf_p2_v.size = out_buf_p2_size_v;
				ime_out_buf_p2_v.align = 64;     ///< address alignment
				ime_out_buf_p2_v.name = "ime_out_buf_p2_v";
				ime_out_buf_p2_v.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_v);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_v, out_buf_p2_size_v) == E_OK) {
					out_addr_p2_v = (UINT32)ime_out_buf_p2_v.vaddr;
				}
				DBG_IND("ime_out_buf_p2_v.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_v.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p2_size_y != 0) {
				//
#if 0
				ime_out_buf_p2_y.size = out_buf_p2_size_y;
				ime_out_buf_p2_y.align = 64;     ///< address alignment
				ime_out_buf_p2_y.name = "ime_out_buf_p2_y";
				ime_out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_y, out_buf_p2_size_y) == E_OK) {
					out_addr_p2_y = (UINT32)ime_out_buf_p2_y.vaddr;
				}
				DBG_IND("ime_out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_y.vaddr);
			}

			if (out_buf_p2_size_u != 0) {
				//
#if 0
				ime_out_buf_p2_u.size = out_buf_p2_size_u;
				ime_out_buf_p2_u.align = 64;     ///< address alignment
				ime_out_buf_p2_u.name = "ime_out_buf_p2_u";
				ime_out_buf_p2_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_u);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_u, out_buf_p2_size_u) == E_OK) {
					out_addr_p2_u = (UINT32)ime_out_buf_p2_u.vaddr;
				}
				DBG_IND("ime_out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_u.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p2_size_y != 0) {
				//
#if 0
				ime_out_buf_p2_y.size = out_buf_p2_size_y;
				ime_out_buf_p2_y.align = 64;     ///< address alignment
				ime_out_buf_p2_y.name = "ime_out_buf_p2_y";
				ime_out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &ime_out_buf_p2_y);
#endif
				if (kdrv_ipp_alloc_buff(&ime_out_buf_p2_y, out_buf_p2_size_y) == E_OK) {
					out_addr_p2_y = (UINT32)ime_out_buf_p2_y.vaddr;
				}
				DBG_IND("ime_out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)ime_out_buf_p2_y.vaddr);
			}

			ime_out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_y;
			ime_out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&ime_out_addr);
	}
#endif

	{
		// set lca subout
		ime_lcaout_img_info.enable = FALSE;

//#if (KDRV_IME_PQ_NEW == 0)
//		ime_lcaout_img_info.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
//#endif
		ime_lcaout_img_info.sub_out_size.width = p_set_param->out_lca_size_h;
		ime_lcaout_img_info.sub_out_size.height = p_set_param->out_lca_size_v;
		ime_lcaout_img_info.sub_out_size.line_ofs = p_set_param->out_lca_lofs_y;

		out_lofs_lca_y = ime_lcaout_img_info.sub_out_size.line_ofs;
		out_buf_lca_size_y = (out_lofs_lca_y * ime_lcaout_img_info.sub_out_size.height);

		if (ime_lcaout_img_info.enable == TRUE) {
			if (kdrv_ipp_alloc_buff(&out_buf0_lca_y, out_buf_lca_size_y) == E_OK) {
				out_addr0_lca_y = (UINT32)out_buf0_lca_y.vaddr;
			}
			DBG_IND("out_buf0_lca_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr0_lca_y);

			if (kdrv_ipp_alloc_buff(&out_buf1_lca_y, out_buf_lca_size_y) == E_OK) {
				out_addr1_lca_y = (UINT32)out_buf1_lca_y.vaddr;
			}
			DBG_IND("out_buf1_lca_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr1_lca_y);
		}

		ime_lcaout_img_info.sub_out_addr = out_addr1_lca_y;

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_SUB, (void *)&ime_lcaout_img_info);
	}


	{
		ime_ipl_func_param.ipl_ctrl_func = 0;
		ime_ipl_func_param.enable = FALSE;

		// lca subin
		ime_lcain_img_info.in_src = p_set_param->lca_in_src;
		ime_lcain_img_info.img_size.width = p_set_param->out_lca_size_h;
		ime_lcain_img_info.img_size.height = p_set_param->out_lca_size_v;
		ime_lcain_img_info.img_size.line_ofs = p_set_param->out_lca_lofs_y;
		ime_lcain_img_info.dma_addr = out_addr0_lca_y;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_REF_IMG, (void *)&ime_lcain_img_info);


		ime_ipl_func_param.ipl_ctrl_func |= KDRV_IME_IQ_FUNC_LCA;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, &ime_ipl_func_param);


		ime_lca_ctrl_param.enable = FALSE;
		ime_lca_ctrl_param.bypass = FALSE;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_FUNC_EN, &ime_lca_ctrl_param);

		ime_lca_set_param_enable = FALSE;
		ime_lca_set_param.chroma.ref_y_wt = 16;   ///< Chroma reference weighting for Y channels
		ime_lca_set_param.chroma.ref_uv_wt = 16;  ///< Chroma reference weighting for UV channels
		ime_lca_set_param.chroma.out_uv_wt = 16;  ///< Chroma adaptation output weighting
		ime_lca_set_param.chroma.y_rng = KDRV_IME_RANGE_8;
		ime_lca_set_param.chroma.y_wt_prc = KDRV_IME_RANGE_8;
		ime_lca_set_param.chroma.y_th = 0;
		ime_lca_set_param.chroma.y_wt_s = 8;
		ime_lca_set_param.chroma.y_wt_e = 0;

		ime_lca_set_param.chroma.uv_rng = KDRV_IME_RANGE_16;
		ime_lca_set_param.chroma.uv_wt_prc = KDRV_IME_RANGE_16;
		ime_lca_set_param.chroma.uv_th = 0;
		ime_lca_set_param.chroma.uv_wt_s = 16;
		ime_lca_set_param.chroma.uv_wt_e = 0;

		ime_lca_set_param.ca.enable = FALSE;
		ime_lca_set_param.luma.enable = FALSE;

#if (KDRV_IME_PQ_NEW == 1)
		ime_lca_set_param.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
#endif

		kdrv_ime_set(id, KDRV_IME_PARAM_IQ_LCA, (void *)&ime_lca_set_param);
	}


#if 0
	{
		// set 3dnr ref-out
		ime_refout_img_info.enable = FALSE;
		ime_refout_img_info.encode.enable = FALSE;
		ime_refout_img_info.flip_en = FALSE;
		ime_refout_img_info.img_line_ofs_y = p_set_param->out_ref_lofs_y;
		ime_refout_img_info.img_line_ofs_cb = p_set_param->out_ref_lofs_uv;

		out_lofs_refout_y = ime_refout_img_info.img_line_ofs_y;
		out_lofs_refout_uv = ime_refout_img_info.img_line_ofs_cb;
		out_buf_refout_size_y = (out_lofs_refout_y * p_set_param->in_height);
		out_buf_refout_size_u = (out_lofs_refout_uv * (p_set_param->in_height >> 1));

		if (ime_refout_img_info.enable == TRUE) {
			if (kdrv_ipp_alloc_buff(&out_buf_refout_y0, out_buf_refout_size_y) == E_OK) {
				out_addr_refout_y0 = (UINT32)out_buf_refout_y0.vaddr;
			}
			DBG_IND("out_buf_refout_y0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_refout_y0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_refout_u0, out_buf_refout_size_u) == E_OK) {
				out_addr_refout_u0 = (UINT32)out_buf_refout_u0.vaddr;
			}
			DBG_IND("out_buf_refout_u0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_refout_u0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_refout_y1, out_buf_refout_size_y) == E_OK) {
				out_addr_refout_y1 = (UINT32)out_buf_refout_y1.vaddr;
			}
			DBG_IND("out_buf_refout_y1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_refout_y1.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_refout_u1, out_buf_refout_size_u) == E_OK) {
				out_addr_refout_u1 = (UINT32)out_buf_refout_u1.vaddr;
			}
			DBG_IND("out_buf_refout_u1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_refout_u1.vaddr);

			ime_refout_img_info.img_addr_y = out_addr_refout_y0;
			ime_refout_img_info.img_addr_cb = out_addr_refout_u0;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG, (void *)&ime_refout_img_info);
	}
#endif

	// 3dnr
	{
		// get 3dnr buffer size for MV / MS
		ime_tmnr_buf_info.in_size_h = p_set_param->in_width;
		ime_tmnr_buf_info.in_size_v = p_set_param->in_height;
		ime_tmnr_buf_info.in_sta_max_num = 8192;

		kdrv_ime_get(id, KDRV_IME_PARAM_IPL_TMNR_BUF_SIZE, &ime_tmnr_buf_info);

		DBG_IND("ime_tmnr_buf_info.get_mv_lofs = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_mv_lofs);
		DBG_IND("ime_tmnr_buf_info.get_mv_size = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_mv_size);

		DBG_IND("ime_tmnr_buf_info.get_ms_lofs = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_ms_lofs);
		DBG_IND("ime_tmnr_buf_info.get_ms_size = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_ms_size);

		DBG_IND("ime_tmnr_buf_info.get_ms_roi_lofs = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_ms_roi_lofs);
		DBG_IND("ime_tmnr_buf_info.get_ms_roi_size = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_ms_roi_size);

		DBG_IND("ime_tmnr_buf_info.get_sta_lofs = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_sta_lofs);
		DBG_IND("ime_tmnr_buf_info.get_sta_size = 0x%08x\r\n", (unsigned int)ime_tmnr_buf_info.get_sta_size);



		if (1) {
			// mv
			out_lofs_mv = ime_tmnr_buf_info.get_mv_lofs;
			out_size_mv = ime_tmnr_buf_info.get_mv_size;
			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_mv0, out_size_mv) == E_OK) {
				out_addr_mv0 = (UINT32)out_buf_tmnr_mv0.vaddr;
			}
			memset((void *)out_addr_mv0, 0x0, out_size_mv);
			DBG_IND("out_buf_tmnr_mv0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_mv0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_mv1, out_size_mv) == E_OK) {
				out_addr_mv1 = (UINT32)out_buf_tmnr_mv1.vaddr;
			}
			memset((void *)out_addr_mv1, 0x0, out_size_mv);
			DBG_IND("out_buf_tmnr_mv1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_mv1.vaddr);

			// ms
			out_lofs_ms = ime_tmnr_buf_info.get_ms_lofs;
			out_size_ms = ime_tmnr_buf_info.get_ms_size;
			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_ms0, out_size_ms) == E_OK) {
				out_addr_ms0 = (UINT32)out_buf_tmnr_ms0.vaddr;
			}
			memset((void *)out_addr_ms0, 0xff, out_size_ms);
			DBG_IND("out_buf_tmnr_ms0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_ms0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_ms1, out_size_ms) == E_OK) {
				out_addr_ms1 = (UINT32)out_buf_tmnr_ms1.vaddr;
			}
			memset((void *)out_addr_ms1, 0xff, out_size_ms);
			DBG_IND("out_buf_tmnr_ms1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_ms1.vaddr);

			// ms_roi-roi
			out_lofs_ms_roi = ime_tmnr_buf_info.get_ms_roi_lofs;
			out_size_ms_roi = ime_tmnr_buf_info.get_ms_roi_size;
			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_ms_roi0, out_size_ms_roi) == E_OK) {
				out_addr_ms_roi0 = (UINT32)out_buf_tmnr_ms_roi0.vaddr;
			}
			DBG_IND("out_buf_tmnr_ms_roi0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_ms_roi0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_ms_roi1, out_size_ms_roi) == E_OK) {
				out_addr_ms_roi1 = (UINT32)out_buf_tmnr_ms_roi1.vaddr;
			}
			DBG_IND("out_buf_tmnr_ms_roi1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_ms_roi1.vaddr);

			// sta
			out_lofs_sta = ime_tmnr_buf_info.get_sta_lofs;
			out_size_sta = ime_tmnr_buf_info.get_sta_size;
			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_sta0, out_size_sta) == E_OK) {
				out_addr_sta0 = (UINT32)out_buf_tmnr_sta0.vaddr;
			}
			DBG_IND("out_buf_tmnr_sta0.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_sta0.vaddr);

			if (kdrv_ipp_alloc_buff(&out_buf_tmnr_sta1, out_size_sta) == E_OK) {
				out_addr_sta1 = (UINT32)out_buf_tmnr_sta1.vaddr;
			}
			DBG_IND("out_buf_tmnr_sta1.vaddr = 0x%08x\r\n", (unsigned int)out_buf_tmnr_sta1.vaddr);
		}
	}



	return 0;
}

int nvt_kdrv_ipp_ime_set_load_param(UINT32 id, nvt_kdrv_ipp_ime_info *p_set_param, UINT32 fnum)
{
	ime_ipl_func_param.ipl_ctrl_func = 0;
	ime_ipl_func_param.enable = TRUE;

	//set input img parameters
	ime_in_img_info.in_src = p_set_param->ime_op_mode;
	ime_in_img_info.type = KDRV_IME_IN_FMT_YUV420;
	ime_in_img_info.ch[0].width = p_set_param->in_width;
	ime_in_img_info.ch[0].height = p_set_param->in_height;
	ime_in_img_info.ch[0].line_ofs = p_set_param->in_width;
	ime_in_img_info.ch[1].line_ofs = p_set_param->in_width;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&ime_in_img_info);

	ime_extend_info.stripe_num = 1;     ///<stripe window number
	ime_extend_info.stripe_h_max = p_set_param->in_width;   ///<max horizontal stripe size
	ime_extend_info.tmnr_en = 0;       ///<3dnr enable
	ime_extend_info.p1_enc_en = 0;      ///<output path1 encoder enable
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_extend_info);
	// lca subout
	if (fnum % 2 == 0) {
		ime_lcaout_img_info.enable = TRUE;
//#if (KDRV_IME_PQ_NEW == 0)
//		ime_lcaout_img_info.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
//#endif
		ime_lcaout_img_info.sub_out_size.width = p_set_param->out_lca_size_h;
		ime_lcaout_img_info.sub_out_size.height = p_set_param->out_lca_size_v;
		ime_lcaout_img_info.sub_out_size.line_ofs = p_set_param->out_lca_lofs_y;

		out_lofs_lca_y = ime_lcaout_img_info.sub_out_size.line_ofs;
		out_buf_lca_size_y = (out_lofs_lca_y * ime_lcaout_img_info.sub_out_size.height);

		ime_lcaout_img_info.sub_out_addr = out_addr1_lca_y;

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_SUB, (void *)&ime_lcaout_img_info);
	} else {
		ime_lcaout_img_info.enable = TRUE;
//#if (KDRV_IME_PQ_NEW == 0)
//		ime_lcaout_img_info.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
//#endif
		ime_lcaout_img_info.sub_out_size.width = p_set_param->out_lca_size_h;
		ime_lcaout_img_info.sub_out_size.height = p_set_param->out_lca_size_v;
		ime_lcaout_img_info.sub_out_size.line_ofs = p_set_param->out_lca_lofs_y;

		out_lofs_lca_y = ime_lcaout_img_info.sub_out_size.line_ofs;
		out_buf_lca_size_y = (out_lofs_lca_y * ime_lcaout_img_info.sub_out_size.height);

		ime_lcaout_img_info.sub_out_addr = out_addr0_lca_y;

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_SUB, (void *)&ime_lcaout_img_info);
	}

	// lca subin
	if (fnum % 2 == 0) {
		ime_lcain_img_info.in_src = p_set_param->lca_in_src;
		ime_lcain_img_info.img_size.width = p_set_param->out_lca_size_h;
		ime_lcain_img_info.img_size.height = p_set_param->out_lca_size_v;
		ime_lcain_img_info.img_size.line_ofs = p_set_param->out_lca_lofs_y;
		ime_lcain_img_info.dma_addr = out_addr0_lca_y;

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_REF_IMG, (void *)&ime_lcain_img_info);
	} else {
		ime_lcain_img_info.in_src = p_set_param->lca_in_src;
		ime_lcain_img_info.img_size.width = p_set_param->out_lca_size_h;
		ime_lcain_img_info.img_size.height = p_set_param->out_lca_size_v;
		ime_lcain_img_info.img_size.line_ofs = p_set_param->out_lca_lofs_y;
		ime_lcain_img_info.dma_addr = out_addr1_lca_y;

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_REF_IMG, (void *)&ime_lcain_img_info);
	}

	ime_ipl_func_param.ipl_ctrl_func = (KDRV_IME_IQ_FUNC_LCA | KDRV_IME_IQ_FUNC_TMNR);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, &ime_ipl_func_param);

	// lca
	if (fnum >= 2) {

		ime_lca_ctrl_param.enable = TRUE;
		//ime_lca_ctrl_param.bypass = FALSE;
		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LCA_FUNC_EN, &ime_lca_ctrl_param);

		ime_lca_set_param_enable = TRUE;
		ime_lca_set_param.chroma.ref_y_wt = 16;   ///< Chroma reference weighting for Y channels
		ime_lca_set_param.chroma.ref_uv_wt = 16;  ///< Chroma reference weighting for UV channels
		ime_lca_set_param.chroma.out_uv_wt = 16;  ///< Chroma adaptation output weighting
		ime_lca_set_param.chroma.y_rng = KDRV_IME_RANGE_8;
		ime_lca_set_param.chroma.y_wt_prc = KDRV_IME_RANGE_8;
		ime_lca_set_param.chroma.y_th = 0;
		ime_lca_set_param.chroma.y_wt_s = 8;
		ime_lca_set_param.chroma.y_wt_e = 0;

		ime_lca_set_param.chroma.uv_rng = KDRV_IME_RANGE_16;
		ime_lca_set_param.chroma.uv_wt_prc = KDRV_IME_RANGE_16;
		ime_lca_set_param.chroma.uv_th = 0;
		ime_lca_set_param.chroma.uv_wt_s = 16;
		ime_lca_set_param.chroma.uv_wt_e = 0;

		ime_lca_set_param.ca.enable = FALSE;
		ime_lca_set_param.luma.enable = FALSE;
#if (KDRV_IME_PQ_NEW == 1)
		ime_lca_set_param.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
#endif
		kdrv_ime_set(id, KDRV_IME_PARAM_IQ_LCA, (void *)&ime_lca_set_param);
	}

	// 3dnr ref-out
	{
		ime_refout_img_info.enable = TRUE;
		ime_refout_img_info.encode.enable = TRUE;
		ime_refout_img_info.flip_en = FALSE;
		ime_refout_img_info.img_line_ofs_y = p_set_param->out_ref_lofs_y;
		ime_refout_img_info.img_line_ofs_cb = p_set_param->out_ref_lofs_uv;

		out_lofs_refout_y = ime_refout_img_info.img_line_ofs_y;
		out_lofs_refout_uv = ime_refout_img_info.img_line_ofs_cb;
		out_buf_refout_size_y = (out_lofs_refout_y * p_set_param->in_height);
		out_buf_refout_size_u = (out_lofs_refout_uv * (p_set_param->in_height >> 1));

		if (fnum % 2 == 0) {
			ime_refout_img_info.img_addr_y = out_addr_refout_y1;
			ime_refout_img_info.img_addr_cb = out_addr_refout_u1;
		} else {
			ime_refout_img_info.img_addr_y = out_addr_refout_y0;
			ime_refout_img_info.img_addr_cb = out_addr_refout_u0;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG, (void *)&ime_refout_img_info);
	}

	// 3dnr
	kdrv_ime_get(id, KDRV_IME_PARAM_IQ_TMNR, &ime_tmnr_set_iq_param);
	DBG_IND("ime_tmnr_set_iq_param.enable: %d\r\n", ime_tmnr_set_iq_param.enable);
	ime_tmnr_set_iq_param.enable = TRUE;
	kdrv_ime_set(id, KDRV_IME_PARAM_IQ_TMNR, &ime_tmnr_set_iq_param);

	ime_tmnr_set_in_img_info.flip_en = FALSE;
	ime_tmnr_set_in_img_info.decode.enable = TRUE;
	ime_tmnr_set_in_img_info.img_line_ofs_y = out_lofs_refout_y;
	ime_tmnr_set_in_img_info.img_line_ofs_cb = out_lofs_refout_uv;

	ime_tmnr_set_mv.mot_vec_ofs = out_lofs_mv;
	ime_tmnr_set_ms.mot_sta_ofs = out_lofs_ms;

	ime_tmnr_set_ms_roi.mot_sta_roi_out_en = TRUE;
	ime_tmnr_set_ms_roi.mot_sta_roi_out_flip_en = FALSE;
	ime_tmnr_set_ms_roi.mot_sta_roi_out_ofs = out_lofs_ms_roi;

	ime_tmnr_set_sta.sta_out_en = TRUE;
	ime_tmnr_set_sta.sta_out_ofs = out_lofs_sta;
	ime_tmnr_set_sta.sta_param = ime_tmnr_buf_info.get_sta_param;

	if (fnum % 2 == 0) {
		ime_tmnr_set_in_img_info.img_addr_y = out_addr_refout_y0;
		ime_tmnr_set_in_img_info.img_addr_cb = out_addr_refout_u0;

		ime_tmnr_set_mv.mot_vec_in_addr = out_addr_mv0;
		ime_tmnr_set_mv.mot_vec_out_addr = out_addr_mv1;

		ime_tmnr_set_ms.mot_sta_in_addr = out_addr_ms0;
		ime_tmnr_set_ms.mot_sta_out_addr = out_addr_ms1;

		ime_tmnr_set_ms_roi.mot_sta_roi_out_addr = out_addr_ms_roi0;

		ime_tmnr_set_sta.sta_out_addr = out_addr_sta0;

	} else {
		ime_tmnr_set_in_img_info.img_addr_y = out_addr_refout_y1;
		ime_tmnr_set_in_img_info.img_addr_cb = out_addr_refout_u1;

		ime_tmnr_set_mv.mot_vec_in_addr = out_addr_mv1;
		ime_tmnr_set_mv.mot_vec_out_addr = out_addr_mv0;

		ime_tmnr_set_ms.mot_sta_in_addr = out_addr_ms1;
		ime_tmnr_set_ms.mot_sta_out_addr = out_addr_ms0;

		ime_tmnr_set_ms_roi.mot_sta_roi_out_addr = out_addr_ms_roi1;

		ime_tmnr_set_sta.sta_out_addr = out_addr_sta1;
	}
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG, &ime_tmnr_set_in_img_info);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_MV, &ime_tmnr_set_mv);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_MS, &ime_tmnr_set_ms);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_MS_ROI, &ime_tmnr_set_ms_roi);
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_TMNR_STA, &ime_tmnr_set_sta);

	kdrv_ime_set(id, KDRV_IME_PARAM_IQ_TMNR_DBG, &ime_tmnr_set_dbg_param);


	{
		if (fnum == 4) {
			out_size_p2_h = 640;
			out_size_p2_v = 480;

			out_lofs_p2_y = 640;
			out_lofs_p2_uv = 640;

			ime_out_img_info[1].path_id = KDRV_IME_PATH_ID_2;
			ime_out_img_info[1].path_en = TRUE;
			ime_out_img_info[1].path_flip_en = FALSE;
			ime_out_img_info[1].type = KDRV_IME_OUT_FMT_Y_PACK_UV420;//Out_Data[i].fmt;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].line_ofs = out_lofs_p2_y;//Out_Data[i].LineOfs;

			//ime_out_img_info[1].ch[1] = IPL_UTI_Y_INFO_CONV2_UV(Out_Data[i].fmt, ime_out_img_info[1].ch[0]);

			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

			ime_out_img_info[1].crp_window.x = 0;
			ime_out_img_info[1].crp_window.y = 0;
			ime_out_img_info[1].crp_window.w = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].crp_window.h = out_size_p2_v;//Out_Data[i].SizeV;
			kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&ime_out_img_info[1]);
			ime_out_img_info[1].path_id = KDRV_IME_PATH_ID_2;
			ime_out_img_info[1].path_en = TRUE;
			ime_out_img_info[1].path_flip_en = FALSE;
			ime_out_img_info[1].type = KDRV_IME_OUT_FMT_Y_PACK_UV420;//Out_Data[i].fmt;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_Y].line_ofs = out_lofs_p2_y;//Out_Data[i].LineOfs;

			//ime_out_img_info[1].ch[1] = IPL_UTI_Y_INFO_CONV2_UV(Out_Data[i].fmt, ime_out_img_info[1].ch[0]);

			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_U].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].width = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].height = out_size_p2_v;//Out_Data[i].SizeV;
			ime_out_img_info[1].ch[KDRV_IPP_YUV_V].line_ofs = out_lofs_p2_uv;//Out_Data[i].LineOfs;

			ime_out_img_info[1].crp_window.x = 0;
			ime_out_img_info[1].crp_window.y = 0;
			ime_out_img_info[1].crp_window.w = out_size_p2_h;//Out_Data[i].SizeH;
			ime_out_img_info[1].crp_window.h = out_size_p2_v;//Out_Data[i].SizeV;
			kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&ime_out_img_info[1]);

			switch (ime_out_img_info[1].type) {
			case KDRV_IME_OUT_FMT_YUV444:
			case KDRV_IME_OUT_FMT_RGB:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
				out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
				out_buf_p2_size_v = (out_lofs_p2_uv * out_size_p2_v);

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
				break;

			case KDRV_IME_OUT_FMT_YUV422:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

				out_lofs_p2_uv = out_lofs_p2_uv >> 1;
				out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
				out_buf_p2_size_v = (out_lofs_p2_uv * out_size_p2_v);

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
				break;

			case KDRV_IME_OUT_FMT_YUV420:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

				out_lofs_p2_uv = out_lofs_p2_uv >> 1;
				out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
				out_buf_p2_size_v = (out_lofs_p2_uv * (out_size_p2_v >> 1));

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = (UINT32)ime_out_buf_p2_v.va_addr;
				break;

			case KDRV_IME_OUT_FMT_Y_PACK_UV444:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
				out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
				out_buf_p2_size_v = out_buf_p2_size_u;

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = out_addr_p2_u;
				break;

			case KDRV_IME_OUT_FMT_Y_PACK_UV422:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
				out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
				out_buf_p2_size_v = out_buf_p2_size_u;

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = out_addr_p2_u;
				break;

			case KDRV_IME_OUT_FMT_Y_PACK_UV420:
				out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

				out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
				out_buf_p2_size_v = out_buf_p2_size_u;

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = (UINT32)ime_out_buf_p2_u.va_addr;
				//out_addr_p2_v = out_addr_p2_u;
				break;

			case KDRV_IME_OUT_FMT_Y:
				out_buf_p2_size_y = (out_lofs_p2_y * (out_size_p2_v >> 1));
				out_buf_p2_size_u = 0;
				out_buf_p2_size_v = 0;

				//out_addr_p2_y = (UINT32)ime_out_buf_p2_y.va_addr;
				//out_addr_p2_u = out_addr_p2_y;
				//out_addr_p2_v = out_addr_p2_y;
				break;

			default:
				break;
			}
		}
	}


	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_LOAD, NULL);

	return 0;
}


static int nvt_kdrv_ipp_ime_save_output(void)
{
	VOS_FILE fp;
	int len;

	if ((ime_out_img_info[0].path_id == KDRV_IME_PATH_ID_1) && (ime_out_img_info[0].path_en == ENABLE)) {
		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			// save y to file
			fp = vos_file_open(ime_out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save u to file
			fp = vos_file_open(ime_out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_u, out_buf_p1_size_u);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(ime_out_p1_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_v, out_buf_p1_size_v);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p1_y);
			kdrv_ipp_rel_buff(&ime_out_buf_p1_u);
			kdrv_ipp_rel_buff(&ime_out_buf_p1_v);
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(ime_out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(ime_out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_u, out_buf_p1_size_u);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p1_y);
			kdrv_ipp_rel_buff(&ime_out_buf_p1_u);
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(ime_out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p1_y);
			break;

		default:
			break;
		}
	}

	if ((ime_out_img_info[1].path_id == KDRV_IME_PATH_ID_2) && (ime_out_img_info[1].path_en == ENABLE)) {
		switch (ime_out_img_info[1].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			// save y to file
			fp = vos_file_open(ime_out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save u to file
			fp = vos_file_open(ime_out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_u, out_buf_p2_size_u);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(ime_out_p2_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_v, out_buf_p2_size_v);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p2_y);
			kdrv_ipp_rel_buff(&ime_out_buf_p2_u);
			kdrv_ipp_rel_buff(&ime_out_buf_p2_v);
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(ime_out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(ime_out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_u, out_buf_p2_size_u);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p2_y);
			kdrv_ipp_rel_buff(&ime_out_buf_p2_u);
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(ime_out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", ime_out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			DBG_IND("ime: write file lenght: %d\r\n", len);
			vos_file_close(fp);

			kdrv_ipp_rel_buff(&ime_out_buf_p2_y);
			break;

		default:
			break;
		}
	}

	if (ime_refout_img_info.enable == TRUE) {
		// save y to file
		fp = vos_file_open(ime_out_refout_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", ime_out_refout_raw_file_y);
			return FALSE;
		}
		len = vos_file_write(fp, (void *)out_addr_refout_y0, out_buf_refout_size_y);
		DBG_IND("ime: write file lenght: %d\r\n", len);
		vos_file_close(fp);

		// save uv to file
		fp = vos_file_open(ime_out_refout_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", ime_out_refout_raw_file_u);
			return FALSE;
		}
		len = vos_file_write(fp, (void *)out_addr_refout_u0, out_buf_refout_size_u);
		DBG_IND("ime: write file lenght: %d\r\n", len);
		vos_file_close(fp);

		kdrv_ipp_rel_buff(&out_buf_refout_y0);
		kdrv_ipp_rel_buff(&out_buf_refout_u0);

		kdrv_ipp_rel_buff(&out_buf_refout_y1);
		kdrv_ipp_rel_buff(&out_buf_refout_u1);
	}

	if (ime_lcaout_img_info.enable == TRUE) {
		// save y to file
		fp = vos_file_open(ime_out_lcaout_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", ime_out_lcaout_raw_file_y);
			return FALSE;
		}
		len = vos_file_write(fp, (void *)ime_lcaout_img_info.sub_out_addr, out_buf_lca_size_y);
		DBG_IND("ime: write file lenght: %d\r\n", len);
		vos_file_close(fp);

		kdrv_ipp_rel_buff(&out_buf0_lca_y);
		kdrv_ipp_rel_buff(&out_buf1_lca_y);
	}



	if (ime_tmnr_set_iq_param.enable == TRUE) {

		kdrv_ipp_rel_buff(&out_buf_tmnr_mv0);
		kdrv_ipp_rel_buff(&out_buf_tmnr_mv1);

		kdrv_ipp_rel_buff(&out_buf_tmnr_ms0);
		kdrv_ipp_rel_buff(&out_buf_tmnr_ms1);

		kdrv_ipp_rel_buff(&out_buf_tmnr_ms_roi0);
		kdrv_ipp_rel_buff(&out_buf_tmnr_ms_roi1);

		kdrv_ipp_rel_buff(&out_buf_tmnr_sta0);
		kdrv_ipp_rel_buff(&out_buf_tmnr_sta1);
	}

	return 0;
}

//----------------------------------------------------------------------------------


static int nvt_kdrv_ipp_trigger(VOID)
{
	/*ime trig*/
	kdrv_ime_trigger(set_kdrv_id[nvt_kdrv_id_ime], NULL, NULL, NULL);
	DBG_IND("kdrv_ime_trigger, done\r\n");

	if ((ime_lca_set_param_enable == TRUE) && (g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		kdrv_ife2_trigger(set_kdrv_id[nvt_kdrv_id_ife2], NULL, NULL, NULL);
		DBG_IND("kdrv_ife2_trigger, done\r\n");
	}

	/*dce trig stripe calculation*/
	dce_trig_info.opmode = KDRV_DCE_OP_MODE_CAL_STRIP;
	kdrv_dce_trigger(set_kdrv_id[nvt_kdrv_id_dce], NULL, NULL, (void *)&dce_trig_info);
	DBG_IND("KDRV_DCE_OP_MODE_CAL_STRIP\r\n");

	/*trig dce start*/
	dce_trig_info.opmode = KDRV_DCE_OP_MODE_NORMAL;
	kdrv_dce_trigger(set_kdrv_id[nvt_kdrv_id_dce], NULL, NULL, (void *)&dce_trig_info);
	DBG_IND("kdrv_dce_trigger, done\r\n");

	/*ipe trig*/
	kdrv_ipe_trigger(set_kdrv_id[nvt_kdrv_id_ipe], NULL, NULL, NULL);
	DBG_IND("kdrv_ipe_trigger, done\r\n");


	/*ife trig*/
	if (g_set_dce_param.dce_op_mode != KDRV_DCE_IN_SRC_DCE_IME) {
		kdrv_ife_trigger(set_kdrv_id[nvt_kdrv_id_ife], NULL, NULL, NULL);
		DBG_IND("kdrv_ife_trigger, done\r\n");
	}

	DBG_IND("ipp trigger done\r\n");

	/******* Start SIE *********/
	if (g_set_dce_param.dce_op_mode == KDRV_DCE_IN_SRC_ALL_DIRECT) {
		sie_trig_cfg.trig_type = KDRV_SIE_TRIG_START;
		sie_trig_cfg.wait_end = TRUE;

		DBG_IND("sie trig\r\n");
		if (kdrv_sie_trigger(set_kdrv_id[nvt_kdrv_id_sie], NULL, NULL, (void *)&sie_trig_cfg) != E_OK) {
			DBG_ERR("sie trigger fail!\r\n");
			return 1;
		}
	}


	return 0;
}
//---------------------------------------------------------------------------------------------

static int nvt_kdrv_ipp_clear_frame_end(VOID)
{
	nvt_kdrv_ife_fend_flag = 0;
	nvt_kdrv_dce_fend_flag = 0;
	nvt_kdrv_ipe_fend_flag = 0;
	nvt_kdrv_ime_fend_flag = 0;

	if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		nvt_kdrv_ife2_fend_flag = 0;
	}

	return 0;
}

static int nvt_kdrv_ipp_wait_frame_end(BOOL clear_flag)
{
	BOOL ife_fend_disable = (g_set_dce_param.dce_op_mode == KDRV_DCE_IN_SRC_DCE_IME);

	/*wait end*/
	if (!ife_fend_disable) {
		while (!nvt_kdrv_ife_fend_flag) {
			vos_task_delay_ms(1);
		}
		DBG_IND("ife end\r\n");
	}

	while (!nvt_kdrv_ipe_fend_flag) {
		vos_task_delay_ms(1);
	}
	DBG_IND("ipe end\r\n");

	while (!nvt_kdrv_ime_fend_flag) {
		vos_task_delay_ms(1);
	}
	DBG_IND("ime end\r\n");

	if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		while (!nvt_kdrv_ife2_fend_flag) {
			vos_task_delay_ms(1);
		}
		DBG_IND("ife2 end\r\n");
	}

	while (!nvt_kdrv_dce_fend_flag) {
		vos_task_delay_ms(1);
	}
	DBG_IND("dce end\r\n");

	if (clear_flag) {
		nvt_kdrv_ipp_clear_frame_end();
	}

	DBG_IND("get IPP end\r\n");

	return 0;
}

void nvt_kdrv_ipp_param_buff_init(void)
{
	UINT32 size;
#if (KDRV_IPP_NEW_INIT_FLOW == ENABLE)
	UINT32 curr_buff_addr;

	size = kdrv_sie_buf_query(1);
	size += kdrv_ife_buf_query(KDRV_IPP_CBUF_MAX_NUM);
	size += kdrv_dce_buf_query(KDRV_IPP_CBUF_MAX_NUM);
	size += kdrv_ipe_buf_query(KDRV_IPP_CBUF_MAX_NUM);
	size += kdrv_ime_buf_query(KDRV_IPP_CBUF_MAX_NUM);
	size += kdrv_ife2_buf_query(KDRV_IPP_CBUF_MAX_NUM);

	if (kdrv_ipp_alloc_buff(&init_ipp_buff, size) == E_OK) {
		DBG_IND("IPP param buff start 0x%08x, size 0x%x\r\n", (unsigned int)init_ipp_buff.vaddr, (unsigned int)size);
		curr_buff_addr = (UINT32)init_ipp_buff.vaddr;
//		curr_buff_addr += kdrv_sie_buf_init(curr_buff_addr, size); //jarkko tmp mark for new sie drv
		curr_buff_addr += kdrv_ife_buf_init(curr_buff_addr, KDRV_IPP_CBUF_MAX_NUM);
		curr_buff_addr += kdrv_dce_buf_init(curr_buff_addr, KDRV_IPP_CBUF_MAX_NUM);
		curr_buff_addr += kdrv_ipe_buf_init(curr_buff_addr, KDRV_IPP_CBUF_MAX_NUM);
		curr_buff_addr += kdrv_ime_buf_init(curr_buff_addr, KDRV_IPP_CBUF_MAX_NUM);
		curr_buff_addr += kdrv_ife2_buf_init(curr_buff_addr, KDRV_IPP_CBUF_MAX_NUM);
		DBG_IND("IPP param buff allocated size 0x%x\r\n", (unsigned int)curr_buff_addr);
	} else {
		DBG_ERR("Allocate IPP parameter buffer fail!\r\n");
	}
#endif
}

void nvt_kdrv_ipp_param_buff_uninit(void)
{
#if (KDRV_IPP_NEW_INIT_FLOW == ENABLE)
	kdrv_ipp_rel_buff(&init_ipp_buff);
#endif

	kdrv_sie_buf_uninit();
	kdrv_ife_buf_uninit();
	kdrv_dce_buf_uninit();
	kdrv_ipe_buf_uninit();
	kdrv_ime_buf_uninit();
	kdrv_ife2_buf_uninit();
}

static int nvt_kdrv_ipp_open(void)
{
	UINT32 id;
	int rt;

	if (g_set_dce_param.dce_op_mode == KDRV_DCE_IN_SRC_ALL_DIRECT) {
		// sie open
		id = set_kdrv_id[nvt_kdrv_id_sie];

#if 1
		sie_opencfg.clk_src_sel = KDRV_SIE_MCLKSRC_480;//KDRV_SIE_CLKSRC_PLL5;
		sie_opencfg.act_mode = KDRV_SIE_IN_PATGEN;
		sie_opencfg.pclk_src_sel = KDRV_SIE_PXCLKSRC_MCLK;
		sie_opencfg.data_rate = 10 * 1000000;
		kdrv_sie_set(id, KDRV_SIE_ITEM_OPENCFG, (void *)(&sie_opencfg));

		sie_mclk_info.clk_en = TRUE;
		sie_mclk_info.mclk_src_sel = KDRV_SIE_MCLKSRC_480;
		sie_mclk_info.clk_rate = 10 * 1000000;
		sie_mclk_info.mclk_id_sel = KDRV_SIE_MCLK;
		kdrv_sie_set(id, KDRV_SIE_ITEM_MCLK, (void *)&sie_mclk_info);
		kdrv_sie_set(id, KDRV_SIE_ITEM_LOAD, NULL);
		//sie_setmclock(sie_mclk_src, 30*1000000, 1);
		DBG_IND("after kdrv sie clk cfg\r\n");
		if (kdrv_sie_open(KDRV_CHIP0, KDRV_VDOCAP_ENGINE0) != 0) {
			DBG_IND("kdrv sie open fail!!\r\n");
			return FALSE;
		}
#endif

		DBG_IND("sie open done\r\n");
	}

	// ife open
	id = set_kdrv_id[nvt_kdrv_id_ife];
	ife_opencfg.ife_clock_sel = 240;
	rt = kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_OPENCFG, (void *)&ife_opencfg);
	if (rt != E_OK) {
		return rt;
	}
	kdrv_ife_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE_ENGINE0);
	nvt_kdrv_ife_fend_flag = 0;
	nvt_kdrv_ife_fstart_flag = 0;
	g_ife_fstart_cnt = 0;
	g_ife_fend_cnt = 0;
	rt = kdrv_ife_set(id, KDRV_IFE_PARAM_IPL_ISR_CB, (void *)&nvt_kdrv_ipp_ife_isr_cb);
	if (rt != E_OK) {
		return rt;
	}

	DBG_IND("ife open done\r\n");

	// dce open
	id = set_kdrv_id[nvt_kdrv_id_dce];
	dce_opencfg.dce_clock_sel = 240;
	rt = kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_OPENCFG, (void *)&dce_opencfg);
	if (rt != E_OK) {
		return rt;
	}
	kdrv_dce_open(KDRV_CHIP0, KDRV_VIDEOPROCS_DCE_ENGINE0);
	nvt_kdrv_dce_fend_flag = 0;
	nvt_kdrv_dce_fstart_flag = 0;
	g_dce_fstart_cnt = 0;
	g_dce_fend_cnt = 0;
	rt = kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ISR_CB, (void *)&nvt_kdrv_ipp_dce_isr_cb);
	if (rt != E_OK) {
		return rt;
	}

	DBG_IND("dce open done\r\n");

	// ipe open
	id = set_kdrv_id[nvt_kdrv_id_ipe];
	ipe_opencfg.ipe_clock_sel = 240;
	rt = kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OPENCFG, (void *)&ipe_opencfg);
	if (rt != E_OK) {
		return rt;
	}
	kdrv_ipe_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IPE_ENGINE0);
	nvt_kdrv_ipe_fend_flag = 0;
	nvt_kdrv_ipe_fstart_flag = 0;
	g_ipe_fstart_cnt = 0;
	g_ipe_fend_cnt = 0;
	rt = kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)&nvt_kdrv_ipp_ipe_isr_cb);
	if (rt != E_OK) {
		return rt;
	}

	DBG_IND("ipe open done\r\n");

	// ime open
	id = set_kdrv_id[nvt_kdrv_id_ime];
	ime_opencfg.ime_clock_sel = 240;
	rt = kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OPENCFG, (void *)&ime_opencfg);
	if (rt != E_OK) {
		return rt;
	}
	kdrv_ime_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
	nvt_kdrv_ime_fend_flag = 0;
	nvt_kdrv_ime_fstart_flag = 0;
	g_ime_fend_cnt = 0;
	g_ime_fstart_cnt = 0;
	rt = kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ISR_CB, (void *)&nvt_kdrv_ipp_ime_isr_cb);
	if (rt != E_OK) {
		return rt;
	}

	DBG_IND("ime open done\r\n");

	// ife2 open
	id = set_kdrv_id[nvt_kdrv_id_ife2];
	ife2_opencfg.ife2_clock_sel = 240;
	rt = kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OPENCFG, (void *)&ife2_opencfg);
	if (rt != E_OK) {
		return rt;
	}
	kdrv_ife2_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
	nvt_kdrv_ife2_fend_flag = 0;
	rt = kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_ISR_CB, (void *)&nvt_kdrv_ipp_ife2_isr_cb);
	if (rt != E_OK) {
		return rt;
	}

	DBG_IND("ife2 open done\r\n");

	return 0;
}

static int nvt_kdrv_ipp_pause(void)
{
	if (g_set_dce_param.dce_op_mode != KDRV_DCE_IN_SRC_DCE_IME) {
		kdrv_ife_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE_ENGINE0);
	}
	kdrv_dce_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_DCE_ENGINE0);
	kdrv_ipe_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IPE_ENGINE0);
	kdrv_ime_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);

	if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		kdrv_ife2_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
	}

	return 0;
}

static int nvt_kdrv_ipp_close(VOID)
{
	kdrv_ife_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE_ENGINE0);
	kdrv_dce_close(KDRV_CHIP0, KDRV_VIDEOPROCS_DCE_ENGINE0);
	kdrv_ipe_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IPE_ENGINE0);
	kdrv_ime_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
	kdrv_ife2_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);

	if (g_set_dce_param.dce_op_mode == KDRV_DCE_IN_SRC_ALL_DIRECT) {
		kdrv_sie_close(KDRV_CHIP0, KDRV_VDOCAP_ENGINE0);
	}

	return 0;
}

/* -------- external APIs for proc command -------- */
int nvt_kdrv_ipp_show_stripe_settings(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
	KDRV_DCE_STRIPE_INFO tmp_stripe = {0};
	INT32 i;

	kdrv_dce_get(set_kdrv_id[nvt_kdrv_id_dce], KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&tmp_stripe);

	for (i = 0; i < (INT32)tmp_stripe.hstp_num; i ++) {
		DBG_DUMP("dce out stripe %u, ime out stripe %u\r\n", (unsigned int)tmp_stripe.hstp[i], (unsigned int)tmp_stripe.ime_out_hstp[i]);
	}

	return 0;
}

int nvt_kdrv_ipp_initialize_hw_and_param_buf(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
	//-- don't change order --
	nvt_kdrv_ipp_set_id();
	nvt_kdrv_ipp_param_buff_init();
	nvt_kdrv_ipp_open();
	//------------------------

	DBG_IND("done.\r\n");
	return 0;
}

int nvt_kdrv_ipp_release_hw_and_param_buf(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
	//-- don't change order --
	nvt_kdrv_ipp_close();
	nvt_kdrv_ipp_param_buff_uninit();
	//------------------------

	DBG_IND("done.\r\n");
	return 0;
}

int nvt_kdrv_ipp_flow02_test(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
	unsigned int dce_set_load = 0, ipe_set_load = 0, ife_op_mode = 0, ife_in_size_h = 0, ife_in_size_v = 0, new_disto_rate = 0, dce_strp_type = 0;

#if defined (__LINUX)
	kstrtoint(pargv[0], 0, &dce_set_load);
	kstrtoint(pargv[1], 0, &ipe_set_load);
	kstrtoint(pargv[2], 0, &ife_op_mode);
	kstrtoint(pargv[3], 0, &ife_in_size_h);
	kstrtoint(pargv[4], 0, &ife_in_size_v);
	snprintf(ife_in_raw, sizeof(ife_in_raw), "/mnt/sd/ipp/input/%s", pargv[5]);
	kstrtoint(pargv[6], 0, &new_disto_rate);
	kstrtoint(pargv[7], 0, &dce_strp_type);
#else
	dce_set_load = atoi(pargv[0]);
	ipe_set_load = atoi(pargv[1]);
	ife_op_mode = atoi(pargv[2]);
	ife_in_size_h = atoi(pargv[3]);
	ife_in_size_v = atoi(pargv[4]);
	snprintf(ife_in_raw, sizeof(ife_in_raw), "/mnt/sd/ipp/input/%s", pargv[5]);
	new_disto_rate = atoi(pargv[6]);
	dce_strp_type = atoi(pargv[7]);
#endif

	DBG_IND("dce_set_load = %u\r\n", dce_set_load);
	DBG_IND("ipe_set_load = %u\r\n", ipe_set_load);
	DBG_IND("ife_op_mode = %u\r\n", ife_op_mode);
	DBG_IND("ife size %ux%u\r\n", ife_in_size_h, ife_in_size_v);
	DBG_IND("ife_in_raw %s\r\n", ife_in_raw);
	DBG_IND("new_disto_rate = %u\r\n", new_disto_rate);
	DBG_IND("dce_strp_type = %u\r\n", dce_strp_type);

	if (ife_op_mode == 0) {
		ife_op_mode = 2;//prevent setting to D2D
	}

	g_set_ife_param.ife_op_mode = (KDRV_IFE_IN_MODE)ife_op_mode;

	g_set_sie_param.crop_out_width  = ife_in_size_h;
	g_set_sie_param.crop_out_height = ife_in_size_v;
	nvt_kdrv_ipp_sie_set_param(set_kdrv_id[nvt_kdrv_id_sie], &g_set_sie_param);

	g_set_ife_param.in_bit = 12;
	g_set_ife_param.out_bit = 12;

	g_set_ife_param.in_raw_width = ife_in_size_h;
	g_set_ife_param.in_raw_height = ife_in_size_v;
	g_set_ife_param.in_raw_lineoffset = ALIGN_CEIL_4(ife_in_size_h * 12 / 8);

	g_set_ife_param.in_raw_crop_width = ife_in_size_h;
	g_set_ife_param.in_raw_crop_height = ife_in_size_v;
	g_set_ife_param.in_raw_crop_pos_x = 0;
	g_set_ife_param.in_raw_crop_pos_y = 0;

	g_set_ife_param.in_hn = 0;
	g_set_ife_param.in_hl = 480;
	g_set_ife_param.in_hm = 1;

	nvt_kdrv_ipp_ife_set_param(set_kdrv_id[nvt_kdrv_id_ife], &g_set_ife_param);
	DBG_IND("nvt_kdrv_ipp_ife_set_param...\r\n");

	// set dce parameters
	if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_IPP) {
		g_set_dce_param.dce_op_mode = KDRV_DCE_IN_SRC_DIRECT;
	} else if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_DIRECT) {
		g_set_dce_param.dce_op_mode = KDRV_DCE_IN_SRC_ALL_DIRECT;
	}
	g_set_dce_param.dce_strp_type = (KDRV_DCE_STRIPE_TYPE)dce_strp_type;
	g_set_dce_param.in_width    = g_set_ife_param.in_raw_crop_width;
	g_set_dce_param.in_height   = g_set_ife_param.in_raw_crop_height;
	g_set_dce_param.new_disto_rate = (UINT32)new_disto_rate;
	nvt_kdrv_ipp_dce_set_param(set_kdrv_id[nvt_kdrv_id_dce], &g_set_dce_param);
	DBG_IND("nvt_kdrv_ipp_dce_set_param...\r\n");

	// set ipe parameters
	if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_IPP) {
		g_set_ipe_param.ipe_op_mode = KDRV_IPE_IN_SRC_DIRECT;
	} else if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_DIRECT) {
		g_set_ipe_param.ipe_op_mode = KDRV_IPE_IN_SRC_ALL_DIRECT;
	}
	g_set_ipe_param.in_width    = g_set_dce_param.in_width;
	g_set_ipe_param.in_height   = g_set_dce_param.in_height;
	nvt_kdrv_ipp_ipe_set_param(set_kdrv_id[nvt_kdrv_id_ipe], &g_set_ipe_param);
	DBG_IND("nvt_kdrv_ipp_ipe_set_param...\r\n");

	// set ime parameters
	if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_IPP) {
		g_set_ime_param.ime_op_mode = KDRV_IME_IN_SRC_DIRECT;
	} else if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_DIRECT) {
		g_set_ime_param.ime_op_mode = KDRV_IME_IN_SRC_ALL_DIRECT;
	}

	g_set_ime_param.in_width    = g_set_ipe_param.in_width;
	g_set_ime_param.in_height   = g_set_ipe_param.in_height;

	g_set_ime_param.out_p1_size_h  = g_set_ime_param.in_width;
	g_set_ime_param.out_p1_size_v  = g_set_ime_param.in_height;
	g_set_ime_param.out_p1_lofs_y  = g_set_ime_param.in_width;
	g_set_ime_param.out_p1_lofs_uv = g_set_ime_param.in_width;

	g_set_ime_param.out_p2_size_h  = g_set_ime_param.in_width / 2;
	g_set_ime_param.out_p2_size_v  = g_set_ime_param.in_height / 2;
	g_set_ime_param.out_p2_lofs_y  = g_set_ime_param.in_width / 2;
	g_set_ime_param.out_p2_lofs_uv = g_set_ime_param.in_width / 2;

	g_set_ime_param.out_ref_size_h  = g_set_ime_param.in_width;
	g_set_ime_param.out_ref_size_v  = g_set_ime_param.in_height;
	g_set_ime_param.out_ref_lofs_y  = g_set_ime_param.in_width;
	g_set_ime_param.out_ref_lofs_uv = g_set_ime_param.in_width;


	g_set_ime_param.out_lca_size_h = g_set_ime_param.in_width >> 2;
	g_set_ime_param.out_lca_size_v = g_set_ime_param.in_height >> 2;
	g_set_ime_param.out_lca_lofs_y = ALIGN_CEIL_4(g_set_ime_param.out_lca_size_h * 3);
	g_set_ime_param.lca_in_src = KDRV_IME_LCA_IN_SRC_DIRECT;
	nvt_kdrv_ipp_ime_set_param(set_kdrv_id[nvt_kdrv_id_ime], &g_set_ime_param);
	DBG_IND("nvt_kdrv_ipp_ime_set_param...\r\n");


	if ((ime_lca_set_param_enable == TRUE) && (g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		// set ife2 parameter
		g_set_ife2_param.in_width = g_set_ime_param.out_lca_size_h;
		g_set_ife2_param.in_height = g_set_ime_param.out_lca_size_v;
		g_set_ife2_param.in_lofs = g_set_ime_param.out_lca_lofs_y;

		if ((g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_DIRECT)) {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DIRECT;
		} else if ((g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_ALL_DIRECT)) {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_ALL_DIRECT;
		} else {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DRAM;
		}
		g_set_ife2_param.in_addr = out_addr0_lca_y;
		g_set_ife2_param.out_addr = out_addr1_lca_y;
		nvt_kdrv_ipp_ife2_set_param(set_kdrv_id[nvt_kdrv_id_ife2], &g_set_ife2_param);
		DBG_IND("nvt_kdrv_ipp_ife2_set_param...\r\n");
	}

	nvt_kdrv_ipp_trigger();

	//nvt_kdrv_ipp_ime_set_load_param(set_kdrv_id[nvt_kdrv_id_ime], &g_set_ime_param, g_fcnt);

	if (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_ALL_DIRECT) {

		while (1) {
			//nvt_kdrv_ife_wait_frame_start(TRUE);
			//DBG_IND("g_ife_fstart_cnt %d\r\n", (int)g_ife_fstart_cnt);

			//nvt_kdrv_dce_wait_frame_start(TRUE);
			//DBG_IND("g_dce_fstart_cnt %d\r\n", (int)g_dce_fstart_cnt);
			if (dce_set_load > 0) {
				nvt_kdrv_ipp_dce_set_load_param(set_kdrv_id[nvt_kdrv_id_dce], &g_set_dce_param, g_dce_fstart_cnt);
				DBG_IND("dce load done\r\n");
			}

			//nvt_kdrv_ipe_wait_frame_start(TRUE);
			//DBG_IND("g_ipe_fstart_cnt %d\r\n", (int)g_ipe_fstart_cnt);
			if (ipe_set_load > 0) {
				nvt_kdrv_ipp_ipe_set_load_param(set_kdrv_id[nvt_kdrv_id_ipe], &g_set_ipe_param, g_ipe_fstart_cnt);
				DBG_IND("ipe load done\r\n");
			}

			//nvt_kdrv_ime_wait_frame_start(TRUE);
			//DBG_IND("g_ime_fstart_cnt %d\r\n", (int)g_ime_fstart_cnt);

			//nvt_kdrv_ipp_wait_frame_end(TRUE);

			if (0) { //g_ime_fstart_cnt == 4) {
				g_set_ime_param.out_lca_size_h = 240;
				g_set_ime_param.out_lca_size_v = 134;
				g_set_ime_param.out_lca_lofs_y = (240 * 3);

				ime_lca_ctrl_param.bypass = TRUE;

				g_set_ife2_param.in_width = g_set_ime_param.out_lca_size_h;
				g_set_ife2_param.in_height = g_set_ime_param.out_lca_size_v;
				g_set_ife2_param.in_lofs = g_set_ime_param.out_lca_lofs_y;

				DBG_IND("lca-ife2 change...\r\n");
			} else {
				ime_lca_ctrl_param.bypass = FALSE;
			}

#if 0
			if (g_ime_fstart_cnt == 2) {
				ife_in_size_h /= 2;
				ife_in_size_v /= 2;

				DBG_IND("change IPP size to %dx%d\r\n", (int)ife_in_size_h, (int)ife_in_size_v);

				g_set_sie_param.crop_out_width  = ife_in_size_h;
				g_set_sie_param.crop_out_height = ife_in_size_v;

				g_set_ife_param.in_raw_width = ife_in_size_h;
				g_set_ife_param.in_raw_height = ife_in_size_v;
				g_set_ife_param.in_raw_crop_width  = ife_in_size_h;
				g_set_ife_param.in_raw_crop_height = ife_in_size_v;
				g_set_ife_param.in_raw_crop_pos_x  = 0;
				g_set_ife_param.in_raw_crop_pos_y  = 0;

				g_set_dce_param.in_width    = g_set_ife_param.in_raw_crop_width;
				g_set_dce_param.in_height   = g_set_ife_param.in_raw_crop_height;

				g_set_ipe_param.in_width    = g_set_dce_param.in_width;
				g_set_ipe_param.in_height   = g_set_dce_param.in_height;

				g_set_ime_param.in_width    = g_set_ipe_param.in_width;
				g_set_ime_param.in_height   = g_set_ipe_param.in_height;

				nvt_kdrv_ipp_sie_set_load_param(set_kdrv_id[nvt_kdrv_id_sie], &g_set_sie_param, g_ime_fstart_cnt);
				nvt_kdrv_ipp_ife_set_load_param(set_kdrv_id[nvt_kdrv_id_ife], &g_set_ife_param, g_ime_fstart_cnt);
				nvt_kdrv_ipp_dce_set_load_param(set_kdrv_id[nvt_kdrv_id_dce], &g_set_dce_param, g_ime_fstart_cnt);
				nvt_kdrv_ipp_ipe_set_load_param(set_kdrv_id[nvt_kdrv_id_ipe], &g_set_ipe_param, g_ime_fstart_cnt);
				nvt_kdrv_ipp_ife2_set_load_param(set_kdrv_id[nvt_kdrv_id_ife2], &g_set_ife2_param, g_ime_fstart_cnt);
				nvt_kdrv_ipp_ime_set_load_param(set_kdrv_id[nvt_kdrv_id_ime], &g_set_ime_param, g_ime_fstart_cnt);

				DBG_IND("change size load done\r\n");
			}
#endif

			if (g_ime_fend_cnt == 7) {

				DBG_IND("stop sie\r\n");

				nvt_kdrv_ipp_clear_frame_end();//sie clock must be fast enough!

				sie_trig_cfg.trig_type = KDRV_SIE_TRIG_STOP;
				sie_trig_cfg.wait_end = TRUE;

				if (kdrv_sie_trigger(set_kdrv_id[nvt_kdrv_id_sie], NULL, NULL, (void *)&sie_trig_cfg) != 0) {
					DBG_ERR("sie trigger stop fail\r\n");
					return FALSE;
				}
				DBG_IND("sie stopped.\r\n");

				DBG_IND("direct wait end\r\n");
				nvt_kdrv_ipp_wait_frame_end(TRUE);
				DBG_IND("g_dce_fend_cnt %d\r\n", (int)g_dce_fend_cnt);
				break;
			}
		}
	} else {
		DBG_IND("fmd2d wait end\r\n");
		nvt_kdrv_ipp_wait_frame_end(FALSE);
	}
	DBG_IND("frame end got\r\n");

	nvt_kdrv_ipp_pause();
	DBG_IND("nvt_kdrv_ipp_pause...\r\n");

	nvt_kdrv_ipp_dce_save_output(&g_set_dce_param);
	nvt_kdrv_ipp_ipe_save_output();
	nvt_kdrv_ipp_ime_save_output();

	if ((sie_out_dst == KDRV_SIE_OUT_DEST_BOTH) || (sie_out_dst == KDRV_SIE_OUT_DEST_DRAM)) {
		kdrv_ipp_rel_buff(&sie_out_buf0);
	}
	if (g_set_ife_param.ife_op_mode == KDRV_IFE_IN_MODE_IPP) {
		kdrv_ipp_rel_buff(&ife_in_buf_a);
	}

	DBG_DUMP("test done.\r\n");

	return 0;
}

int nvt_kdrv_ipp_flow1_test(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
	unsigned long dce_in_size_h = 0, dce_in_size_v = 0;
	UINT32 dce_in_y_addr = 0;
	UINT32 dce_in_uv_addr = 0;
	UINT32 dce_in_y_buf_size = 0;
	UINT32 dce_in_uv_buf_size = 0;

	IPP_BUFF_INFO dce_in_buf_y = {0};
	IPP_BUFF_INFO dce_in_buf_uv = {0};

	char dce_in_y_raw[260] = "";
	char dce_in_uv_raw[260] = "";

#if defined (__LINUX)
	if (kstrtoul(pargv[0], 0, &dce_in_size_h)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}
	if (kstrtoul(pargv[1], 0, &dce_in_size_v)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[1]);
		return -EINVAL;
	}
	sprintf(dce_in_y_raw, "/mnt/sd/ipp/input/%s", pargv[2]);
	sprintf(dce_in_uv_raw, "/mnt/sd/ipp/input/%s", pargv[3]);
#else
	dce_in_size_h = (unsigned long)atoi(pargv[0]);
	dce_in_size_v = (unsigned long)atoi(pargv[1]);
	sprintf(dce_in_y_raw, "/mnt/sd/ipp/input/%s", pargv[2]);
	sprintf(dce_in_uv_raw, "/mnt/sd/ipp/input/%s", pargv[3]);
#endif

	// YUV 422
	dce_in_y_buf_size = dce_in_size_h * dce_in_size_v;
	dce_in_uv_buf_size = dce_in_size_h * dce_in_size_v;

	if (kdrv_ipp_alloc_buff(&dce_in_buf_y, dce_in_y_buf_size) == E_OK) {
		dce_in_y_addr = (UINT32)dce_in_buf_y.vaddr;
		nvt_kdrv_ipp_load_file(dce_in_y_raw, dce_in_y_addr, dce_in_y_buf_size);
		DBG_IND("dce_in_y input : va_addr = 0x%08x, phy_addr = 0x%08x, size = 0x%08x\r\n"
				, (unsigned int)dce_in_buf_y.vaddr
				, (unsigned int)dce_in_buf_y.paddr
				, (unsigned int)dce_in_buf_y.size);
	} else {
		return 1;
	};
	if (kdrv_ipp_alloc_buff(&dce_in_buf_uv, dce_in_uv_buf_size) == E_OK) {
		dce_in_uv_addr = (UINT32)dce_in_buf_uv.vaddr;
		nvt_kdrv_ipp_load_file(dce_in_uv_raw, dce_in_uv_addr, dce_in_uv_buf_size);
		DBG_IND("dce_in_uv input : va_addr = 0x%08x, phy_addr = 0x%08x, size = 0x%08x\r\n"
				, (unsigned int)dce_in_buf_uv.vaddr
				, (unsigned int)dce_in_buf_uv.paddr
				, (unsigned int)dce_in_buf_uv.size);
	} else {
		kdrv_ipp_rel_buff(&dce_in_buf_y);
		return 1;
	};

	// set dce parameters
	g_set_dce_param.dce_op_mode = KDRV_DCE_IN_SRC_DCE_IME;
	g_set_dce_param.in_width    = dce_in_size_h;
	g_set_dce_param.in_height   = dce_in_size_v;
	g_set_dce_param.addr_y      = dce_in_y_addr;
	g_set_dce_param.addr_uv     = dce_in_uv_addr;
	nvt_kdrv_ipp_dce_set_param(set_kdrv_id[nvt_kdrv_id_dce], &g_set_dce_param);
	DBG_IND("nvt_kdrv_ipp_dce_set_param...\r\n");

	// set ipe parameters
	g_set_ipe_param.ipe_op_mode = KDRV_IPE_IN_SRC_DIRECT;
	g_set_ipe_param.in_width    = g_set_dce_param.in_width;
	g_set_ipe_param.in_height   = g_set_dce_param.in_height;
	nvt_kdrv_ipp_ipe_set_param(set_kdrv_id[nvt_kdrv_id_ipe], &g_set_ipe_param);
	DBG_IND("nvt_kdrv_ipp_ipe_set_param...\r\n");

	// set ime parameters
	g_set_ime_param.ime_op_mode = KDRV_IME_IN_SRC_DIRECT;
	g_set_ime_param.in_width    = g_set_ipe_param.in_width;
	g_set_ime_param.in_height   = g_set_ipe_param.in_height;

	g_set_ime_param.out_p1_size_h = g_set_ime_param.in_width;
	g_set_ime_param.out_p1_size_v = g_set_ime_param.in_height;
	g_set_ime_param.out_p1_lofs_y  = g_set_ime_param.in_width;
	g_set_ime_param.out_p1_lofs_uv = g_set_ime_param.in_width;

	g_set_ime_param.out_p2_size_h  = g_set_ime_param.in_width / 2;
	g_set_ime_param.out_p2_size_v  = g_set_ime_param.in_height / 2;
	g_set_ime_param.out_p2_lofs_y  = g_set_ime_param.in_width / 2;
	g_set_ime_param.out_p2_lofs_uv = g_set_ime_param.in_width / 2;

	g_set_ime_param.out_ref_size_h = g_set_ime_param.in_width;
	g_set_ime_param.out_ref_size_v = g_set_ime_param.in_height;
	g_set_ime_param.out_ref_lofs_y  = g_set_ime_param.in_width;
	g_set_ime_param.out_ref_lofs_uv = g_set_ime_param.in_width;

	g_set_ime_param.out_lca_size_h = g_set_ime_param.in_width >> 2;
	g_set_ime_param.out_lca_size_v = g_set_ime_param.in_height >> 2;
	g_set_ime_param.out_lca_lofs_y = ALIGN_CEIL_4(g_set_ime_param.out_lca_size_h * 3);
	g_set_ime_param.lca_in_src = KDRV_IME_LCA_IN_SRC_DIRECT;
	nvt_kdrv_ipp_ime_set_param(set_kdrv_id[nvt_kdrv_id_ime], &g_set_ime_param);
	DBG_IND("nvt_kdrv_ipp_ime_set_param...\r\n");


	if ((ime_lca_set_param_enable == TRUE) && (g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		// set ife2 parameter
		g_set_ife2_param.in_width = g_set_ime_param.out_lca_size_h;
		g_set_ife2_param.in_height = g_set_ime_param.out_lca_size_v;
		g_set_ife2_param.in_lofs = g_set_ime_param.out_lca_lofs_y;

		if ((g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_DIRECT)) {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DIRECT;
		} else if ((g_set_ime_param.lca_in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_ALL_DIRECT)) {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_ALL_DIRECT;
		} else {
			g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DRAM;
		}
		g_set_ife2_param.in_addr = out_addr0_lca_y;
		g_set_ife2_param.out_addr = out_addr1_lca_y;
		nvt_kdrv_ipp_ife2_set_param(set_kdrv_id[nvt_kdrv_id_ife2], &g_set_ife2_param);
	}

	nvt_kdrv_ipp_trigger();

	nvt_kdrv_ipp_wait_frame_end(FALSE);

	nvt_kdrv_ipp_pause();
	DBG_IND("nvt_kdrv_ipp_pause...\r\n");

	nvt_kdrv_ipp_dce_save_output(&g_set_dce_param);
	nvt_kdrv_ipp_ipe_save_output();
	nvt_kdrv_ipp_ime_save_output();

	kdrv_ipp_rel_buff(&dce_in_buf_y);
	kdrv_ipp_rel_buff(&dce_in_buf_uv);

	DBG_DUMP("test done.\r\n");

	return 0;
}

int nvt_kdrv_lca_test(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv)
{
#if 0
	VOS_FILE fp;
	int len = 0;

	UINT32 ime_in_size_h = 0, ime_in_size_v = 0;
	UINT32 ime_in_lofs_y = 0, ime_in_lofs_uv = 0;
	UINT32 ime_in_buf_size_y = 0, ime_in_buf_size_uv = 0;
	UINT32 ime_in_buf_addr_y = 0, ime_in_buf_addr_uv = 0;

	UINT32 ime_out_size_h = 0, ime_out_size_v = 0;
	UINT32 ime_out_lofs_y = 0, ime_out_lofs_uv = 0;
	UINT32 ime_out_buf_size_y = 0, ime_out_buf_size_uv = 0;
	UINT32 ime_out_buf_addr_y = 0, ime_out_buf_addr_uv = 0;

	UINT32 ime_in_lca_size_h = 0, ime_in_lca_size_v = 0;
	UINT32 ime_in_lca_lofs = 0;
	//UINT32 ime_in_lca_buf_size = 0;
	//UINT32 ime_in_lca_buf_addr[2] = {0};

	UINT32 ime_out_lca_size_h = 0, ime_out_lca_size_v = 0;
	UINT32 ime_out_lca_lofs = 0;
	UINT32 ime_out_lca_buf_size = 0;
	UINT32 ime_out_lca_buf_addr[2] = {0};

	//UINT32 ife2_in_buf_addr = 0, ife2_out_buf_addr = 0;


	//UINT32 dce_in_y_addr = 0;
	//UINT32 dce_in_uv_addr = 0;

	//UINT32 dce_in_y_buf_size = 0;
	//UINT32 dce_in_uv_buf_size = 0;

	IPP_BUFF_INFO get_buf_info = {0};

	char ime_in_y_raw[260] = "";
	char ime_in_uv_raw[260] = "";

	UINT32 frm = 0;

	DBG_IND("nvt_kdrv_lca_test: -begin\r\n");

	// Y/UV-Packed, 420
	ime_in_size_h = 1920;
	ime_in_size_v = 1080;
	ime_in_lofs_y = 1920;
	ime_in_lofs_uv = 1920;
	ime_in_buf_size_y = ime_in_lofs_y * ime_in_size_v;
	ime_in_buf_size_uv = ime_in_lofs_uv * (ime_in_size_v >> 1);

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_in_buf_size_y) != E_OK) {
		return -1;
	}
	ime_in_buf_addr_y = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_in_buf_addr_y: %08x\r\n", ime_in_buf_addr_y);

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_in_buf_size_uv) != E_OK) {
		return -1;
	}
	ime_in_buf_addr_uv = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_in_buf_addr_uv: %08x\r\n", ime_in_buf_addr_uv);

	// Y/UV-Packed, 420
	ime_out_size_h = 1920;
	ime_out_size_v = 1080;
	ime_out_lofs_y = 1920;
	ime_out_lofs_uv = 1920;
	ime_out_buf_size_y = ime_out_lofs_y * ime_out_size_v;
	ime_out_buf_size_uv = ime_out_lofs_uv * (ime_out_size_v >> 1);

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_out_buf_size_y) != E_OK) {
		return -1;
	}
	ime_out_buf_addr_y = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_out_buf_addr_y: %08x\r\n", ime_out_buf_addr_y);

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_out_buf_size_uv) != E_OK) {
		return -1;
	}
	ime_out_buf_addr_uv = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_out_buf_addr_uv: %08x\r\n", ime_out_buf_addr_uv);

	// YUV-Packed, 444
	ime_in_lca_size_h = 480;
	ime_in_lca_size_v = 270;
	ime_in_lca_lofs = (ime_in_lca_size_h * 3);
	//ime_in_lca_buf_size = ime_in_lca_lofs * ime_in_lca_size_v;
#if 0
	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_in_lca_buf_size) == E_OK) {
		ime_in_lca_buf_addr[0] = (UINT32)get_buf_info.vaddr;
		//DBG_IND("ime_in_lca_buf_addr[0]: %08x\r\n", ime_in_lca_buf_addr[0]);
	} else {
		return -1;
	};

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_in_lca_buf_size) == E_OK) {
		ime_in_lca_buf_addr[1] = (UINT32)get_buf_info.vaddr;
		//DBG_IND("ime_in_lca_buf_addr[1]: %08x\r\n", ime_in_lca_buf_addr[1]);
	} else {
		return -1;
	};
#endif

	// YUV-Packed, 444
	ime_out_lca_size_h = 480;
	ime_out_lca_size_v = 270;
	ime_out_lca_lofs = (ime_out_lca_size_h * 3);
	ime_out_lca_buf_size = ime_out_lca_lofs * ime_out_lca_size_v;
#if 1
	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_out_lca_buf_size) != E_OK) {
		return -1;
	}
	ime_out_lca_buf_addr[0] = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_out_lca_buf_addr[0]: %08x\r\n", ime_out_lca_buf_addr[0]);

	memset((void *)&get_buf_info, 0x0, sizeof(IPP_BUFF_INFO));
	if (kdrv_ipp_alloc_buff(&get_buf_info, ime_out_lca_buf_size) != E_OK) {
		return -1;
	}
	ime_out_lca_buf_addr[1] = (UINT32)get_buf_info.vaddr;
	//DBG_IND("ime_out_lca_buf_addr[1]: %08x\r\n", ime_out_lca_buf_addr[1]);
#endif




	nvt_kdrv_ipp_set_id();

	nvt_kdrv_ipp_open();
	DBG_IND("nvt_kdrv_ipp_open done.\r\n");

	DBG_IND("\r\n");

	for(frm = 0; frm < 6; frm++) {
		//DBG_IND("frame_num: %d, -begin\r\n", frm);

		sprintf(ime_in_y_raw, "/mnt/sd/img/1920x1080_lift0p8_02_%04d_y.raw", (int)(frm+1));
		sprintf(ime_in_uv_raw, "/mnt/sd/img/1920x1080_lift0p8_02_%04d_uv.raw", (int)(frm+1));

		nvt_kdrv_ipp_load_file(ime_in_y_raw, ime_in_buf_addr_y, ime_in_buf_size_y);
		nvt_kdrv_ipp_load_file(ime_in_uv_raw, ime_in_buf_addr_uv, ime_in_buf_size_uv);

		//CHKPNT;

		// set interrupt
		ime_interrupt_en_info = (KDRV_IME_INTE_FRM_START | KDRV_IME_INTE_FRM_END);
		kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_INTER, (void *)&ime_interrupt_en_info);

		//CHKPNT;

		//set input img parameters
		ime_in_img_info.in_src = KDRV_IME_IN_SRC_DRAM;
		ime_in_img_info.type = KDRV_IME_IN_FMT_Y_PACK_UV420;
		ime_in_img_info.ch[KDRV_IPP_YUV_Y].width = ime_in_size_h;
		ime_in_img_info.ch[KDRV_IPP_YUV_Y].height = ime_in_size_v;
		ime_in_img_info.ch[KDRV_IPP_YUV_Y].line_ofs = ime_in_lofs_y;
		ime_in_img_info.ch[KDRV_IPP_YUV_U].width = ime_in_size_h;
		ime_in_img_info.ch[KDRV_IPP_YUV_U].height = ime_in_size_v;
		ime_in_img_info.ch[KDRV_IPP_YUV_U].line_ofs = ime_in_lofs_uv;
		ime_in_img_info.dma_flush = KDRV_IME_DO_BUF_FLUSH;
		kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_IN_IMG, (void *)&ime_in_img_info);

		ime_in_addr.addr_y = ime_in_buf_addr_y;
		ime_in_addr.addr_cb = ime_in_buf_addr_uv;
		ime_in_addr.addr_cr = ime_in_buf_addr_uv;
		kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_IN_ADDR, (void *)&ime_in_addr);

		//CHKPNT;

		ime_extend_info.stripe_num = 1;     ///<stripe window number
		ime_extend_info.stripe_h_max = ime_in_size_h;   ///<max horizontal stripe size
		ime_extend_info.tmnr_en = 0;       ///<3dnr enable
		ime_extend_info.p1_enc_en = 0;      ///<output path1 encoder enable
		kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_extend_info);

		//CHKPNT;

		// output path1
		{
			ime_out_img_info[0].path_id = KDRV_IME_PATH_ID_1;
			ime_out_img_info[0].path_en = TRUE;
			ime_out_img_info[0].path_flip_en = FALSE;
			ime_out_img_info[0].type = KDRV_IME_OUT_FMT_Y_PACK_UV420;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].width = ime_out_size_h;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].height = ime_out_size_v;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_Y].line_ofs = ime_out_lofs_y;

			ime_out_img_info[0].ch[KDRV_IPP_YUV_U].width = ime_out_size_h;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_U].height = ime_out_size_v;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_U].line_ofs = ime_out_lofs_uv;

			ime_out_img_info[0].ch[KDRV_IPP_YUV_V].width = ime_out_size_h;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_V].height = ime_out_size_v;
			ime_out_img_info[0].ch[KDRV_IPP_YUV_V].line_ofs = ime_out_lofs_uv;

			ime_out_img_info[0].dma_flush = KDRV_IME_DO_BUF_FLUSH;

			ime_out_img_info[0].crp_window.x = 0;
			ime_out_img_info[0].crp_window.y = 0;
			ime_out_img_info[0].crp_window.w = ime_out_size_h;//Out_Data[i].SizeH;
			ime_out_img_info[0].crp_window.h = ime_out_size_v;//Out_Data[i].SizeV;
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&ime_out_img_info[0]);

			//set output path1 address
			ime_out_addr.path_id = KDRV_IME_PATH_ID_1;
			ime_out_addr.addr_info.addr_y  = (UINT32)ime_out_buf_addr_y;
			ime_out_addr.addr_info.addr_cb = (UINT32)ime_out_buf_addr_uv;
			ime_out_addr.addr_info.addr_cr = (UINT32)ime_out_buf_addr_uv;
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&ime_out_addr);
		}

		//CHKPNT;



		{
			// set lca subout
			ime_lcaout_img_info.enable = TRUE;
			//ime_lcaout_img_info.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
			ime_lcaout_img_info.sub_out_size.width = ime_out_lca_size_h;
			ime_lcaout_img_info.sub_out_size.height = ime_out_lca_size_v;
			ime_lcaout_img_info.sub_out_size.line_ofs = ime_out_lca_lofs;

			if ((frm & 0x00000001) == 0) {
				out_addr0_lca_y = ime_out_lca_buf_addr[0];
				ime_lcaout_img_info.sub_out_addr = out_addr0_lca_y;

				//DBG_IND("frame_num: %d,  lca_out_addr: %08x\r\n", (int)frm, (unsigned long)out_addr0_lca_y);
			} else {
				out_addr1_lca_y = ime_out_lca_buf_addr[1];
				ime_lcaout_img_info.sub_out_addr = out_addr1_lca_y;

				//DBG_IND("frame_num: %d,  lca_out_addr: %08x\r\n", (int)frm, out_addr1_lca_y);
			}

			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_LCA_SUB, (void *)&ime_lcaout_img_info);
		}

		//CHKPNT;

		if (frm >= 1)
		{
			ime_ipl_func_param.ipl_ctrl_func = 0;
			ime_ipl_func_param.enable = FALSE;

			// lca subin
			g_set_ime_param.lca_in_src = KDRV_IME_LCA_IN_SRC_DIRECT;

			ime_lcain_img_info.in_src = KDRV_IME_LCA_IN_SRC_DIRECT;
			ime_lcain_img_info.img_size.width = ime_in_lca_size_h;
			ime_lcain_img_info.img_size.height = ime_in_lca_size_v;
			ime_lcain_img_info.img_size.line_ofs = ime_in_lca_lofs;
			if ((frm & 0x00000001) == 1) {
				ime_lcain_img_info.dma_addr = out_addr0_lca_y;
			} else {
				ime_lcain_img_info.dma_addr = out_addr1_lca_y;
			}
			//DBG_IND("frame_num: %d,  lca_in_addr: %08x\r\n", frm, ime_lcain_img_info.dma_addr);
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_LCA_REF_IMG, (void *)&ime_lcain_img_info);


			ime_ipl_func_param.ipl_ctrl_func |= KDRV_IME_IQ_FUNC_LCA;
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_ALL_FUNC_EN, &ime_ipl_func_param);


			ime_lca_ctrl_param.enable = TRUE;
			ime_lca_ctrl_param.bypass = FALSE;
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_LCA_FUNC_EN, &ime_lca_ctrl_param);

			ime_lca_set_param_enable = TRUE;
			ime_lca_set_param.chroma.ref_y_wt = 31;   ///< Chroma reference weighting for Y channels, tag: 0,  src: 31
			ime_lca_set_param.chroma.ref_uv_wt = 1;  ///< Chroma reference weighting for UV channels. tag: 31, src: 0
			ime_lca_set_param.chroma.out_uv_wt = 31;  ///< Chroma adaptation output weighting
			ime_lca_set_param.chroma.y_rng = KDRV_IME_RANGE_32;
			ime_lca_set_param.chroma.y_wt_prc = KDRV_IME_RANGE_32;
			ime_lca_set_param.chroma.y_th = 0;
			ime_lca_set_param.chroma.y_wt_s = 32;
			ime_lca_set_param.chroma.y_wt_e = 0;

			ime_lca_set_param.chroma.uv_rng = KDRV_IME_RANGE_64;
			ime_lca_set_param.chroma.uv_wt_prc = KDRV_IME_RANGE_64;
			ime_lca_set_param.chroma.uv_th = 0;
			ime_lca_set_param.chroma.uv_wt_s = 64;
			ime_lca_set_param.chroma.uv_wt_e = 0;

			ime_lca_set_param.ca.enable = FALSE;
			ime_lca_set_param.luma.enable = FALSE;

			ime_lca_set_param.sub_out_src = KDRV_IME_LCA_SUBOUT_SRC_B;
			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IQ_LCA, (void *)&ime_lca_set_param);

			if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
				// set ife2 parameter
				g_set_ife2_param.in_width = ime_out_lca_size_h;
				g_set_ife2_param.in_height = ime_out_lca_size_v;
				g_set_ife2_param.in_lofs = ime_in_lca_lofs;

				if ((ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_DIRECT)) {
					g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DIRECT;
				} else if ((ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_ALL_DIRECT)) {
					g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_ALL_DIRECT;
				} else if  ((ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT) && (g_set_ime_param.ime_op_mode == KDRV_IME_IN_SRC_DRAM)) {
					g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DIRECT;
				} else {
					g_set_ife2_param.ife2_op_mode = KDRV_IFE2_OUT_DST_DRAM;
				}

				if ((frm & 0x00000001) == 1) {
					g_set_ife2_param.in_addr = out_addr0_lca_y;
					g_set_ife2_param.out_addr = out_addr0_lca_y;
				} else {
					g_set_ife2_param.in_addr = out_addr1_lca_y;
					g_set_ife2_param.out_addr = out_addr1_lca_y;
				}
				//DBG_IND("frame_num: %d,  ife2_in_addr: %08x\r\n", frm, g_set_ife2_param.in_addr);
				nvt_kdrv_ipp_ife2_set_param(set_kdrv_id[nvt_kdrv_id_ife2], &g_set_ife2_param);
			}
		} else {
			ime_ipl_func_param.ipl_ctrl_func = 0;
			ime_ipl_func_param.enable = FALSE;

			kdrv_ime_set(set_kdrv_id[nvt_kdrv_id_ime], KDRV_IME_PARAM_IPL_ALL_FUNC_EN, &ime_ipl_func_param);
		}

		//CHKPNT;

		//nvt_kdrv_ipp_trigger();
		kdrv_ime_trigger(set_kdrv_id[nvt_kdrv_id_ime], NULL, NULL, NULL);
		//DBG_IND("kdrv_ime_trigger, done\r\n");
		if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
			kdrv_ife2_trigger(set_kdrv_id[nvt_kdrv_id_ife2], NULL, NULL, NULL);
			//DBG_IND("kdrv_ife2_trigger, done\r\n");
		}

		//nvt_kdrv_ipp_wait_frame_end(FALSE);
		while (!nvt_kdrv_ime_fend_flag) {
			vos_task_delay_ms(1);
		}
		//DBG_IND("ime end\r\n");

		if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
			while (!nvt_kdrv_ife2_fend_flag) {
				vos_task_delay_ms(1);
			}
			//DBG_IND("ife2 end\r\n");
		}
		nvt_kdrv_ipp_clear_frame_end();

		//nvt_kdrv_ipp_pause();
		kdrv_ime_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
		if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
			kdrv_ife2_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
		}
		//DBG_IND("nvt_kdrv_ipp_pause...\r\n");



		//nvt_kdrv_ipp_ime_save_output();
		// save y to file
		sprintf(ime_out_p1_raw_file_y, "/mnt/sd/output/img_out_p1_y_%04d.raw", (int)frm);
		fp = vos_file_open(ime_out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_y);
			return FALSE;
		}
		len = vos_file_write(fp, (void *)ime_out_buf_addr_y, ime_out_buf_size_y);
		DBG_IND("ime: write file lenght: %d\r\n", len);
		vos_file_close(fp);

		// save uv to file
		sprintf(ime_out_p1_raw_file_u, "/mnt/sd/output/img_out_p1_uv_%04d.raw", (int)frm);
		fp = vos_file_open(ime_out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", ime_out_p1_raw_file_u);
			return FALSE;
		}
		len = vos_file_write(fp, (void *)ime_out_buf_addr_uv, ime_out_buf_size_uv);
		DBG_IND("ime: write file lenght: %d\r\n", len);
		vos_file_close(fp);

		//DBG_IND("frame_num: %d, -end\r\n\r\n\r\n", frm);

	}

	//nvt_kdrv_ipp_close();
	kdrv_ime_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
	if ((ime_lca_set_param_enable == TRUE) && (ime_lcain_img_info.in_src == KDRV_IME_LCA_IN_SRC_DIRECT)) {
		kdrv_ife2_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
	}
	//DBG_IND("nvt_kdrv_ipp_close done.\r\n");

	kdrv_ime_buf_uninit();
	kdrv_ife2_buf_uninit();
	//DBG_IND("nvt_kdrv_ipp_close done.\r\n");

	//DBG_IND("nvt_kdrv_lca_test: -end\r\n");
#endif

	return 0;
}
