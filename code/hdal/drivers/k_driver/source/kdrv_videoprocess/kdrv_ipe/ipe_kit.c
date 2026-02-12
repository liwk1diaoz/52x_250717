

#include "ipe_kit.h"
#include "ipe_platform.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include <kwrap/file.h>

#define IPE_TOOL_KIT_EN    0


#define VERF_IQ_ALL 0

typedef enum {
	IPE_DMA_IN_Y,
	IPE_DMA_IN_UV,
	IPE_DMA_IN_SUBIMG,
	IPE_DMA_OUT_Y,
	IPE_DMA_OUT_UV,
	IPE_DMA_OUT_SUBIMG,
	IPE_DMA_OUT_VA,
	IPE_DMA_IO_MAX
} KDRV_IPE_DMAIO;

typedef struct {
	BOOL enable;
	UINT32 *vaddr;
	UINT32 *paddr;
	UINT32 size;
} IPE_OUT_DMA_INFO;

#if (IPE_TOOL_KIT_EN == 1)

static KDRV_IPE_OPENCFG kdrv_ipe_open_obj;
static KDRV_IPE_IN_IMG_INFO in_img_info = {0};
static KDRV_IPE_OUT_IMG_INFO out_img_info = {0};
static KDRV_IPE_DMA_ADDR_INFO in_addr_info = {0};
static KDRV_IPE_DMA_SUBIN_ADDR subin_addr_info = {0};
static KDRV_IPE_DMA_ADDR_INFO out_addr_info = {0};
static KDRV_IPE_DMA_SUBOUT_ADDR subout_addr_info = {0};
static KDRV_IPE_SUBIMG_PARAM subimg_info = {0};
static KDRV_IPE_INTE  inte_en = {0};
static KDRV_IPE_ETH_INFO eth_info = {0};
static KDRV_IPE_EEXT_PARAM eext_info = {0};
static KDRV_IPE_EDGE_OVERSHOOT_PARAM overshoot_info = {0};
static KDRV_IPE_EEXT_TONEMAP_PARAM eext_tonemap_info = {0};
static KDRV_IPE_RGBLPF_PARAM rgblpf_info = {0};
static KDRV_IPE_PFR_PARAM  pfr_info = {0};
static KDRV_IPE_CC_EN_PARAM cc_en_info = {0};
static KDRV_IPE_CC_PARAM cc_info = {0};
static KDRV_IPE_CCM_PARAM ccm_info = {0};
static KDRV_IPE_CCTRL_EN_PARAM cctrl_en_info = {0};
static KDRV_IPE_CADJ_EE_PARAM cadj_ee_info = {0};
static KDRV_IPE_CADJ_YCCON_PARAM cadj_yccon_info = {0};
static KDRV_IPE_CADJ_COFS_PARAM cadj_cofs_info = {0};
static KDRV_IPE_CADJ_RAND_PARAM cadj_rand_info = {0};
static KDRV_IPE_CADJ_HUE_PARAM cadj_hue_info = {0};
static KDRV_IPE_CADJ_FIXTH_PARAM cadj_fixth_info = {0};
static KDRV_IPE_CADJ_MASK_PARAM cadj_mask_info = {0};
static KDRV_IPE_CST_PARAM cst_info = {0};
static KDRV_IPE_CSTP_PARAM cstp_info = {0};
static KDRV_IPE_GAMYRAND_PARAM gamyrand_info = {0};
static KDRV_IPE_GAMMA_PARAM gamma_info = {0};
static KDRV_IPE_YCURVE_PARAM ycurve_info = {0};
static KDRV_IPE_DEFOG_PARAM dfg_info  = {0};
static KDRV_IPE_LCE_PARAM lce_info  = {0};
static KDRV_IPE_EDGEDBG_PARAM edgdbg_info  = {0};
static KDRV_IPE_VA_PARAM va_param  = {0};
static KDRV_IPE_DMA_VA_ADDR va_addr_info  = {0};
static KDRV_IPE_VA_WIN_INFO va_win_info  = {0};
static KDRV_IPE_PARAM_IPL_FUNC_EN ipl_en_info = {0};

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
#else
static void *handle_yin_buff = NULL;
static void *handle_yout_buff = NULL;
static void *handle_uvin_buff = NULL;
static void *handle_uvout_buff = NULL;
static void *handle_subin_buff = NULL;
static void *handle_subout_buff = NULL;
static void *handle_va_buff = NULL;

