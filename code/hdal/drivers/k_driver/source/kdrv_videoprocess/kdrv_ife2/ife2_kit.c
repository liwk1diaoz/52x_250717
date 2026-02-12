

#if defined (__UITRON) || defined (__ECOS)

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "Type.h"
#include "kernel.h"

#elif defined (__FREERTOS)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <malloc.h>


#include "kwrap/file.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"



extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);

#elif defined (__KERNEL__) || defined (__LINUX)
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <comm/nvtmem.h>

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include <kwrap/file.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "ife2_drv.h"

#endif


#include "ife2_dbg.h"

#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ife2.h"
#include "kdrv_videoprocess/kdrv_ipp.h"

#include "ife2_platform.h"
#include "ife2_kit.h"


//----------------------------------------------------------

#define IFE2_TOOL_KIT_EN    0


#if (IFE2_TOOL_KIT_EN == 1)

typedef struct _ife2_img_info_ {
	UINT32 in_img_size_h;
	UINT32 in_img_size_v;
	UINT32 in_img_ofs;

	UINT32 in_addr;
	UINT32 out_addr;
} ife2_img_info;




static UINT32 ife2_fmd_flag = 0;
INT32 kdrv_ife2_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IFE2_INT_FMD) {
		ife2_fmd_flag = 1;
	}
	return 0;
}




KDRV_IFE2_OPENCFG ife2_open_obj;
KDRV_IFE2_IN_INFO ife2_in_img_info;
KDRV_IFE2_OUT_IMG ife2_out_img_info;
//UINT32 in_addr = 0, out_addr = 0;

KDRV_IFE2_INTE ife2_interrupt_en;
KDRV_IFE2_GRAY_STATIST ife2_gray_stl;
KDRV_IFE2_REFCENT_PARAM ife2_refcen_info;
KDRV_IFE2_FILTER_PARAM ife2_filter_info;

#endif


#if defined (__KERNEL__) || defined (__LINUX)

#if (IFE2_TOOL_KIT_EN == 1)

char in_ife2_file[256] = "//mnt//sd//ife2//input//in_yuv.raw";
char out_ife2_file[256] = "//mnt//sd//ife2//output//out_yuv.raw";

struct nvt_fmem_mem_info_t ife2_in_buf_y = {0};
struct nvt_fmem_mem_info_t ife2_out_buf_y = {0};

#endif


int nvt_ife2_api_dump(PIFE2_INFO pife2_info, unsigned char argc, char **pargv)
{
	DBG_IND("Dump kdriver\r\n");
	kdrv_ife2_dump_info(printk);

	return 0;
}

int nvt_kdrv_ife2_test(PIFE2_INFO pife2_info, unsigned char argc, char **pargv)
{
#if (IFE2_TOOL_KIT_EN == 1)
	//mm_segment_t old_fs;
	//struct file *fp;
	VOS_FILE fp;
	int len = 0;
	int ret;


	void *hdl_in_buf_y = NULL;
	void *hdl_out_buf_y = NULL;


	UINT32 in_size_h = 480, in_size_v = 270;
	UINT32 in_lofs_y = (in_size_h * 3);//, in_lofs_uv = 640;
	UINT32 in_addr_y = 0;//, in_addr_u = 0, in_addr_v = 0;
	UINT32 in_buf_size_y = 0;//, in_buf_size_u = 0, in_buf_size_v = 0;

	int out_size_h = 480, out_size_v = 270;
	int out_lofs_y = (out_size_h * 3);//, out_lofs_uv = 640;
	UINT32 out_addr_y = 0;//, out_addr_u = 0, out_addr_v = 0;
	UINT32 out_buf_size_y = 0;//, out_buf_size_u = 0, out_buf_size_v = 0;


	INT32 handle;//, i;//, out_buf_alloc_cnt;

	UINT32 id;


	DBG_DUMP("ife2 d2d flow alloc memory and load test file\n");

	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0, 0);

	// input Y buffer
	in_buf_size_y = (in_lofs_y * in_size_v);
	ret = nvt_fmem_mem_info_init(&ife2_in_buf_y, NVT_FMEM_ALLOC_CACHE, in_buf_size_y, NULL);
	if (ret >= 0) {
		hdl_in_buf_y = nvtmem_alloc_buffer(&ife2_in_buf_y);
		in_addr_y = (UINT32)ife2_in_buf_y.vaddr;
	}
	DBG_DUMP("ife2_in_buf_y.vaddr = 0x%08x\r\n", (unsigned int)ife2_in_buf_y.vaddr);
