#include "dce_kit.h"
#include "dce_platform.h"
#include "dce_lib.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include <kwrap/file.h>
#include <kwrap/task.h>

#if (defined __LINUX)
#include <comm/nvtmem.h>
#endif


#define DCE_TOOL_KIT_EN     0

int kdrv_api_dce_dump_info(unsigned char argc, char **pargv)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	kdrv_dce_dump_info(printf);
#else
	kdrv_dce_dump_info(printk);
#endif
	return 0;
}

int kdrv_api_dce_hw_power_saving(unsigned char argc, char **pargv)
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
		pll_setClkAutoGating(DCE_M_GCLK);
		pll_setPclkAutoGating(DCE_GCLK);
	} else {
		pll_clearClkAutoGating(DCE_M_GCLK);
		pll_clearPclkAutoGating(DCE_GCLK);
	}

	DBG_IND("end\r\n");
#else
	DBG_WRN("Not defined in this platform!\r\n");
#endif

	return 0;
}

int kdrv_api_dce_fw_power_saving(unsigned char argc, char **pargv)
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
		fw_power_saving_en = TRUE;
	} else {
		fw_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");
#else
	DBG_WRN("Not defined in this platform!\r\n");
#endif

	return 0;
}

// debug level
int kdrv_api_dce_set_dbg_level(unsigned char argc, char **pargv)
{
	unsigned int level;
	if (argc < 1) {
		DBG_DUMP("wrong argument:%d", argc);
		return -1;
	} else {
		sscanf(pargv[0], "%x", &level);
	}

	dce_debug_level = level;
	return 0;
}


#if (DCE_TOOL_KIT_EN == 1)

typedef enum {
	DCE_DMA_IN_Y,
	DCE_DMA_IN_UV,
	DCE_DMA_IN_WDR,
	DCE_DMA_IN_2DLUT,
	DCE_DMA_OUT_Y,
	DCE_DMA_OUT_UV,
	DCE_DMA_OUT_WDR,
	DCE_DMA_OUT_CFA,
	DCE_DMA_IO_MAX
} KDRV_DCE_DMAIO;

typedef struct {
	BOOL enable;
	UINT32 *vaddr;
	UINT32 *ori_vaddr;
	UINT32 *paddr;
	UINT32 size;
	void *handle;
} DCE_BUFF_INFO;

UINT32 g_test_g_lut[65] = {
	65535, 64828, 64235, 63662, 63107, 62569, 62047, 61540, 61046, 60566,
	60099, 59644, 59201, 58768, 58346, 57934, 57532, 57139, 56755, 56380,
	56013, 55653, 55302, 54958, 54621, 54290, 53967, 53650, 53339, 53034,
	52734, 52441, 52152, 51869, 51592, 51319, 51050, 50787, 50528, 50273,
	50023, 49776, 49534, 49296, 49061, 48830, 48603, 48379, 48158, 47941,
	47727, 47517, 47309, 47104, 46902, 46704, 46507, 46314, 46123, 45935,
	45749, 45566, 45385, 45207, 45031
};

static UINT32 kdrv_dce_fmd_flag = 0;
KDRV_DCE_INTE dce_inte_en = {0};
KDRV_DCE_IN_INFO in_info = {0};
KDRV_DCE_IMG_IN_DMA_INFO in_dma_info = {0};
KDRV_DCE_IMG_OUT_DMA_INFO out_dma_info = {0};
KDRV_DCE_STRIPE_INFO ipl_stripe = {0};
KDRV_DCE_EXTEND_INFO extend_info = {0};
KDRV_DCE_SUBIMG_IN_ADDR wdr_subin_addr = {0};
KDRV_DCE_SUBIMG_OUT_ADDR wdr_subout_addr = {0};
KDRV_DCE_WDR_SUBIMG_PARAM wdr_subimg_info = {0};
KDRV_DCE_DC_CAC_PARAM dc_cac_info = {0};
KDRV_DCE_TRIG_TYPE_INFO trig_info = {0};
KDRV_DCE_STRIPE_PARAM dce_stripe = {0};
KDRV_DCE_GDC_CENTER_INFO gdc_cent = {0};
KDRV_DCE_DC_SRAM_SEL dc_sram_sel = {0};
KDRV_DCE_CFA_PARAM cfa_info = {0};
KDRV_DCE_2DLUT_PARAM lut2d_info = {0};
KDRV_DCE_FOV_PARAM fov_info = {0};
KDRV_DCE_WDR_PARAM wdr_param = {0};
KDRV_DCE_HIST_PARAM hist_param = {0};
KDRV_DCE_CFA_SUBOUT_INFO cfa_subout_param = {0};
KDRV_DCE_PARAM_IPL_FUNC_EN ipl_all_func_en = {0};