struct nvt_fmem_mem_info_t y_in_buf_info = {0};
struct nvt_fmem_mem_info_t y_out_buf_info = {0};
struct nvt_fmem_mem_info_t uv_in_buf_info = {0};
struct nvt_fmem_mem_info_t uv_out_buf_info = {0};
struct nvt_fmem_mem_info_t subin_buf_info = {0};
struct nvt_fmem_mem_info_t subout_buf_info = {0};
struct nvt_fmem_mem_info_t va_buf_info = {0};
#endif


IPE_OUT_DMA_INFO ipe_dma_io[IPE_DMA_IO_MAX] = {0};

UINT16 gamma_day[129] = {
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

UINT16 y_curve[129] = { //linear
	0, 8, 16, 24, 32, 40, 48, 56, 64, 72,
	80, 88, 96, 104, 112, 120, 128, 136, 144, 152,
	160, 168, 176, 184, 192, 200, 208, 216, 224, 232,
	240, 248, 256, 264, 272, 280, 288, 296, 304, 312,
	320, 328, 336, 344, 352, 360, 368, 376, 384, 392,
	400, 408, 416, 424, 432, 440, 448, 456, 464, 472,
	480, 488, 496, 504, 512, 520, 528, 536, 544, 552,
	560, 568, 576, 584, 592, 600, 608, 616, 624, 632,
	640, 648, 656, 664, 672, 680, 688, 696, 704, 712,
	720, 728, 736, 744, 752, 760, 768, 776, 784, 792,
	800, 808, 816, 824, 832, 840, 848, 856, 864, 872,
	880, 888, 896, 904, 912, 920, 928, 936, 944, 952,
	960, 968, 976, 984, 992, 1000, 1008, 1016, 1023
};


#endif


//static KDRV_IPE_EPROC_PARAM eproc_info = {0};//

//static KDRV_IPE_3DCC_PARAM iq_3dcc_info = {0};




// debug cmd
UINT32 ipe_dbg_gamm_read_en = 0;
int kdrv_api_ipe_dump_info(unsigned char argc, char **pargv)
{
#if (IPE_TOOL_KIT_EN == 1)

	unsigned int func_en;

	if (argc >= 1) {
		sscanf(pargv[0], "%x", &func_en);
		DBG_DUMP("---- IPE gamma dump enable = 0x%.8x ----\r\n", func_en);
		ipe_dbg_gamm_read_en = func_en;
	}
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	kdrv_ipe_dump_info(printf);
#else
	kdrv_ipe_dump_info(printk);
#endif

#endif

	return 0;
}

int kdrv_api_ipe_set_dbg_level(unsigned char argc, char **pargv)
{
#if (IPE_TOOL_KIT_EN == 1)

	unsigned int level;
	if (argc < 1) {
		DBG_DUMP("wrong argument:%d", argc);
		return -1;
	} else {
		sscanf(pargv[0], "%x", &level);
	}

	DBG_DUMP("dbg_lv = %d\r\n", level);
	ipe_debug_level = level;

#endif

	return 0;
}

// debug cmd
int kdrv_api_ipe_set_dbg_cmd(unsigned char argc, char **pargv)
{
#if (IPE_TOOL_KIT_EN == 1)

	unsigned int cmd;
	if (argc < 1) {
		DBG_DUMP("wrong argument:%d", argc);
		cmd = IPE_DBG_DEFAULTON;
	} else {
		sscanf(pargv[0], "%x", &cmd);
		cmd = (IPE_DBG_DEFAULTON | cmd);
	}

	DBG_DUMP("dbg_opt = \r\n");
	DBG_DUMP(" | 0x00000001 = VA         | 0x00000002 = DEFOG     | 0x00000004 = CCM   | 0x00000008 = EEXT  | \r\n");
	DBG_DUMP(" | 0x10000000 = DEFAULTON  | \r\n");
	DBG_DUMP("set dbg option(%x) \n", (unsigned int)cmd);

	ipe_dbg_cmd = cmd;
#endif

	return 0;
}

UINT32 ipe_dbg_en = 0;
unsigned int ipe_argv[3] = {0};

#if (IPE_TOOL_KIT_EN == 1)
//iq debug
int kdrv_api_ipe_iq_debug(unsigned char argc, char **pargv)
{
	unsigned int func_en;

	if (argc >= 1) {
		sscanf(pargv[0], "%x", &func_en);

		DBG_DUMP("---- IPE debug iq enable = 0x%.8x ----\r\n", func_en);

		//LCE diff wt
		if ((func_en == 1) && (argc >= 4)) {
			sscanf(pargv[1], "%d", &ipe_argv[0]);
			sscanf(pargv[2], "%d", &ipe_argv[1]);
			sscanf(pargv[3], "%d", &ipe_argv[2]);
			DBG_IND("---- IPE diff wt_neg  %d ----\r\n", ipe_argv[0]);
			DBG_IND("---- IPE diff wt_avg  %d ----\r\n", ipe_argv[1]);
			DBG_IND("---- IPE diff wt_pos  %d ----\r\n", ipe_argv[2]);
		}

		//edge gamma
		if ((func_en == 2) && (argc >= 2)) {
			sscanf(pargv[1], "%d", &ipe_argv[0]);
			DBG_IND("---- IPE edge gammpa  selection %d --\r\n", ipe_argv[0]);
		}

		//rgb lpf
		if ((func_en == 4) && (argc >= 4)) {
			sscanf(pargv[1], "%d", &ipe_argv[0]);
			sscanf(pargv[2], "%d", &ipe_argv[1]);
			DBG_IND("---- IPE RGBLPF th1 %d --\r\n", ipe_argv[0]);
			DBG_IND("---- IPE RGBLPF th2 %d --\r\n", ipe_argv[1]);
		}

		ipe_dbg_en = func_en;
	} else {
		DBG_IND("---- IPE iq test command ----\r\n");
		DBG_IND("---- echo iq func (0: none, 1: lce wt, 2: edge gamma, 3: rgblpf) ----\r\n");
		DBG_IND("---- echo iq 1 wt_neg, wt_avg, wt_pos ----\r\n");
		DBG_IND("---- echo iq 2 sel (edge gamma 0: as gamma, 1: modified) ----\r\n");
		DBG_IND("---- echo iq 3 rth1, rth2 ----\r\n");
	}

	return 0;
}
#endif

#if (IPE_TOOL_KIT_EN == 1)

static UINT32 kdrv_ipe_fmd_flag = 0;
INT32 KDRV_IPE_ISR_CB(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_IPE_INT_FMD) {
		kdrv_ipe_fmd_flag = 1;
	}
	return 0;
}