#if 0
	ife2_in_buf_y.size = in_buf_size_y;
	ife2_in_buf_y.align = 64;      ///< address alignment
	ife2_in_buf_y.name = "ife2_in_buf_y";
	ife2_in_buf_y.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &ife2_in_buf_y);
	in_addr_y = (UINT32)ife2_in_buf_y.va_addr;
	DBG_DUMP("ife2_in_buf_y.va_addr = 0x%08x, ife2_in_buf_y.phy_addr = 0x%08x\r\n", (UINT32)ife2_in_buf_y.va_addr, ife2_in_buf_y.phy_addr);
#endif


	// output Y buffer
	out_buf_size_y = (out_lofs_y * out_size_v);
	ret = nvt_fmem_mem_info_init(&ife2_out_buf_y, NVT_FMEM_ALLOC_CACHE, out_buf_size_y, NULL);
	if (ret >= 0) {
		hdl_out_buf_y = nvtmem_alloc_buffer(&ife2_out_buf_y);
		out_addr_y = (UINT32)ife2_out_buf_y.vaddr;
	}
	DBG_DUMP("ife2_out_buf_y.vaddr = 0x%08x\r\n", (unsigned int)ife2_out_buf_y.vaddr);

#if 0
	ife2_out_buf_y.size = out_buf_size_y;
	ife2_out_buf_y.align = 64;     ///< address alignment
	ife2_out_buf_y.name = "ife2_out_buf_y";
	ife2_out_buf_y.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &ife2_out_buf_y);
	out_addr_y = (UINT32)ife2_out_buf_y.va_addr;
	DBG_DUMP("ife2_out_buf_y.va_addr = 0x%08x, ife2_out_buf_y.phy_addr = 0x%08x\r\n", (UINT32)ife2_out_buf_y.va_addr, ife2_out_buf_y.phy_addr);
#endif


	// load y file
	fp = vos_file_open(in_ife2_file, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_ife2_file);
		return -EFAULT;
	}
	len = vos_file_read(fp, (void *)in_addr_y, in_buf_size_y);
	DBG_DUMP("ife2: read file lenght: %d\r\n", (int)len);
	vos_file_close(fp);

#if 0
	KDRV_IFE2_SET_FP kdrv_ife2_set_fp[KDRV_IFE2_PARAM_MAX] = {
		kdrv_ife2_set_trig,
		kdrv_ife2_set_ife2_in_img_info,
		kdrv_ife2_set_in_addr,
		kdrv_ife2_set_ife2_out_img_info,
		kdrv_ife2_set_out_addr,
		kdrv_ife2_set_isrcb,
		kdrv_ife2_set_inte_en,
		kdrv_ife2_set_gray_stat,
		kdrv_ife2_set_refcent_param,
		kdrv_ife2_set_filter_param,
		kdrv_ife2_set_burst_param,
	} ;