DCE_BUFF_INFO dce_dma_io[DCE_DMA_IO_MAX] = {0};

INT32 KDRV_DCE_ISR_CB(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_DCE_INT_FMD) {
		kdrv_dce_fmd_flag = 1;
	}
	return 0;
}

ER kdrv_dce_alloc_buff(DCE_BUFF_INFO *buff, UINT32 req_size)
{
	UINT32 size_align_64 = KDRV_DCE_CEIL_64(req_size) + 64;
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	buff->ori_vaddr = buff->vaddr = (UINT32 *)malloc(size_align_64 * sizeof(UINT8));
	if (!VOS_IS_ALIGNED((UINT32)buff->vaddr)) {
		buff->vaddr = (UINT32 *)KDRV_DCE_CEIL_64((UINT32)buff->vaddr);
	}
	if (buff->vaddr == NULL) {
		DBG_ERR("get buffer fail, size = %u->%u\n", req_size, (unsigned int)size_align_64);
		return -E_NOMEM;
	}
#else
	int ret = 0;
	struct nvt_fmem_mem_info_t linux_buff = {0};
	ret = nvt_fmem_mem_info_init(&linux_buff, NVT_FMEM_ALLOC_CACHE, size_align_64, NULL);
	if (ret >= 0) {
		buff->handle = nvtmem_alloc_buffer(&linux_buff);//save handle to release it
		buff->ori_vaddr = buff->vaddr = (UINT32 *)linux_buff.vaddr;
		if (!VOS_IS_ALIGNED((UINT32)buff->vaddr)) {
			buff->vaddr = (UINT32 *)KDRV_DCE_CEIL_64((UINT32)buff->vaddr);
		}
		buff->paddr = (UINT32 *)dce_platform_va2pa((UINT32)buff->vaddr);
	} else {
		DBG_ERR("get buffer fail, size = %u\n", (unsigned int)size_align_64);
		return -EFAULT;
	}
#endif
	buff->enable = TRUE;
	/*
	 *DBG_IND("vaddr 0x%08x\r\n", (unsigned int)buff->vaddr);
	 *DBG_IND("paddr 0x%08x\r\n", (unsigned int)buff->paddr);
	 *DBG_IND("size 0x%08x\r\n", (unsigned int)size_align_64);
	 */

	return E_OK;
}

void kdrv_dce_rel_buff(DCE_BUFF_INFO *buff)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	free(buff->ori_vaddr);
#else
	int ret = 0;
	ret = nvtmem_release_buffer(buff->handle);
	buff->handle = NULL;
	if (ret < 0) {
		pr_info("line %d: error releasing buffer\n", __LINE__);
	}
#endif
	buff->enable = FALSE;
	buff->size = 0;
	buff->ori_vaddr = NULL;
	buff->vaddr = NULL;
	buff->paddr = NULL;
}