ER ipe_buffer_allocate(UINT32 in_w, UINT32 in_h)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	/*get Y in buffer*/
	ipe_dma_io[IPE_DMA_IN_Y].size = in_w * in_h;
	ipe_dma_io[IPE_DMA_IN_Y].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_IN_Y].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_IN_Y].vaddr == NULL) {
		DBG_ERR("get input y buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_IN_Y].size);
		return -E_NOMEM;
	}

	/*get UV in buffer*/
	ipe_dma_io[IPE_DMA_IN_UV].size = in_w * in_h * 2;
	ipe_dma_io[IPE_DMA_IN_UV].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_IN_UV].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_IN_UV].vaddr == NULL) {
		DBG_ERR("get input uv buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_IN_UV].size);
		return -E_NOMEM;
	}

	/*get subimg in buffer*/
	ipe_dma_io[IPE_DMA_IN_SUBIMG].size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
	ipe_dma_io[IPE_DMA_IN_SUBIMG].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_IN_SUBIMG].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_IN_SUBIMG].vaddr == NULL) {
		DBG_ERR("get input subimg buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_IN_SUBIMG].size);
		return -E_NOMEM;
	}

	/*get Y out buffer*/
	ipe_dma_io[IPE_DMA_OUT_Y].size = in_w * in_h;
	ipe_dma_io[IPE_DMA_OUT_Y].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_OUT_Y].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_OUT_Y].vaddr == NULL) {
		DBG_ERR("get output y buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_OUT_Y].size);
		return -E_NOMEM;
	}

	/*get UV out buffer*/
	ipe_dma_io[IPE_DMA_OUT_UV].size  = in_w * in_h * 2;
	ipe_dma_io[IPE_DMA_OUT_UV].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_OUT_UV].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_OUT_UV].vaddr == NULL) {
		DBG_ERR("get output uv buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_OUT_UV].size);
		return -E_NOMEM;
	}

	/*get subimg out buffer*/
	ipe_dma_io[IPE_DMA_OUT_SUBIMG].size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
	ipe_dma_io[IPE_DMA_OUT_SUBIMG].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_OUT_SUBIMG].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_OUT_SUBIMG].vaddr == NULL) {
		DBG_ERR("get output subout buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_OUT_SUBIMG].size);
		return -E_NOMEM;
	}

	/*get va out buffer*/
	ipe_dma_io[IPE_DMA_OUT_VA].size = (va_param.va_lofs * va_param.win_num.h);
	ipe_dma_io[IPE_DMA_OUT_VA].vaddr = (UINT32 *)malloc(ipe_dma_io[IPE_DMA_OUT_VA].size * sizeof(UINT8));
	if (ipe_dma_io[IPE_DMA_OUT_VA].vaddr == NULL) {
		DBG_ERR("get output va buffer fail, size = %u\n", (unsigned int)ipe_dma_io[IPE_DMA_OUT_VA].size);
		return -E_NOMEM;
	}
#else
	int ret = 0;
	/*get Y in buffer*/
	ipe_dma_io[IPE_DMA_IN_Y].size = in_w * in_h;
	ret = nvt_fmem_mem_info_init(&y_in_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_IN_Y].size, NULL);
	if (ret >= 0) {
		handle_yin_buff = fmem_alloc_from_cma(&y_in_buf_info, 0);
	} else {
		DBG_ERR("get input buffer fail\n");
		return -E_NOMEM;
	}

	/*get UV in buffer*/
	ipe_dma_io[IPE_DMA_IN_UV].size = in_w * in_h * 2;
	ret = nvt_fmem_mem_info_init(&uv_in_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_IN_UV].size, NULL);
	if (ret >= 0) {
		handle_uvin_buff = fmem_alloc_from_cma(&uv_in_buf_info, 0);
	} else {
		DBG_ERR("get input buffer fail\n");
		return -E_NOMEM;
	}

	/*get subimg in buffer*/
	ipe_dma_io[IPE_DMA_IN_SUBIMG].size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
	ret = nvt_fmem_mem_info_init(&subin_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_IN_SUBIMG].size, NULL);
	if (ret >= 0) {
		handle_subin_buff = fmem_alloc_from_cma(&subin_buf_info, 0);
	} else {
		DBG_ERR("get input buffer fail\n");
		return -E_NOMEM;
	}


	/*get Y out buffer*/
	ipe_dma_io[IPE_DMA_OUT_Y].size = in_w * in_h;
	ret = nvt_fmem_mem_info_init(&y_out_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_OUT_Y].size, NULL);
	if (ret >= 0) {
		handle_yout_buff = fmem_alloc_from_cma(&y_out_buf_info, 0);
	} else {
		DBG_ERR("get output buffer fail\n");
		return -E_NOMEM;
	}

	/*get UV out buffer*/
	ipe_dma_io[IPE_DMA_OUT_UV].size = in_w * in_h * 2;
	ret = nvt_fmem_mem_info_init(&uv_out_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_OUT_UV].size, NULL);
	if (ret >= 0) {
		handle_uvout_buff = fmem_alloc_from_cma(&uv_out_buf_info, 0);
	} else {
		DBG_ERR("get output buffer fail\n");
		return -E_NOMEM;
	}

	/*get subimg out buffer*/
	ipe_dma_io[IPE_DMA_OUT_SUBIMG].size = (subimg_info.subimg_size.h_size * subimg_info.subimg_size.v_size * 4);
	ret = nvt_fmem_mem_info_init(&subout_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_OUT_SUBIMG].size, NULL);
	if (ret >= 0) {
		handle_subout_buff = fmem_alloc_from_cma(&subout_buf_info, 0);
	} else {
		DBG_ERR("get output buffer fail\n");
		return -E_NOMEM;
	}

	/*get va out buffer*/
	ipe_dma_io[IPE_DMA_OUT_VA].size = (va_param.va_lofs * va_param.win_num.h);
	ret = nvt_fmem_mem_info_init(&va_buf_info, NVT_FMEM_ALLOC_CACHE, ipe_dma_io[IPE_DMA_OUT_VA].size, NULL);
	if (ret >= 0) {
		handle_va_buff = fmem_alloc_from_cma(&va_buf_info, 0);
	} else {
		DBG_ERR("get output buffer fail\n");
		return -E_NOMEM;
	}

	ipe_dma_io[IPE_DMA_IN_Y].vaddr          = (UINT32 *)y_in_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_IN_UV].vaddr         = (UINT32 *)uv_in_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_IN_SUBIMG].vaddr     = (UINT32 *)subin_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_OUT_Y].vaddr         = (UINT32 *)y_out_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_OUT_UV].vaddr        = (UINT32 *)uv_out_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_OUT_SUBIMG].vaddr    = (UINT32 *)subout_buf_info.vaddr;
	ipe_dma_io[IPE_DMA_OUT_VA].vaddr        = (UINT32 *)va_buf_info.vaddr;

	ipe_dma_io[IPE_DMA_IN_Y].paddr          = (UINT32 *)y_in_buf_info.paddr;
	ipe_dma_io[IPE_DMA_IN_UV].paddr         = (UINT32 *)uv_in_buf_info.paddr;
	ipe_dma_io[IPE_DMA_IN_SUBIMG].paddr     = (UINT32 *)subin_buf_info.paddr;
	ipe_dma_io[IPE_DMA_OUT_Y].paddr         = (UINT32 *)y_out_buf_info.paddr;
	ipe_dma_io[IPE_DMA_OUT_UV].paddr        = (UINT32 *)uv_out_buf_info.paddr;
	ipe_dma_io[IPE_DMA_OUT_SUBIMG].paddr    = (UINT32 *)subout_buf_info.paddr;
	ipe_dma_io[IPE_DMA_OUT_VA].paddr        = (UINT32 *)va_buf_info.paddr;