#endif


	/******** ime d2d flow *********/
	DBG_DUMP("ife2 d2d flow start\n");
	/* open config */
	ife2_open_obj.ife2_clock_sel = 240;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OPENCFG, (void *)&ife2_open_obj);

	/*open*/

	handle = kdrv_ife2_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
	if (handle == -1) {
		DBG_DUMP("ife2 open fail...\r\n");
		return -1;
	}

	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_RESET, NULL);

	/* set isr */
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_ISR_CB, (void *)&kdrv_ife2_isr_cb);

	/* set interrupt */
	ife2_interrupt_en = KDRV_IFE2_INTE_FMD;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_INTER, (void *)&ife2_interrupt_en);

	ife2_in_img_info.ch.width = in_size_h;
	ife2_in_img_info.ch.height = in_size_v;
	ife2_in_img_info.ch.line_ofs = in_lofs_y;
	ife2_in_img_info.type = KDRV_IFE2_IN_FMT_PACK_YUV444;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_IMG_INFO, (void *)&ife2_in_img_info);
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_ADDR, (void *)&in_addr_y);

	ife2_out_img_info.out_dst = KDRV_IFE2_OUT_DST_DRAM;
	ife2_out_img_info.line_ofs = out_lofs_y;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO, (void *)&ife2_out_img_info);
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_ADDR, (void *)&out_addr_y);

	ife2_gray_stl.u_th0 = 125;
	ife2_gray_stl.u_th1 = 130;
	ife2_gray_stl.v_th0 = 125;
	ife2_gray_stl.v_th1 = 130;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_GRAY_STATIST, (void *)&ife2_gray_stl);


	ife2_refcen_info.refcent_y.rng_th[0] = 4;
	ife2_refcen_info.refcent_y.rng_th[1] = 8;
	ife2_refcen_info.refcent_y.rng_th[2] = 12;
	ife2_refcen_info.refcent_y.rng_wt[0] = 8;
	ife2_refcen_info.refcent_y.rng_wt[1] = 4;
	ife2_refcen_info.refcent_y.rng_wt[2] = 2;
	ife2_refcen_info.refcent_y.rng_wt[3] = 1;
	ife2_refcen_info.refcent_y.cent_wt = 16;    ///< Reference center weighting
	ife2_refcen_info.refcent_y.outl_dth = 0;  ///< Outlier difference threshold
	ife2_refcen_info.refcent_y.outl_th = 7;   ///< Reference center outlier threshold

	ife2_refcen_info.refcent_uv.rng_th[0] = 4;
	ife2_refcen_info.refcent_uv.rng_th[1] = 8;
	ife2_refcen_info.refcent_uv.rng_th[2] = 12;
	ife2_refcen_info.refcent_uv.rng_wt[0] = 8;
	ife2_refcen_info.refcent_uv.rng_wt[1] = 4;
	ife2_refcen_info.refcent_uv.rng_wt[2] = 2;
	ife2_refcen_info.refcent_uv.rng_wt[3] = 1;
	ife2_refcen_info.refcent_uv.cent_wt = 16;
	ife2_refcen_info.refcent_uv.outl_dth = 0;
	ife2_refcen_info.refcent_uv.outl_th = 7;

	//ife2_refcen_info.refcent_y.rng_th_addr = (UINT32)ref_cent_y_range_th;
	//ife2_refcen_info.refcent_y.rng_wt_addr = (UINT32)ref_cent_y_range_wet;
	//ife2_refcen_info.refcent_y.cent_wt = ref_cent_y_cnt_wet;
	//ife2_refcen_info.refcent_y.outl_dth = ref_cent_y_outlier_dth;
	//ife2_refcen_info.refcent_y.outl_th = ref_cent_y_outlier_th;

	//ife2_refcen_info.refcent_uv.rng_th_addr = (UINT32)ref_cent_uv_range_th;
	//ife2_refcen_info.refcent_uv.rng_wt_addr = (UINT32)ref_cent_uv_range_wet;
	//ife2_refcen_info.refcent_uv.cent_wt = ref_cent_uv_cnt_wet;
	//ife2_refcen_info.refcent_uv.outl_dth = ref_cent_uv_outlier_dth;
	//ife2_refcen_info.refcent_uv.outl_th = ref_cent_uv_outlier_th;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_REFCENT, (void *)&ife2_refcen_info);




	ife2_filter_info.set_y.filt_th[0] = 4;
	ife2_filter_info.set_y.filt_th[1] = 8;
	ife2_filter_info.set_y.filt_th[2] = 16;
	ife2_filter_info.set_y.filt_th[3] = 32;
	ife2_filter_info.set_y.filt_th[4] = 64;

	ife2_filter_info.set_y.filt_wt[0] = 16;
	ife2_filter_info.set_y.filt_wt[1] = 8;
	ife2_filter_info.set_y.filt_wt[2] = 4;
	ife2_filter_info.set_y.filt_wt[3] = 2;
	ife2_filter_info.set_y.filt_wt[4] = 1;
	ife2_filter_info.set_y.filt_wt[5] = 0;

	ife2_filter_info.set_u.filt_th[0] = 4;
	ife2_filter_info.set_u.filt_th[1] = 8;
	ife2_filter_info.set_u.filt_th[2] = 16;
	ife2_filter_info.set_u.filt_th[3] = 32;
	ife2_filter_info.set_u.filt_th[4] = 64;

	ife2_filter_info.set_u.filt_wt[0] = 16;
	ife2_filter_info.set_u.filt_wt[1] = 8;
	ife2_filter_info.set_u.filt_wt[2] = 4;
	ife2_filter_info.set_u.filt_wt[3] = 2;
	ife2_filter_info.set_u.filt_wt[4] = 1;
	ife2_filter_info.set_u.filt_wt[5] = 0;

	ife2_filter_info.set_v.filt_th[0] = 4;
	ife2_filter_info.set_v.filt_th[1] = 8;
	ife2_filter_info.set_v.filt_th[2] = 16;
	ife2_filter_info.set_v.filt_th[3] = 32;
	ife2_filter_info.set_v.filt_th[4] = 64;

	ife2_filter_info.set_v.filt_wt[0] = 16;
	ife2_filter_info.set_v.filt_wt[1] = 8;
	ife2_filter_info.set_v.filt_wt[2] = 4;
	ife2_filter_info.set_v.filt_wt[3] = 2;
	ife2_filter_info.set_v.filt_wt[4] = 1;
	ife2_filter_info.set_v.filt_wt[5] = 0;

	ife2_filter_info.enable = TRUE;
	ife2_filter_info.size = KDRV_IFE2_FLTR_SIZE_5x5;
	ife2_filter_info.edg_dir.hv_th = 128;
	ife2_filter_info.edg_dir.pn_th = 128;
	ife2_filter_info.edg_ker_size = KDRV_IFE2_EKNL_SIZE_5x5;
	//ife2_filter_info.set_y.filt_th_addr = (UINT32)filter_filter_set_y_filter_th;
	//ife2_filter_info.set_y.filt_wt_addr = (UINT32)filter_filter_set_y_filter_wet;
	//ife2_filter_info.set_u.filt_th_addr = (UINT32)filter_filter_set_u_filter_th;
	//ife2_filter_info.set_u.filt_wt_addr = (UINT32)filter_filter_set_u_filter_wet;
	//ife2_filter_info.set_v.filt_th_addr = (UINT32)filter_filter_set_v_filter_th;
	//ife2_filter_info.set_v.filt_wt_addr = (UINT32)filter_filter_set_v_filter_wet;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_FILTER, (void *)&ife2_filter_info);


	/* trigger */
	DBG_DUMP("ife2 d2d flow trigger\n");
	ife2_fmd_flag = 0;
	kdrv_ife2_trigger(id, NULL, NULL, NULL);
	//kdrv_ife2_set(handle, KDRV_IFE2_PARAM_IPL_TRIG, NULL);

	/*wait end*/
	DBG_DUMP("ife2 d2d flow wait end\n");
	while (!ife2_fmd_flag) {
		vos_util_delay_ms(10);
	}
	DBG_DUMP("ife2 d2d flow end\n");

	/* pause */
	kdrv_ife2_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);

	/* close */
	kdrv_ife2_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);

	/* save image */
	// save output to file
	fp = vos_file_open(out_ife2_file, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", out_ife2_file);
		return FALSE;
	}
	len = vos_file_write(fp, (void *)out_addr_y, out_buf_size_y);
	DBG_DUMP("ife2: write file lenght: %d\r\n", (int)len);
	vos_file_close(fp);

	DBG_DUMP("ife2 d2d flow free memory\n");
	/* free buffer */
	//frm_free_buf_ddr(ife2_in_buf_y.va_addr);
	//frm_free_buf_ddr(ife2_out_buf_y.va_addr);

	ret = nvtmem_release_buffer(hdl_in_buf_y);
	ret = nvtmem_release_buffer(hdl_out_buf_y);

	return len;