ER dce_buffer_allocate(UINT32 in_w, UINT32 in_h)
{
	/*get Y in buffer*/
	dce_dma_io[DCE_DMA_IN_Y].size = (((in_w * in_h) + 3) / 4) * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_IN_Y], dce_dma_io[DCE_DMA_IN_Y].size);

	/*get UV in buffer*/
	dce_dma_io[DCE_DMA_IN_UV].size = (((in_w * in_h) + 3) / 4) * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_IN_UV], dce_dma_io[DCE_DMA_IN_UV].size);

	/*get WDR in buffer*/
	dce_dma_io[DCE_DMA_IN_WDR].size = 16 * 9 * 2 * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_IN_WDR], dce_dma_io[DCE_DMA_IN_WDR].size);

	dce_dma_io[DCE_DMA_IN_2DLUT].size = 65 * 65 * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_IN_2DLUT], dce_dma_io[DCE_DMA_IN_2DLUT].size);

	/*get Y out buffer*/
	dce_dma_io[DCE_DMA_OUT_Y].size = (((in_w * in_h) + 3) / 4) * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_OUT_Y], dce_dma_io[DCE_DMA_OUT_Y].size);

	/*get UV out buffer*/
	dce_dma_io[DCE_DMA_OUT_UV].size = (((in_w * in_h) + 3) / 4) * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_OUT_UV], dce_dma_io[DCE_DMA_OUT_UV].size);

	/*get wdr out buffer*/
	dce_dma_io[DCE_DMA_OUT_WDR].size = 16 * 9 * 2 * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_OUT_WDR], dce_dma_io[DCE_DMA_OUT_WDR].size);

	/*get cfa out buffer*/
	dce_dma_io[DCE_DMA_OUT_CFA].size = (((in_w * in_h) + 3) / 8) * 4;
	kdrv_dce_alloc_buff(&dce_dma_io[DCE_DMA_OUT_CFA], dce_dma_io[DCE_DMA_OUT_CFA].size);

	return E_OK;
}

void dce_buffer_release(void)
{
	int i = 0;
	for (i = 0; i < DCE_DMA_IO_MAX; i++) {
		if (dce_dma_io[i].enable) {
			DBG_IND("free 0x%08x, size = 0x%08x\n", (unsigned int)dce_dma_io[i].ori_vaddr, (unsigned int)dce_dma_io[i].size);
			kdrv_dce_rel_buff(&dce_dma_io[i]);
		}
	}
}