#endif

	return E_OK;
}

void ipe_buffer_release(void)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	int i = 0;

	for (i = 0; i < IPE_DMA_IO_MAX; i++) {
		DBG_IND("free %d\n", i);
		free(ipe_dma_io[i].vaddr);
		ipe_dma_io[i].vaddr = NULL;
	}
#else
	int ret = 0;
	/* free buffer */
	ret = fmem_release_from_cma(handle_yin_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
	ret = fmem_release_from_cma(handle_yout_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
	ret = fmem_release_from_cma(handle_uvin_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
	ret = fmem_release_from_cma(handle_uvout_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}

	ret = fmem_release_from_cma(handle_subin_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
	ret = fmem_release_from_cma(handle_subout_buff, 0);
	if (ret < 0) {
		pr_info("error release %d\n", __LINE__);
	}
#endif
}


int ipe_load_input_data(char **pargv)
{
	VOS_FILE fp;
	char file_path[256] = "";
	int len = 0;

	/* load image */
	sprintf(file_path, "/mnt/sd/%s.raw", pargv[0]);
	fp = vos_file_open(file_path, O_RDONLY, 0);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_read(fp, (void *)ipe_dma_io[IPE_DMA_IN_Y].vaddr, ipe_dma_io[IPE_DMA_IN_Y].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);
#if 1
	sprintf(file_path, "/mnt/sd/%s.raw", pargv[1]);
	fp = vos_file_open(file_path, O_RDONLY, 0);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_read(fp, (void *)ipe_dma_io[IPE_DMA_IN_UV].vaddr, ipe_dma_io[IPE_DMA_IN_UV].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	sprintf(file_path, "/mnt/sd/%s.raw", pargv[2]);
	fp = vos_file_open(file_path, O_RDONLY, 0);
	if (fp == -1) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_read(fp, (void *)ipe_dma_io[IPE_DMA_IN_SUBIMG].vaddr, ipe_dma_io[IPE_DMA_IN_SUBIMG].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);
#endif

	return len;
}

int ipe_dump_output_data(char **pargv)
{
	VOS_FILE fp;
	char file_path[256] = "";
	int len = 0;

	DBG_IND("ipe dump y\n");
	sprintf(file_path, "/mnt/sd/%s_outy.raw", pargv[0]);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)ipe_dma_io[IPE_DMA_OUT_Y].vaddr, ipe_dma_io[IPE_DMA_OUT_Y].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

#if 0
	sprintf(file_path, "/mnt/sd/%s_outyin.raw", pargv[0]);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)ipe_dma_io[IPE_DMA_IN_Y].vaddr, ipe_dma_io[IPE_DMA_IN_Y].size);
	DBG_USER("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);
#endif

#if 1
	sprintf(file_path, "/mnt/sd/%s_outuv.raw", pargv[1]);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)ipe_dma_io[IPE_DMA_OUT_UV].vaddr, ipe_dma_io[IPE_DMA_OUT_UV].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);

	sprintf(file_path, "/mnt/sd/%s_outsub.raw", pargv[2]);
	fp = vos_file_open(file_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
		DBG_ERR("failed in file open:%s\n", file_path);
		return -1;
	}
	len = vos_file_write(fp, (void *)ipe_dma_io[IPE_DMA_OUT_SUBIMG].vaddr, ipe_dma_io[IPE_DMA_OUT_SUBIMG].size);
	DBG_IND("read length %d\r\n", (unsigned int)len);
	vos_file_close(fp);
#endif
	DBG_IND("ipe d2d dump end\n");

	return len;

}
#endif