#else
	return 0;
#endif
}

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ife2_hw_power_saving(PIFE2_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		if (clk_get_phase(ife2_clk[0]) == 0) {
			clk_set_phase(ife2_clk[0], 1);
		}

		if (clk_get_phase(ife2_pclk[0]) == 0) {
			clk_set_phase(ife2_pclk[0], 1);
		}

	} else {
		if (clk_get_phase(ife2_clk[0]) == 1) {
			clk_set_phase(ife2_clk[0], 0);
		}

		if (clk_get_phase(ife2_pclk[0]) == 1) {
			clk_set_phase(ife2_pclk[0], 0);
		}
	}

	DBG_DUMP("ife2-mclk: 0xB0-bit14,  ife2-pclk: 0xC0-bit14\r\n");
	debug_dumpmem(0xf00200B0, 0x10);
	debug_dumpmem(0xf00200C0, 0x10);

	DBG_IND("end\r\n");

	return 0;
}

int nvt_ife2_fw_power_saving(PIFE2_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		fw_ife2_power_saving_en = TRUE;
	} else {
		fw_ife2_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");

	return 0;
}
#endif


#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

#if (IFE2_TOOL_KIT_EN == 1)

char in_ife2_file[256] = "/mnt/sd/ife2/input/in_yuv.raw";
char out_ife2_file[256] = "/mnt/sd/ife2/output/out_yuv.raw";