int dce_load_input_data(char **pargv)
{
	VOS_FILE fp;
	char file_path[256] = "";
	int len = 0;
	char sd_root_path[256] = "/mnt/sd";

	/*load Y in data*/
	sprintf(file_path, "%s/%s.bin", sd_root_path, pargv[0]);
	fp = vos_file_open(file_path, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_read(fp, (void *)dce_dma_io[DCE_DMA_IN_Y].vaddr, dce_dma_io[DCE_DMA_IN_Y].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);


	/*load UV in data*/
	sprintf(file_path, "%s/%s.bin", sd_root_path, pargv[1]);
	fp = vos_file_open(file_path, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_read(fp, (void *)dce_dma_io[DCE_DMA_IN_UV].vaddr, dce_dma_io[DCE_DMA_IN_UV].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	/*load Wdr in data*/
#if 1
	if (strcmp(pargv[3], "X")) {
		sprintf(file_path, "%s/%s.bin", sd_root_path, pargv[3]);
		fp = vos_file_open(file_path, O_RDONLY, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return -1;
		}
		len = vos_file_read(fp, (void *)dce_dma_io[DCE_DMA_IN_WDR].vaddr, dce_dma_io[DCE_DMA_IN_WDR].size);
		DBG_IND("read length %d\r\n", (unsigned int)len);
		vos_file_close(fp);
	}
#endif

	/*load 2DLUT in data*/
	if (strcmp(pargv[2], "X")) {
		sprintf(file_path, "%s/%s.bin", sd_root_path, pargv[2]);
		fp = vos_file_open(file_path, O_RDONLY, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", file_path);
			return -1;
		}
		len = vos_file_read(fp, (void *)dce_dma_io[DCE_DMA_IN_2DLUT].vaddr, dce_dma_io[DCE_DMA_IN_2DLUT].size);
		DBG_IND("read length %d\r\n", (unsigned int)len);
		vos_file_close(fp);
	}

	return 0;
}

int dce_dump_output_data(char **pargv)
{
	VOS_FILE fp;
	char file_path[256] = "";
	int len = 0;
	char sd_root_path[256] = "/mnt/sd";

	/* save image */
	sprintf(file_path, "%s/y_out.bin", sd_root_path);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)dce_dma_io[DCE_DMA_OUT_Y].vaddr, dce_dma_io[DCE_DMA_OUT_Y].size);
	DBG_IND("write length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	sprintf(file_path, "%s/uv_out.bin", sd_root_path);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)dce_dma_io[DCE_DMA_OUT_UV].vaddr, dce_dma_io[DCE_DMA_OUT_UV].size);
	DBG_IND("write length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	sprintf(file_path, "%s/wdr_out.bin", sd_root_path);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)dce_dma_io[DCE_DMA_OUT_WDR].vaddr, dce_dma_io[DCE_DMA_OUT_WDR].size);
	DBG_IND("write length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	sprintf(file_path, "%s/cfa_out.bin", sd_root_path);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)dce_dma_io[DCE_DMA_OUT_CFA].vaddr, dce_dma_io[DCE_DMA_OUT_CFA].size);
	DBG_IND("write length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	return len;
}



#endif


int kdrv_api_dce_test(unsigned char argc, char **pargv)
{
#if (DCE_TOOL_KIT_EN == 1)
	ER rt = 0;
	int len = 0;
	unsigned long in_w, in_h;
	INT32 id = 0, chip = KDRV_CHIP0, engine = KDRV_VIDEOPROCS_DCE_ENGINE0, channel = 0;
	KDRV_DCE_OPENCFG dce_open_obj;
	KDRV_DCE_IN_IMG_INFO in_img_info = {0};
	KDRV_DCE_OUT_IMG_INFO out_img_info = {0};
	DCE_BUFF_INFO init_dce_buff = {0};
	INT32 i;
	UINT32 *p_2dlut = NULL;
	UINT32 drv_buf_size = 0;

	drv_buf_size = kdrv_dce_buf_query(KDRV_IPP_CBUF_MAX_NUM);
	if (kdrv_dce_alloc_buff(&init_dce_buff, drv_buf_size) == E_OK) {
		kdrv_dce_buf_init((UINT32)init_dce_buff.vaddr, KDRV_IPP_CBUF_MAX_NUM);
	}

	DBG_IND("argc %d\r\n", argc);
	for (i = 0; i < argc; i++) {
		DBG_IND("pargv[%d] = %s\r\n", (unsigned int)i, pargv[i]);
	}

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	in_w = (unsigned long)atoi(pargv[4]);
	in_h = (unsigned long)atoi(pargv[5]);
#else
	kstrtoul(pargv[4], 0, &in_w);
	kstrtoul(pargv[5], 0, &in_h);
#endif
	DBG_IND("input data %s %s %s %s %d %d\n", pargv[0], pargv[1], pargv[2], pargv[3], (int)in_w, (int)in_h);


	DBG_IND("buffer allocate\n");
	rt = dce_buffer_allocate(in_w, in_h);
	if (rt != E_OK) {
		return rt;
	}

	for (i = 0; i < DCE_DMA_IO_MAX; i++) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		nvt_dbg(IND, "[%d]:va_addr = 0x%08x, size = 0x%08x\r\n", (unsigned int) i, (unsigned int)dce_dma_io[i].vaddr, (unsigned int)dce_dma_io[i].size);
#else
		nvt_dbg(IND, "[%d]:va_addr = 0x%08x, phy_addr = 0x%08x, size = 0x%08x\r\n", (unsigned int) i, (unsigned int)dce_dma_io[i].vaddr, (unsigned int)dce_dma_io[i].paddr, (unsigned int)dce_dma_io[i].size);
#endif
	}

	DBG_IND("load input data\n");
	len = dce_load_input_data(pargv);
	if (len == -1) {
		goto EOP;
	}

	/******** dce d2d flow *********/
	DBG_IND("dce d2d flow start\n");
	id = KDRV_DEV_ID(chip, engine, channel);
	/*open*/
	dce_open_obj.dce_clock_sel = 240;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_OPENCFG, (void *)&dce_open_obj);
	if (kdrv_dce_open(chip, engine) != 0) {
		DBG_ERR("set opencfg fail!\r\n");
		goto EOP;
	}

	DBG_IND("kdrv_dce_open\r\n");

	/*set input information*/
	in_info.in_src = KDRV_DCE_IN_SRC_DRAM;
	in_info.type = KDRV_DCE_D2D_FMT_Y_PACK_UV422;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&in_info);
	DBG_IND("set KDRV_DCE_PARAM_IPL_IN_INFO\r\n");

#if 0
	dce_stripe.stripe_type = KDRV_DCE_STRIPE_CUSTOMER;
	for (i = 0; i < 16; i++) {
		dce_stripe.hstp[i] = 64;
	}
#else
	dce_stripe.stripe_type = KDRV_DCE_STRIPE_AUTO;
#endif
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_STRIPE_PARAM, (void *)&dce_stripe);
	DBG_IND("set KDRV_DCE_PARAM_IQ_STRIPE_PARAM\r\n");

	/*set input img parameters*/
	in_img_info.img_size_h = in_w;
	in_img_info.img_size_v = in_h;
	in_img_info.lineofst[KDRV_DCE_YUV_Y] = ((in_w + 3) / 4) * 4;
	in_img_info.lineofst[KDRV_DCE_YUV_UV] = ((in_w + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
	DBG_IND("set KDRV_DCE_PARAM_IPL_IN_IMG\r\n");


	/*set input address*/
	in_dma_info.addr_y = (UINT32)dce_dma_io[DCE_DMA_IN_Y].vaddr;
	in_dma_info.addr_cbcr = (UINT32)dce_dma_io[DCE_DMA_IN_UV].vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IMG_DMA_IN, (void *)&in_dma_info);
	DBG_IND("set KDRV_DCE_PARAM_IPL_IMG_DMA_IN\r\n");

	/*set output img parameters*/
	out_img_info.crop_start_h = 0;
	out_img_info.crop_start_v = 0;
	out_img_info.crop_size_h = in_w;
	out_img_info.crop_size_v = in_h;
	out_img_info.lineofst[KDRV_DCE_YUV_Y] = ((in_w + 3) / 4) * 4;
	out_img_info.lineofst[KDRV_DCE_YUV_UV] = ((in_w + 3) / 4) * 4;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
	DBG_IND("KDRV_DCE_PARAM_IPL_OUT_IMG\r\n");

	/*set output address*/
	out_dma_info.addr_y = (UINT32)dce_dma_io[DCE_DMA_OUT_Y].vaddr;
	out_dma_info.addr_cbcr = (UINT32)dce_dma_io[DCE_DMA_OUT_UV].vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_IMG_DMA_OUT, (void *)&out_dma_info);
	DBG_IND("KDRV_DCE_PARAM_IPL_IMG_DMA_OUT\r\n");

	/*set wdr subimg input address*/
	wdr_subin_addr.addr = (UINT32)dce_dma_io[DCE_DMA_IN_WDR].vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBIN, (void *)&wdr_subin_addr);
	DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBIN\r\n");

	/*set wdr subimg output address*/
	wdr_subout_addr.enable = FALSE;//TRUE;
	wdr_subout_addr.addr = (UINT32)dce_dma_io[DCE_DMA_OUT_WDR].vaddr;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DMA_SUBOUT, (void *)&wdr_subout_addr);
	DBG_IND("KDRV_DCE_PARAM_IPL_DMA_SUBOUT\r\n");

	/*set CFA parameteres*/
	cfa_info.cfa_enable = FALSE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_CFA, (void *)&cfa_info);
	DBG_IND("KDRV_DCE_PARAM_IQ_CFA\r\n");

	/*set Cfa subout parameteres*/
	cfa_subout_param.cfa_subout_enable = FALSE;//TRUE;
	cfa_subout_param.cfa_subout_flip_enable = FALSE;
	cfa_subout_param.cfa_addr = (UINT32)dce_dma_io[DCE_DMA_OUT_CFA].vaddr;
	cfa_subout_param.cfa_lofs = ((in_w + 3) / 8) * 4;
	cfa_subout_param.subout_ch_sel = KDRV_DCE_CFA_SUBOUT_CH1;
	cfa_subout_param.subout_byte = KDRV_DCE_CFA_SUBOUT_1BYTE;
	cfa_subout_param.subout_shiftbit = 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_CFAOUT, (void *)&cfa_subout_param);

	/*set WDR parameteres*/
	kdrv_dce_get(id, KDRV_DCE_PARAM_IQ_WDR, (void *)&wdr_param);
	wdr_param.wdr_enable = TRUE;
	wdr_param.tonecurve_enable = TRUE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR, (void *)&wdr_param);

	ipl_all_func_en.enable = wdr_param.wdr_enable = TRUE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_WDR;
	ipl_all_func_en.enable = wdr_param.tonecurve_enable = TRUE;
	ipl_all_func_en.ipl_ctrl_func |= KDRV_DCE_IQ_FUNC_TONECURVE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set WDR subimg parameteres*/
	wdr_subimg_info.subimg_size_h = 16;
	wdr_subimg_info.subimg_size_v = 9;
	wdr_subimg_info.subimg_lofs_in = 16 * 4 * 2;
	wdr_subimg_info.subimg_lofs_out = 16 * 4 * 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_WDR_SUBIMG, (void *)&wdr_subimg_info);

	/*set Histogram parameteres*/
	ipl_all_func_en.enable = hist_param.hist_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_HISTOGRAM;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set GDC center*/
	gdc_cent.geo_center_x = in_w / 2;
	gdc_cent.geo_center_y = in_h / 2;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_GDC_CENTER, (void *)&gdc_cent);
	DBG_IND("KDRV_DCE_PARAM_IPL_GDC_CENTER\r\n");

	/*set DC/CAC parameters*/
	dc_sram_sel.sram_mode = KDRV_DCE_SRAM_DCE;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_DC_SRAM_MODE, (void *)&dc_sram_sel);

	ipl_all_func_en.enable = dc_cac_info.dc_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_DC;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	dc_cac_info.dc.geo_dist_x = 4095;
	dc_cac_info.dc.geo_dist_y = 4095;
	memcpy(dc_cac_info.dc.geo_lut_g, g_test_g_lut, GDC_TABLE_NUM * 4);
	ipl_all_func_en.enable = dc_cac_info.cac.cac_enable = FALSE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_CAC;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	dc_cac_info.cac.cac_sel = 0;
	dc_cac_info.cac.geo_r_lut_gain = 4000;
	dc_cac_info.cac.geo_g_lut_gain = 4096;
	dc_cac_info.cac.geo_b_lut_gain = 4200;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IQ_DC_CAC, (void *)&dc_cac_info);
	DBG_IND("KDRV_DCE_PARAM_IQ_DC_CAC\r\n");

	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_DC;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	/*set 2DLUT parameters*/
	ipl_all_func_en.enable = lut2d_info.lut_2d_enable = TRUE;
	ipl_all_func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_2DLUT;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_all_func_en);

	lut2d_info.lut_num_select = KDRV_DCE_2DLUT_65_65_GRID;
	lut2d_info.lut_2d_y_dist_off = 0;
	p_2dlut = (UINT32 *)dce_dma_io[DCE_DMA_IN_2DLUT].vaddr;
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

	kdrv_dce_fmd_flag = 0;
	/*set call back funcion*/
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_ISR_CB, (void *)&KDRV_DCE_ISR_CB);
	DBG_IND("KDRV_DCE_PARAM_IPL_ISR_CB\r\n");

	/*trig stripe calculation*/
	trig_info.opmode = KDRV_DCE_OP_MODE_CAL_STRIP;
	kdrv_dce_trigger(id, NULL, NULL, (void *)&trig_info);
	DBG_IND("KDRV_DCE_OP_MODE_CAL_STRIP\r\n");

	/*set interrupt enable*/
	dce_inte_en = KDRV_DCE_INTE_ALL;
	kdrv_dce_set(id, KDRV_DCE_PARAM_IPL_INTER, (void *)&dce_inte_en);
	DBG_IND("set KDRV_DCE_PARAM_IPL_INTER\r\n");

	/*trig dce start*/
	kdrv_dce_fmd_flag = 0;
	trig_info.opmode = KDRV_DCE_OP_MODE_NORMAL;
	kdrv_dce_trigger(id, NULL, NULL, (void *)&trig_info);
	DBG_IND("KDRV_DCE_OP_MODE_NORMAL\r\n");

	while (!kdrv_dce_fmd_flag) {
		vos_task_delay_ms(10);
	}

	kdrv_dce_pause(chip, engine);

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	//kdrv_dce_dump_info(printf);
#else
	//kdrv_dce_dump_info(printk);
#endif

	/*close*/
	kdrv_dce_close(chip, engine);
	DBG_IND("dce d2d flow end\n");
	/******** dce d2d flow *********/

	DBG_IND("dump output data\n");
	len = dce_dump_output_data(pargv);
	if (len == -1) {
		goto EOP;
	}

EOP:

	/* free buffer */
	dce_buffer_release();

	return len;
#else
	return 0;
#endif
}