int kdrv_api_ipe_test(unsigned char argc, char **pargv)
{
#if (IPE_TOOL_KIT_EN == 1)

	ER rt;
	int len = 0;
	UINT32 in_w = 0, in_h = 0;
	UINT32 id = 0, chip = 0, engine = 0x5003, channel = 0;

	INT16 cc_coef[9] = {427, -134, -37, -98, 377, -22, 59, -268, 465};
	//INT16 cc_coef[9] = {358, 256, 0, -183, 256, -88, 0, 256, 452}; //full yuv to rgb

#if (VERF_IQ_ALL == 1)
	static KDRV_IPE_PARAM_IQ_ALL_PARAM iq_param = {0};
#endif

	UINT32 i = 0;

	for (i = 0; i < argc; i++) {
		DBG_IND("pargv[%u] = %s\r\n", (unsigned int)i, pargv[i]);
	}

	id = KDRV_DEV_ID(chip, engine, channel);
	/*open*/
	kdrv_ipe_open_obj.ipe_clock_sel = 240;
	DBG_IND("ipe d2d set open\n");
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OPENCFG, (void *)&kdrv_ipe_open_obj);

	DBG_IND("ipe d2d flow open\n");
	kdrv_ipe_open(chip, engine);

	//goto EOP;

	/*get parametr*/
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EEXT, (void *)&eext_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT, (void *)&overshoot_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EEXT_TONEMAP, (void *)&eext_tonemap_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_RGBLPF, (void *)&rgblpf_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_PFR, (void *)&pfr_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CC_EN, (void *)&cc_en_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CC, (void *)&cc_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CCM, (void *)&ccm_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CCTRL_EN, (void *)&cctrl_en_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_EE, (void *)&cadj_ee_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_YCCON, (void *)&cadj_yccon_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_COFS, (void *)&cadj_cofs_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_RAND, (void *)&cadj_rand_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_HUE, (void *)&cadj_hue_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_FIXTH, (void *)&cadj_fixth_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CADJ_MASK, (void *)&cadj_mask_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CST, (void *)&cst_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_CSTP, (void *)&cstp_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_GAMYRAND, (void *)&gamyrand_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_3DCC, (void *)&iq_3dcc_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_DEFOG, (void *)&dfg_info);
	kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_LCE, (void *)&lce_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_EDGEDBG, (void *)&edgdbg_info);
	//kdrv_ipe_get(id, KDRV_IPE_PARAM_IQ_VA_PARAM, (void *)&va_param);

	rgblpf_info.enable = 0;
	pfr_info.enable = 0;
	//yuv2rgb
	cc_en_info.enable = 0;
	//cc_info.cc_ofs_sel = KDRV_CC_OFS_Y_FULL;
	ccm_info.cc_range = KDRV_IPE_CCRANGE_3_8;
	ccm_info.cc_gamma_sel = KDRV_IPE_CC_PRE_GAM;
	for (i = 0; i < 9; i++) {
		ccm_info.coef[i] = cc_coef[i];
	}
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
	cst_info.enable = 1;
	cst_info.cst_off_sel = KDRV_IPE_CST_MINUS128;
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
	edgdbg_info.enable = 0;
	va_param.enable = 1;
	va_param.indep_va_enable = 1;
	va_param.indep_win[0].enable = 1;
	va_param.indep_win[1].enable = 1;
	va_param.indep_win[2].enable = 1;
	va_param.indep_win[3].enable = 1;
	va_param.indep_win[4].enable = 1;
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
		va_win_info.indep_roi_ratio[i].x = 100;
		va_win_info.indep_roi_ratio[i].y = 100;
		va_win_info.indep_roi_ratio[i].w = 200;
		va_win_info.indep_roi_ratio[i].h = 150;
	}
	//iq_3dcc_info.enable =  0;
	//iq_3dcc_info.cc3d_rnd.round_opt = KDRV_IPE_ROUNDING;
	//iq_3dcc_info.cc3d_rnd.rst_en = 0;

	//in_w = 1152;
	//in_h = 64;
	in_w = 1920;
	in_h = 1080;

	DBG_IND("buffer allocate\n");
	rt = ipe_buffer_allocate(in_w, in_h);
	if (rt != E_OK) {
		return rt;
	}

	for (i = 0; i < IPE_DMA_IO_MAX; i++) {
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
		DBG_IND("[%d]:va_addr = 0x%08x, size = 0x%08x\r\n", (unsigned int) i, (unsigned int)ipe_dma_io[i].vaddr, (unsigned int)ipe_dma_io[i].size);
#else
		DBG_IND("[%d]:va_addr = 0x%08x, phy_addr = 0x%08x, size = 0x%08x\r\n", (unsigned int) i, (unsigned int)ipe_dma_io[i].vaddr, (unsigned int)ipe_dma_io[i].paddr, (unsigned int)ipe_dma_io[i].size);
#endif
	}

	DBG_IND("load input data\n");
	len = ipe_load_input_data(pargv);
	if (len == -1) {
		goto EOP;
	}

	/******** ipe d2d flow *********/
	DBG_IND("ipe d2d flow start test\n");

	/*set input img parameters*/
	in_img_info.in_src = KDRV_IPE_IN_SRC_DRAM;
	in_img_info.type = KDRV_IPE_IN_FMT_Y_PACK_UV444;
	in_img_info.yuv_range_fmt = KDRV_IPE_YUV_FMT_FULL;
	in_img_info.ch.y_width = in_w;
	in_img_info.ch.y_height = in_h;
	in_img_info.ch.y_line_ofs = in_w;
	in_img_info.ch.uv_line_ofs = in_w * 2;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

	/*set input address*/
	in_addr_info.addr_y = (UINT32)ipe_dma_io[IPE_DMA_IN_Y].vaddr;
	in_addr_info.addr_uv = (UINT32)ipe_dma_io[IPE_DMA_IN_UV].vaddr;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_IN_ADDR, (void *)&in_addr_info);

	/*set output img parameters*/
	out_img_info.type = KDRV_IPE_IN_FMT_Y_PACK_UV444;
	out_img_info.dram_out = TRUE;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

	/*set output address*/
	out_addr_info.addr_y = (UINT32)ipe_dma_io[IPE_DMA_OUT_Y].vaddr;
	out_addr_info.addr_uv = (UINT32)ipe_dma_io[IPE_DMA_OUT_UV].vaddr;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_OUT_ADDR, (void *)&out_addr_info);

	/*set subimg input/output dma address*/
	subin_addr_info.addr = (UINT32)ipe_dma_io[IPE_DMA_IN_SUBIMG].vaddr;
	subout_addr_info.enable = 0;
	subout_addr_info.addr = (UINT32)ipe_dma_io[IPE_DMA_OUT_SUBIMG].vaddr;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBIN_ADDR, (void *)&subin_addr_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, (void *)&subout_addr_info);

	/*set va output address*/
	va_addr_info.addr_va = (UINT32)ipe_dma_io[IPE_DMA_OUT_VA].vaddr;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_VA_ADDR, (void *)&va_addr_info);

	/*set eth parametr*/
	eth_info.enable = 0;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ETH, (void *)&eth_info);