UINT32 *ife2_in_buf_y = NULL;
UINT32 *ife2_out_buf_y = NULL;

#endif

int nvt_ife2_api_dump(unsigned char argc, char **pargv)
{
	DBG_IND("Dump kdriver\r\n");
	kdrv_ife2_dump_info(printf);

	return 0;
}

int nvt_kdrv_ife2_test(unsigned char argc, char **pargv)
{
#if (IFE2_TOOL_KIT_EN == 1)
	//mm_segment_t old_fs;
	VOS_FILE fp;
	int len = 0;
	//int ret;


	//void *hdl_in_buf_y = NULL;
	//void *hdl_out_buf_y = NULL;


	UINT32 in_size_h = 480, in_size_v = 270;
	UINT32 in_lofs_y = (in_size_h * 3);//, in_lofs_uv = 640;
	UINT32 in_addr_y = 0;//, in_addr_u = 0, in_addr_v = 0;
	UINT32 in_buf_size_y = 0;//, in_buf_size_u = 0, in_buf_size_v = 0;

	int out_size_h = 480, out_size_v = 270;
	int out_lofs_y = (out_size_h * 3);//, out_lofs_uv = 640;
	UINT32 out_addr_y = 0;//, out_addr_u = 0, out_addr_v = 0;
	UINT32 out_buf_size_y = 0;//, out_buf_size_u = 0, out_buf_size_v = 0;


	INT32 handle;//, i;//, out_buf_alloc_cnt;

	UINT32 id;


	DBG_IND("ife2 d2d flow alloc memory and load test file\n");

	kdrv_ife2_init();
	ife2_platform_prepare_clk();

	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0, 0);

	// input Y buffer
	in_buf_size_y = (in_lofs_y * in_size_v);
	if (ife2_in_buf_y != NULL) {
		vPortFree((void *)ife2_in_buf_y);
		ife2_in_buf_y = NULL;
	}
	ife2_in_buf_y = (UINT32 *)pvPortMalloc((size_t)IFE2_ALIGN_CEIL_8(in_buf_size_y));

	if (ife2_in_buf_y != NULL) {
		in_addr_y = (UINT32)ife2_in_buf_y;
	}
	nvt_dbg(IND, "ife2_in_buf_y.vaddr = 0x%08x\r\n", (unsigned int)in_addr_y);


	// output Y buffer
	out_buf_size_y = (out_lofs_y * out_size_v);
	if (ife2_out_buf_y != NULL) {
		vPortFree((void *)ife2_out_buf_y);
		ife2_out_buf_y = NULL;
	}
	ife2_out_buf_y = (UINT32 *)pvPortMalloc((size_t)IFE2_ALIGN_CEIL_8(out_buf_size_y));

	if (ife2_out_buf_y != NULL) {
		out_addr_y = (UINT32)ife2_out_buf_y;
	}
	nvt_dbg(IND, "ife2_out_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_y);


	// load y file
	if (ife2_in_buf_y != NULL) {
		fp = vos_file_open(in_ife2_file, O_RDONLY, 0);
		if (fp == -1) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_ife2_file);
			return -1;
		}
		len = vos_file_read(fp, (void *)ife2_in_buf_y, in_buf_size_y);
		nvt_dbg(IND, "ife2: read file lenght: %d\r\n", (int)len);
		len = vos_file_close(fp);
		nvt_dbg(IND, "ife2: read file lenght: %d\r\n", (int)len);
	}



	/******** ime d2d flow *********/
	DBG_IND("ife2 d2d flow start\n");
	/* open config */
	ife2_open_obj.ife2_clock_sel = 240;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OPENCFG, (void *)&ife2_open_obj);

	/*open*/

	handle = kdrv_ife2_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);
	if (handle == -1) {
		DBG_IND("ife2 open fail...\r\n");
		return -1;
	}

	/* set isr */
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_ISR_CB, (void *)&kdrv_ife2_isr_cb);

	/* set interrupt */
	ife2_interrupt_en = KDRV_IFE2_INTE_FMD;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_INTER, (void *)&ife2_interrupt_en);

	ife2_in_img_info.ch.width = in_size_h;
	ife2_in_img_info.ch.height = in_size_v;
	ife2_in_img_info.ch.line_ofs = in_lofs_y;
	ife2_in_img_info.type = KDRV_IFE2_IN_FMT_PACK_YUV444;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_IMG_INFO, (void *)&ife2_in_img_info);
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_IN_ADDR, (void *)&in_addr_y);

	ife2_out_img_info.out_dst = KDRV_IFE2_OUT_DST_DRAM;
	ife2_out_img_info.line_ofs = out_lofs_y;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO, (void *)&ife2_out_img_info);
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IPL_OUT_ADDR, (void *)&out_addr_y);

	ife2_gray_stl.u_th0 = 125;
	ife2_gray_stl.u_th1 = 130;
	ife2_gray_stl.v_th0 = 125;
	ife2_gray_stl.v_th1 = 130;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_GRAY_STATIST, (void *)&ife2_gray_stl);


	ife2_refcen_info.refcent_y.rng_th[0] = 4;
	ife2_refcen_info.refcent_y.rng_th[1] = 8;
	ife2_refcen_info.refcent_y.rng_th[2] = 12;
	ife2_refcen_info.refcent_y.rng_wt[0] = 8;
	ife2_refcen_info.refcent_y.rng_wt[1] = 4;
	ife2_refcen_info.refcent_y.rng_wt[2] = 2;
	ife2_refcen_info.refcent_y.rng_wt[3] = 1;
	ife2_refcen_info.refcent_y.cent_wt = 16;    ///< Reference center weighting
	ife2_refcen_info.refcent_y.outl_dth = 0;  ///< Outlier difference threshold
	ife2_refcen_info.refcent_y.outl_th = 7;   ///< Reference center outlier threshold

	ife2_refcen_info.refcent_uv.rng_th[0] = 4;
	ife2_refcen_info.refcent_uv.rng_th[1] = 8;
	ife2_refcen_info.refcent_uv.rng_th[2] = 12;
	ife2_refcen_info.refcent_uv.rng_wt[0] = 8;
	ife2_refcen_info.refcent_uv.rng_wt[1] = 4;
	ife2_refcen_info.refcent_uv.rng_wt[2] = 2;
	ife2_refcen_info.refcent_uv.rng_wt[3] = 1;
	ife2_refcen_info.refcent_uv.cent_wt = 16;
	ife2_refcen_info.refcent_uv.outl_dth = 0;
	ife2_refcen_info.refcent_uv.outl_th = 7;

	//ife2_refcen_info.refcent_y.rng_th_addr = (UINT32)ref_cent_y_range_th;
	//ife2_refcen_info.refcent_y.rng_wt_addr = (UINT32)ref_cent_y_range_wet;
	//ife2_refcen_info.refcent_y.cent_wt = ref_cent_y_cnt_wet;
	//ife2_refcen_info.refcent_y.outl_dth = ref_cent_y_outlier_dth;
	//ife2_refcen_info.refcent_y.outl_th = ref_cent_y_outlier_th;

	//ife2_refcen_info.refcent_uv.rng_th_addr = (UINT32)ref_cent_uv_range_th;
	//ife2_refcen_info.refcent_uv.rng_wt_addr = (UINT32)ref_cent_uv_range_wet;
	//ife2_refcen_info.refcent_uv.cent_wt = ref_cent_uv_cnt_wet;
	//ife2_refcen_info.refcent_uv.outl_dth = ref_cent_uv_outlier_dth;
	//ife2_refcen_info.refcent_uv.outl_th = ref_cent_uv_outlier_th;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_REFCENT, (void *)&ife2_refcen_info);




	ife2_filter_info.set_y.filt_th[0] = 4;
	ife2_filter_info.set_y.filt_th[1] = 8;
	ife2_filter_info.set_y.filt_th[2] = 16;
	ife2_filter_info.set_y.filt_th[3] = 32;
	ife2_filter_info.set_y.filt_th[4] = 64;

	ife2_filter_info.set_y.filt_wt[0] = 16;
	ife2_filter_info.set_y.filt_wt[1] = 8;
	ife2_filter_info.set_y.filt_wt[2] = 4;
	ife2_filter_info.set_y.filt_wt[3] = 2;
	ife2_filter_info.set_y.filt_wt[4] = 1;
	ife2_filter_info.set_y.filt_wt[5] = 0;

	ife2_filter_info.set_u.filt_th[0] = 4;
	ife2_filter_info.set_u.filt_th[1] = 8;
	ife2_filter_info.set_u.filt_th[2] = 16;
	ife2_filter_info.set_u.filt_th[3] = 32;
	ife2_filter_info.set_u.filt_th[4] = 64;

	ife2_filter_info.set_u.filt_wt[0] = 16;
	ife2_filter_info.set_u.filt_wt[1] = 8;
	ife2_filter_info.set_u.filt_wt[2] = 4;
	ife2_filter_info.set_u.filt_wt[3] = 2;
	ife2_filter_info.set_u.filt_wt[4] = 1;
	ife2_filter_info.set_u.filt_wt[5] = 0;

	ife2_filter_info.set_v.filt_th[0] = 4;
	ife2_filter_info.set_v.filt_th[1] = 8;
	ife2_filter_info.set_v.filt_th[2] = 16;
	ife2_filter_info.set_v.filt_th[3] = 32;
	ife2_filter_info.set_v.filt_th[4] = 64;

	ife2_filter_info.set_v.filt_wt[0] = 16;
	ife2_filter_info.set_v.filt_wt[1] = 8;
	ife2_filter_info.set_v.filt_wt[2] = 4;
	ife2_filter_info.set_v.filt_wt[3] = 2;
	ife2_filter_info.set_v.filt_wt[4] = 1;
	ife2_filter_info.set_v.filt_wt[5] = 0;

	ife2_filter_info.enable = TRUE;
	ife2_filter_info.size = KDRV_IFE2_FLTR_SIZE_5x5;
	ife2_filter_info.edg_dir.hv_th = 128;
	ife2_filter_info.edg_dir.pn_th = 128;
	ife2_filter_info.edg_ker_size = KDRV_IFE2_EKNL_SIZE_5x5;
	//ife2_filter_info.set_y.filt_th_addr = (UINT32)filter_filter_set_y_filter_th;
	//ife2_filter_info.set_y.filt_wt_addr = (UINT32)filter_filter_set_y_filter_wet;
	//ife2_filter_info.set_u.filt_th_addr = (UINT32)filter_filter_set_u_filter_th;
	//ife2_filter_info.set_u.filt_wt_addr = (UINT32)filter_filter_set_u_filter_wet;
	//ife2_filter_info.set_v.filt_th_addr = (UINT32)filter_filter_set_v_filter_th;
	//ife2_filter_info.set_v.filt_wt_addr = (UINT32)filter_filter_set_v_filter_wet;
	kdrv_ife2_set(id, KDRV_IFE2_PARAM_IQ_FILTER, (void *)&ife2_filter_info);


	/* trigger */
	DBG_IND("ife2 d2d flow trigger\n");
	ife2_fmd_flag = 0;
	kdrv_ife2_trigger(id, NULL, NULL, NULL);
	//kdrv_ife2_set(handle, KDRV_IFE2_PARAM_IPL_TRIG, NULL);

	/*wait end*/
	DBG_IND("ife2 d2d flow wait end\n");
	while (!ife2_fmd_flag) {
		//msleep(10);
		vos_util_delay_ms(10);
	}
	DBG_IND("ife2 d2d flow end\n");

	/* pause */
	kdrv_ife2_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);



	/* save image */
	// save output to file
	fp = vos_file_open(out_ife2_file, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\n", out_ife2_file);
		return -1;
	}
	len = vos_file_write(fp, (void *)ife2_out_buf_y, out_buf_size_y);
	nvt_dbg(IND, "ife2: write file lenght: %d\r\n", (int)len);
	len = vos_file_close(fp);


	DBG_IND("ife2 d2d flow free memory\n");
	/* free buffer */
	if (ife2_in_buf_y != NULL) {
		vPortFree((void *)ife2_in_buf_y);
		ife2_in_buf_y = NULL;
	}

	if (ife2_out_buf_y != NULL) {
		vPortFree((void *)ife2_out_buf_y);
		ife2_out_buf_y = NULL;
	}

	//frm_free_buf_ddr(ife2_in_buf_y.va_addr);
	//frm_free_buf_ddr(ife2_out_buf_y.va_addr);

	//ret = nvtmem_release_buffer(hdl_in_buf_y);
	//ret = nvtmem_release_buffer(hdl_out_buf_y);

	/* close */
	kdrv_ife2_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IFE2_ENGINE0);

	kdrv_ife2_uninit();

	return len;
#else
	return 0;
#endif
}

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ife2_hw_power_saving(unsigned char argc, char **pargv)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	unsigned int enable;

	if (argc > 0) {
		enable = (unsigned int)atoi(pargv[0]);
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		pll_setClkAutoGating(IFE2_M_GCLK);
		pll_setPclkAutoGating(IFE2_GCLK);
	} else {
		pll_clearClkAutoGating(IFE2_M_GCLK);
		pll_clearPclkAutoGating(IFE2_GCLK);
	}

	DBG_IND("end\r\n");
#else
	DBG_WRN("Not defined in this platform!\r\n");
#endif

	return 0;
}

int nvt_ife2_fw_power_saving(unsigned char argc, char **pargv)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	unsigned int enable;

	if (argc > 0) {
		enable = (unsigned int)atoi(pargv[0]);
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		fw_ife2_power_saving_en = TRUE;
	} else {
		fw_ife2_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");
#else
	DBG_WRN("Not defined in this platform!\r\n");
#endif

	return 0;
}
#endif

#else
#endif