#if (VERF_IQ_ALL != 1) //normal case

	/*set edge eext parametr*/
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EEXT, (void *)&eext_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT, (void *)&overshoot_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_EEXT_TONEMAP, (void *)&eext_tonemap_info);

	/*set parametr*/
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_RGBLPF, (void *)&rgblpf_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_PFR, (void *)&pfr_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CC_EN, (void *)&cc_en_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CC, (void *)&cc_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CCM, (void *)&ccm_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CCTRL_EN, (void *)&cctrl_en_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_EE, (void *)&cadj_ee_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_YCCON, (void *)&cadj_yccon_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_COFS, (void *)&cadj_cofs_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_RAND, (void *)&cadj_rand_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_HUE, (void *)&cadj_hue_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_FIXTH, (void *)&cadj_fixth_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CADJ_MASK, (void *)&cadj_mask_info);
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_CST, (void *)&cst_info);
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
	gamma_info.enable = 1;
	gamma_info.option = KDRV_IPE_GAMMA_RGB_COMBINE;

	for (i = 0; i < 129 ; i++) {
		gamma_info.gamma_lut[0][i] = (UINT32)gamma_day[i];
		gamma_info.gamma_lut[1][i] = (UINT32)gamma_day[i];
		gamma_info.gamma_lut[2][i] = (UINT32)gamma_day[i];
	}
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_GAMMA, (void *)&gamma_info);
	DBG_IND("set KDRV_IPE_PARAM_IQ_GAMMA\r\n");


	/*set ycurve parametr*/
	ycurve_info.enable = 0;
	ycurve_info.ycurve_sel = KDRV_YCURVE_RGB_Y;
	for (i = 0; i < 129 ; i++) {
		ycurve_info.y_curve_lut[i] = (UINT32)y_curve[i];
	}
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_YCURVE, (void *)&ycurve_info);
	DBG_IND("set KDRV_IPE_PARAM_IQ_YCURVE\r\n");

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
	//iq_param.p_3dcc         = &iq_3dcc_info;
	iq_param.p_defog        = &dfg_info;
	iq_param.p_lce          = &lce_info;
	iq_param.p_subimg       = &subimg_info;
	iq_param.p_edgedbg      = &edgdbg_info;
	iq_param.p_va           = &va_param;

	kdrv_ipe_set(id, KDRV_IPE_PARAM_IQ_ALL, (void *)&iq_param);
	DBG_IND("set KDRV_IPE_PARAM_IQ_ALL\r\n");
#endif

	ipl_en_info.enable = 1;
	ipl_en_info.ipl_ctrl_func = KDRV_IPE_IQ_FUNC_ALL;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&ipl_en_info);

	kdrv_ipe_fmd_flag = 0;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)&KDRV_IPE_ISR_CB);

	/*set interrupt enable*/
	inte_en = KDRV_IPE_INTE_ALL;
	kdrv_ipe_set(id, KDRV_IPE_PARAM_IPL_INTER, (void *)&inte_en);
	DBG_IND("set KDRV_IPE_PARAM_IPL_INTER\r\n");
#if 1
	/*trig*/
	kdrv_ipe_trigger(id, NULL, NULL, NULL);
	DBG_IND("kdrv_ipe_trigger\r\n");

	/*wait end*/
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	while (!kdrv_ipe_fmd_flag) {
		vos_task_delay_ms(10);
	}
#else
	while (!kdrv_ipe_fmd_flag) {
		vos_util_delay_ms(10);
	}
#endif
	//goto EOP;

	/*pause*/
	DBG_IND("ipe pause\r\n");
	kdrv_ipe_pause(chip, engine);
	kdrv_ipe_fmd_flag = 0;
#endif
	//kdrv_ipe_dump_info(printk);

	/*close*/
	kdrv_ipe_close(chip, engine);
	DBG_IND("ipe d2d flow end\n");
	/******** ipe d2d flow *********/

	DBG_IND("dump output data\n");
	len = ipe_dump_output_data(pargv);
	if (len == -1) {
		goto EOP;
	}

EOP:

	ipe_buffer_release();

	return len;
#else
	return 0;
#endif
}


#if defined (__KERNEL__)

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ipe_hw_power_saving(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable;

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
		if (clk_get_phase(ipe_clk[0]) == 0) {
			clk_set_phase(ipe_clk[0], 1);
		}

		if (clk_get_phase(ipe_pclk[0]) == 0) {
			clk_set_phase(ipe_pclk[0], 1);
		}

	} else {
		if (clk_get_phase(ipe_clk[0]) == 1) {
			clk_set_phase(ipe_clk[0], 0);
		}

		if (clk_get_phase(ipe_pclk[0]) == 1) {
			clk_set_phase(ipe_pclk[0], 0);
		}
	}

	DBG_DUMP("ipe-mclk: 0xB0-bit2,  ipe-pclk: 0xC0-bit2\r\n");
	debug_dumpmem(0xf00200B0, 0x10);
	debug_dumpmem(0xf00200C0, 0x10);

	DBG_IND("end\r\n");

	return 0;
}

int nvt_ipe_fw_power_saving(PIPE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable;

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
		fw_ipe_power_saving_en = TRUE;
	} else {
		fw_ipe_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");

	return 0;
}
#endif

#elif defined (__FREERTOS)

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ipe_hw_power_saving(unsigned char argc, char **pargv)
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

int nvt_ipe_fw_power_saving(unsigned char argc, char **pargv)
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
		fw_ipe_power_saving_en = TRUE;
	} else {
		fw_ipe_power_saving_en = FALSE;
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


