
#if defined (__UITRON) || defined (__ECOS)

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "Type.h"
#include "kernel.h"

#elif defined (__FREERTOS )

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <kwrap/file.h>

#include <malloc.h>
#include "kwrap/type.h"
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
//#include <comm/nvtmem.h>


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

#include "ime_drv.h"

#endif

//#include "ime_api.h"
#include "ime_dbg.h"

//#include "kdrv_videoprocess/kdrv_ipp_utility.h"
//#include "kdrv_videoprocess/kdrv_ipp_config.h"
//#include "kdrv_videoprocess/kdrv_ime.h"
//#include "kdrv_videoprocess/kdrv_ipp.h"

#include "ime_platform.h"
#include "ime_kit.h"


//-------------------------------------------------------

#define IME_TOOL_KIT_EN    0




#if (IME_TOOL_KIT_EN == 1)

typedef struct _ime_img_info_ {
	UINT32 in_size_h;
	UINT32 in_size_v;
	UINT32 in_size_ofs_y;
	UINT32 in_size_ofs_uv;
	UINT32 in_addr_y;
	UINT32 in_addr_uv;

	UINT32 out_size_h;
	UINT32 out_size_v;
	UINT32 out_size_ofs_y;
	UINT32 out_size_ofs_uv;
	UINT32 out_addr_y;
	UINT32 out_addr_uv;
} ime_img_info;

static UINT32 kdrv_ime_fmd_flag = 0;
INT32 kdrv_ime_isr_cb(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{

	if (sts & KDRV_IME_INT_FRM_END) {
		kdrv_ime_fmd_flag = 1;
	}

	return 0;
}



void *hdl_in_buf_y = NULL, *hdl_in_buf_u = NULL, *hdl_in_buf_v = NULL;
void *hdl_out_buf_p1_y = NULL, *hdl_out_buf_p1_u = NULL, *hdl_out_buf_p1_v = NULL;
void *hdl_out_buf_p2_y = NULL, *hdl_out_buf_p2_u = NULL, *hdl_out_buf_p2_v = NULL;


UINT32 in_size_h = 640, in_size_v = 480;
UINT32 in_lofs_y = 640, in_lofs_uv = 320;
UINT32 in_addr_y = 0, in_addr_u = 0, in_addr_v = 0;
UINT32 in_buf_size_y = 0, in_buf_size_u = 0, in_buf_size_v = 0;

int out_size_p1_h = 640, out_size_p1_v = 480;
int out_lofs_p1_y = 640, out_lofs_p1_uv = 640;
UINT32 out_addr_p1_y, out_addr_p1_u, out_addr_p1_v;
UINT32 out_buf_p1_size_y = 0, out_buf_p1_size_u = 0, out_buf_p1_size_v = 0;

int out_size_p2_h = 428, out_size_p2_v = 240;
int out_lofs_p2_y = 428, out_lofs_p2_uv = 428;
UINT32 out_addr_p2_y, out_addr_p2_u, out_addr_p2_v;
UINT32 out_buf_p2_size_y = 0, out_buf_p2_size_u = 0, out_buf_p2_size_v = 0;


struct nvt_fmem_mem_info_t kdrv_ime_test_get_chl_buf = {0};
void *hdl_ime_test_chl_buf = NULL;



KDRV_IME_OPENCFG ime_open_obj;
KDRV_IME_IN_INFO ime_in_img_info;
KDRV_IME_DMA_ADDR_INFO in_addr;
KDRV_IME_OUT_PATH_IMG_INFO ime_out_img_info[2];
KDRV_IME_OUT_PATH_ADDR_INFO out_addr;

KDRV_IME_PARAM_IPL_FUNC_EN ime_ipl_func_en = {0};
KDRV_IME_YCC_CVT_PARAM ime_yuvcvt_param = {0};




#endif





#if defined (__LINUX) || defined (__KERNEL__)

int nvt_ime_api_dump(PIME_INFO pime_info, unsigned char argc, char **pargv)
{
	DBG_IND("Dump kdriver\r\n");
	//kdrv_ime_dump_info(printk);

	return 0;
}

char in_raw_file_y[260] = "//mnt//sd//ime//input//img_in_y.raw";
char in_raw_file_u[260] = "//mnt//sd//ime//input//img_in_u.raw";
char in_raw_file_v[260] = "//mnt//sd//ime//input//img_in_v.raw";


char out_p1_raw_file_y[260] = "//mnt//sd//ime//output//img_out_p1_y.raw";
char out_p1_raw_file_u[260] = "//mnt//sd//ime//output//img_out_p1_u.raw";
char out_p1_raw_file_v[260] = "//mnt//sd//ime//output//img_out_p1_v.raw";

char out_p2_raw_file_y[260] = "//mnt//sd//ime//output//img_out_p2_y.raw";
char out_p2_raw_file_u[260] = "//mnt//sd//ime//output//img_out_p2_u.raw";
char out_p2_raw_file_v[260] = "//mnt//sd//ime//output//img_out_p2_v.raw";

char out_refout_raw_file_y[260] = "//mnt//sd//ime//output//img_out_refout_y.raw";
char out_refout_raw_file_u[260] = "//mnt//sd//ime//output//img_out_refout_u.raw";

char out_lcaout_raw_file_y[260] = "//mnt//sd//ime//output//img_out_lcaout_y.raw";


struct nvt_fmem_mem_info_t in_buf_y, in_buf_u, in_buf_v;
struct nvt_fmem_mem_info_t out_buf_p1_y, out_buf_p1_u, out_buf_p1_v;
struct nvt_fmem_mem_info_t out_buf_p2_y, out_buf_p2_u, out_buf_p2_v;


int nvt_kdrv_ime_test(PIME_INFO pime_info, unsigned char argc, char **pargv)
{
#if (IME_TOOL_KIT_EN == 1)
	INT32 handle;//, out_buf_p1_alloc_cnt;
	UINT32 id = 0;

	//mm_segment_t old_fs;
	//struct file *fp;
	VOS_FILE fp;
	int len;
	int ret = 0;
	UINT32 kdrv_ime_chl_buf_nums = 0, kdrv_ime_buf_size = 0, kdrv_get_addr = 0;

	kdrv_ime_chl_buf_nums = 4;
	kdrv_ime_buf_size = kdrv_ime_buf_query(kdrv_ime_chl_buf_nums);
	ret = nvt_fmem_mem_info_init(&kdrv_ime_test_get_chl_buf, NVT_FMEM_ALLOC_CACHE, kdrv_ime_buf_size, NULL);
	if (ret >= 0) {
		hdl_ime_test_chl_buf = nvtmem_alloc_buffer(&kdrv_ime_test_get_chl_buf);
		kdrv_get_addr = (UINT32)kdrv_ime_test_get_chl_buf.vaddr;
	}
	kdrv_ime_buf_init(kdrv_get_addr, kdrv_ime_chl_buf_nums);


	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0, 0);

	DBG_ERR("ime d2d flow alloc memory and load test file\n");

	// input Y buffer
	in_buf_size_y = (in_lofs_y * in_size_v);
	ret = nvt_fmem_mem_info_init(&in_buf_y, NVT_FMEM_ALLOC_CACHE, in_buf_size_y, NULL);
	if (ret >= 0) {
		hdl_in_buf_y = nvtmem_alloc_buffer(&in_buf_y);
		in_addr_y = (UINT32)in_buf_y.vaddr;
	}
#if 0
	in_buf_y.size = in_buf_size_y;
	in_buf_y.align = 64;      ///< address alignment
	in_buf_y.name = "in_buf_y";
	in_buf_y.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &in_buf_y);
	in_addr_y = (UINT32)in_buf_y.va_addr;
#endif
	DBG_ERR("in_buf_y.vaddr = 0x%08x\r\n", (UINT32)in_buf_y.vaddr);

	// input U buffer
	in_buf_size_u = (in_lofs_uv * in_size_v >> 1);
	ret = nvt_fmem_mem_info_init(&in_buf_u, NVT_FMEM_ALLOC_CACHE, in_buf_size_u, NULL);
	if (ret >= 0) {
		hdl_in_buf_u = nvtmem_alloc_buffer(&in_buf_u);
		in_addr_u = (UINT32)in_buf_u.vaddr;
	}
#if 0
	in_buf_u.size = in_buf_size_u;
	in_buf_u.align = 64;      ///< address alignment
	in_buf_u.name = "in_buf_u";
	in_buf_u.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &in_buf_u);
	in_addr_u = (UINT32)in_buf_u.va_addr;
#endif
	DBG_ERR("in_buf_u.vaddr = 0x%08x\r\n", (UINT32)in_buf_u.vaddr);

	// input V buffer
	in_buf_size_v = (in_lofs_uv * in_size_v >> 1);
	ret = nvt_fmem_mem_info_init(&in_buf_v, NVT_FMEM_ALLOC_CACHE, in_buf_size_v, NULL);
	if (ret >= 0) {
		hdl_in_buf_v = nvtmem_alloc_buffer(&in_buf_v);
		in_addr_v = (UINT32)in_buf_v.vaddr;
	}
#if 0
	in_buf_v.size = in_buf_size_v;
	in_buf_v.align = 64;      ///< address alignment
	in_buf_v.name = "in_buf_v";
	in_buf_v.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &in_buf_v);
	in_addr_v = (UINT32)in_buf_v.va_addr;
#endif

	DBG_ERR("in_buf_v.vaddr = 0x%08x\r\n", (UINT32)in_buf_v.vaddr);



	// load y file
	fp = vos_file_open(in_raw_file_y, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_y);
		return -EFAULT;
	}
	len = vos_file_read(fp, (void *)in_addr_y, in_buf_size_y);
	DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
	vos_file_close(fp);

	// load u file
	fp = vos_file_open(in_raw_file_u, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_u);
		return -EFAULT;
	}
	len = vos_file_read(fp, (void *)in_addr_u, in_buf_size_u);
	DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
	vos_file_close(fp);


	// load v file
	fp = vos_file_open(in_raw_file_v, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_v);
		return -EFAULT;
	}
	len = vos_file_read(fp, (void *)in_addr_v, in_buf_size_v);
	DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
	vos_file_close(fp);


	/******** ime d2d flow *********/
	DBG_ERR("ime d2d flow start\n");
	/* open config */
	ime_open_obj.ime_clock_sel = 240;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OPENCFG, (void *)&ime_open_obj);

	/*open*/
	//ime_open_obj.ime_clock_sel = 480;
	handle = kdrv_ime_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
	if (handle == -1) {
		DBG_ERR("ime open fail...\r\n");
		return -1;
	}

	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_RESET, NULL);

	/* set isr */
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ISR_CB, (void *)&kdrv_ime_isr_cb);

	//set input img parameters
	ime_in_img_info.in_src = KDRV_IME_IN_SRC_DRAM;
	ime_in_img_info.type = KDRV_IME_IN_FMT_YUV420;
	ime_in_img_info.ch[0].width = in_size_h;
	ime_in_img_info.ch[0].height = in_size_v;
	ime_in_img_info.ch[0].line_ofs = in_lofs_y;
	ime_in_img_info.ch[1].line_ofs = in_lofs_uv;
	ime_in_img_info.dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&ime_in_img_info);


	//set input address
	in_addr.addr_y  = (UINT32)in_buf_y.vaddr;
	in_addr.addr_cb = (UINT32)in_buf_u.vaddr;
	in_addr.addr_cr = (UINT32)in_buf_v.vaddr;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_ADDR, (void *)&in_addr);

	//set output path parameters
	//for (i = 0; i < IME_EXAM_OUT_MAX; i ++) {
	{
		ime_out_img_info[0].path_id = KDRV_IME_PATH_ID_1;
		ime_out_img_info[0].path_en = ENABLE;
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

		ime_out_img_info[0].dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;

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

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = (out_lofs_p1_uv * out_size_p1_v);

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = (out_lofs_p1_uv * (out_size_p1_v >> 1));

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p1_size_y = (out_lofs_p1_y * (out_size_p1_v >> 1));
			out_buf_p1_size_u = 0;
			out_buf_p1_size_v = 0;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = out_addr_p1_y;
			//out_addr_p1_v = out_addr_p1_y;
			break;

		default:
			break;
		}

		out_addr.path_id = KDRV_IME_PATH_ID_1;

		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p1_size_y != 0) {
				//
#if 0
				out_buf_p1_y.size = out_buf_p1_size_y;
				out_buf_p1_y.align = 64;     ///< address alignment
				out_buf_p1_y.name = "out_buf_p1_y";
				out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_y, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_y = nvtmem_alloc_buffer(&out_buf_p1_y);
					out_addr_p1_y = (UINT32)out_buf_p1_y.vaddr;
				}
				DBG_ERR("out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y.vaddr);
			}

			if (out_buf_p1_size_u != 0) {
				//
#if 0
				out_buf_p1_u.size = out_buf_p1_size_u;
				out_buf_p1_u.align = 64;     ///< address alignment
				out_buf_p1_u.name = "out_buf_p1_u";
				out_buf_p1_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_u);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_u, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_u, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_u = nvtmem_alloc_buffer(&out_buf_p1_u);
					out_addr_p1_u = (UINT32)out_buf_p1_u.vaddr;
				}
				DBG_ERR("out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_u.vaddr);
			}

			if (out_buf_p1_size_v != 0) {
				//
#if 0
				out_buf_p1_v.size = out_buf_p1_size_v;
				out_buf_p1_v.align = 64;     ///< address alignment
				out_buf_p1_v.name = "out_buf_p1_v";
				out_buf_p1_v.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_v);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_v, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_v, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_v = nvtmem_alloc_buffer(&out_buf_p1_v);
					out_addr_p1_v = (UINT32)out_buf_p1_v.vaddr;
				}
				DBG_ERR("out_buf_p1_v.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_v.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p1_size_y != 0) {
				//
#if 0
				out_buf_p1_y.size = out_buf_p1_size_y;
				out_buf_p1_y.align = 64;     ///< address alignment
				out_buf_p1_y.name = "out_buf_p1_y";
				out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_y, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_y = nvtmem_alloc_buffer(&out_buf_p1_y);
					out_addr_p1_y = (UINT32)out_buf_p1_y.vaddr;
				}
				DBG_ERR("out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y.vaddr);
			}

			if (out_buf_p1_size_u != 0) {
				//
#if 0
				out_buf_p1_u.size = out_buf_p1_size_u;
				out_buf_p1_u.align = 64;     ///< address alignment
				out_buf_p1_u.name = "out_buf_p1_u";
				out_buf_p1_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_u);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_u, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_u, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_u = nvtmem_alloc_buffer(&out_buf_p1_u);
					out_addr_p1_u = (UINT32)out_buf_p1_u.vaddr;
				}
				DBG_ERR("out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_u.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p1_size_y != 0) {
				//
#if 0
				out_buf_p1_y.size = out_buf_p1_size_y;
				out_buf_p1_y.align = 64;     ///< address alignment
				out_buf_p1_y.name = "out_buf_p1_y";
				out_buf_p1_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p1_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p1_y, NVT_FMEM_ALLOC_CACHE, out_buf_p1_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p1_y = nvtmem_alloc_buffer(&out_buf_p1_y);
					out_addr_p1_y = (UINT32)out_buf_p1_y.vaddr;
				}
				DBG_ERR("out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&out_addr);
	}

	{
		ime_out_img_info[1].path_id = KDRV_IME_PATH_ID_2;
		ime_out_img_info[1].path_en = ENABLE;
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

		ime_out_img_info[1].dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;

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

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = (out_lofs_p2_uv * out_size_p2_v);

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = (out_lofs_p2_uv * (out_size_p2_v >> 1));

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = (out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p2_size_y = (out_lofs_p2_y * out_size_p2_v);

			out_buf_p2_size_u = (out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p2_size_y = (out_lofs_p2_y * (out_size_p2_v >> 1));
			out_buf_p2_size_u = 0;
			out_buf_p2_size_v = 0;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = out_addr_p2_y;
			//out_addr_p2_v = out_addr_p2_y;
			break;

		default:
			break;
		}

		out_addr.path_id = KDRV_IME_PATH_ID_2;

		switch (ime_out_img_info[1].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p2_size_y != 0) {
				//
#if 0
				out_buf_p2_y.size = out_buf_p2_size_y;
				out_buf_p2_y.align = 64;     ///< address alignment
				out_buf_p2_y.name = "out_buf_p2_y";
				out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_y, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_y = nvtmem_alloc_buffer(&out_buf_p2_y);
					out_addr_p2_y = (UINT32)out_buf_p2_y.vaddr;
				}
				DBG_ERR("out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y.vaddr);
			}

			if (out_buf_p2_size_u != 0) {
				//
#if 0
				out_buf_p2_u.size = out_buf_p2_size_u;
				out_buf_p2_u.align = 64;     ///< address alignment
				out_buf_p2_u.name = "out_buf_p2_u";
				out_buf_p2_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_u);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_u, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_u, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_u = nvtmem_alloc_buffer(&out_buf_p2_u);
					out_addr_p2_u = (UINT32)out_buf_p2_u.vaddr;
				}
				DBG_ERR("out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_u.vaddr);
			}

			if (out_buf_p2_size_v != 0) {
				//
#if 0
				out_buf_p2_v.size = out_buf_p2_size_v;
				out_buf_p2_v.align = 64;     ///< address alignment
				out_buf_p2_v.name = "out_buf_p2_v";
				out_buf_p2_v.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_v);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_v, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_v, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_v = nvtmem_alloc_buffer(&out_buf_p2_v);
					out_addr_p2_v = (UINT32)out_buf_p2_v.vaddr;
				}
				DBG_ERR("out_buf_p2_v.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_v.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p2_size_y != 0) {
				//
#if 0
				out_buf_p2_y.size = out_buf_p2_size_y;
				out_buf_p2_y.align = 64;     ///< address alignment
				out_buf_p2_y.name = "out_buf_p2_y";
				out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_y, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_y = nvtmem_alloc_buffer(&out_buf_p2_y);
					out_addr_p2_y = (UINT32)out_buf_p2_y.vaddr;
				}
				DBG_ERR("out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y.vaddr);
			}

			if (out_buf_p2_size_u != 0) {
				//
#if 0
				out_buf_p2_u.size = out_buf_p2_size_u;
				out_buf_p2_u.align = 64;     ///< address alignment
				out_buf_p2_u.name = "out_buf_p2_u";
				out_buf_p2_u.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_u);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_u, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_u, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_u = nvtmem_alloc_buffer(&out_buf_p2_u);
					out_addr_p2_u = (UINT32)out_buf_p2_u.vaddr;
				}
				DBG_ERR("out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_u.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p2_size_y != 0) {
				//
#if 0
				out_buf_p2_y.size = out_buf_p2_size_y;
				out_buf_p2_y.align = 64;     ///< address alignment
				out_buf_p2_y.name = "out_buf_p2_y";
				out_buf_p2_y.alloc_type = ALLOC_CACHEABLE;
				frm_get_buf_ddr(DDR_ID0, &out_buf_p2_y);
#endif
				ret = nvt_fmem_mem_info_init(&out_buf_p2_y, NVT_FMEM_ALLOC_CACHE, out_buf_p2_size_y, NULL);
				if (ret >= 0) {
					hdl_out_buf_p2_y = nvtmem_alloc_buffer(&out_buf_p2_y);
					out_addr_p2_y = (UINT32)out_buf_p2_y.vaddr;
				}
				DBG_ERR("out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y.vaddr);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&out_addr);
	}

#if 0
	DBG_ERR("ime iq_yuv-cvt...\r\n");
	ime_yuvcvt_param.enable = TRUE;
	ime_yuvcvt_param.cvt_sel = KDRV_IME_YCC_CVT_BT601;
	kdrv_ime_set(id, KDRV_IME_PARAM_IQ_YCCCVT, (void *)&ime_yuvcvt_param);


	ime_ipl_func_en.ipl_ctrl_func = KDRV_IME_IQ_FUNC_YUVCVT;
	ime_ipl_func_en.enable = TRUE;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, (void *)&ime_ipl_func_en);
	DBG_ERR("ime ime_ipl_func_en...\r\n");
#endif

	/* trigger */
	DBG_ERR("ime d2d flow trigger\n");
	kdrv_ime_fmd_flag = 0;
	//kdrv_ime_set(handle, KDRV_IME_PARAM_IPL_TRIG, NULL);
	kdrv_ime_trigger(id, NULL, NULL, NULL);

	/*wait end*/
	DBG_ERR("ime d2d flow wait end\n");
	while (!kdrv_ime_fmd_flag) {
		vos_util_delay_ms(10);
	}
	DBG_ERR("ime d2d flow end\n");

	kdrv_ime_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);

#if 0
	/* save image */
	for (i = 0; i < IME_EXAM_OUT_MAX; i ++) {

		//get output path parameters
		out_addr.path_id = DAL_IME_PATH_ID_1 + i;
		dal_ime_get(handle, DAL_IME_ITEM_OUT_ADDR, (void *)&out_addr);

		DBG_ERR("path(%d) save ime output image w:%d, h:%d, y_addr:0x%.8x size:0x%.8x\r\n", i, Out_Data[i].LineOfs, Out_Data[i].SizeV, (UINT32)out_buf_p1_info[i].va_addr, out_buf_p1_info[i].size);
		fp = vos_file_open(Out_Data[i].FileName, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", Out_Data[i].FileName);
			goto IME_CLOSE;
		}
		len = vos_file_write(fp, out_buf_p1_info[i].va_addr, out_buf_p1_info[i].size, &fp->f_pos);
		vos_file_close(fp);
	}
#endif

	if ((ime_out_img_info[0].path_id == KDRV_IME_PATH_ID_1) && (ime_out_img_info[0].path_en == ENABLE)) {
		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			vos_file_close(fp);

			// save u to file
			fp = vos_file_open(out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_u, out_buf_p1_size_u);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(out_p1_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_v, out_buf_p1_size_v);
			vos_file_close(fp);

			//frm_free_buf_ddr(out_buf_p1_y.va_addr);
			//frm_free_buf_ddr(out_buf_p1_u.va_addr);
			//frm_free_buf_ddr(out_buf_p1_v.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_y);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_u);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_v);

			hdl_out_buf_p1_y = NULL;
			hdl_out_buf_p1_u = NULL;
			hdl_out_buf_p1_v = NULL;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_u, out_buf_p1_size_u);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);

			//frm_free_buf_ddr(out_buf_p1_y.va_addr);
			//frm_free_buf_ddr(out_buf_p1_u.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_y);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_u);

			hdl_out_buf_p1_y = NULL;
			hdl_out_buf_p1_u = NULL;
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p1_y, out_buf_p1_size_y);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);

			//frm_free_buf_ddr(out_buf_p1_y.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p1_y);

			hdl_out_buf_p1_y = NULL;
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
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			vos_file_close(fp);

			// save u to file
			fp = vos_file_open(out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_u, out_buf_p2_size_u);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(out_p2_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_v, out_buf_p2_size_v);
			vos_file_close(fp);

			//frm_free_buf_ddr(out_buf_p2_y.va_addr);
			//frm_free_buf_ddr(out_buf_p2_u.va_addr);
			//frm_free_buf_ddr(out_buf_p2_v.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_y);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_u);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_v);

			hdl_out_buf_p2_y = NULL;
			hdl_out_buf_p2_u = NULL;
			hdl_out_buf_p2_v = NULL;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);

			// save uv to file
			fp = vos_file_open(out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_u, out_buf_p2_size_u);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);


			//frm_free_buf_ddr(out_buf_p2_y.va_addr);
			//frm_free_buf_ddr(out_buf_p2_u.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_y);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_u);

			hdl_out_buf_p2_y = NULL;
			hdl_out_buf_p2_u = NULL;
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_addr_p2_y, out_buf_p2_size_y);
			DBG_ERR("ime: read file lenght: %d\r\n", (int)len);
			vos_file_close(fp);

			//frm_free_buf_ddr(out_buf_p2_y.va_addr);
			ret = nvtmem_release_buffer(hdl_out_buf_p2_y);

			hdl_out_buf_p2_y = NULL;
			break;

		default:
			break;
		}
	}

//IME_CLOSE:
	/* close */
	kdrv_ime_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);

//IME_END:
	DBG_ERR("ime d2d flow free memory\n");
	/* free buffer */
	//frm_free_buf_ddr(in_buf_y.va_addr);
	//frm_free_buf_ddr(in_buf_u.va_addr);
	//frm_free_buf_ddr(in_buf_v.va_addr);

	ret = nvtmem_release_buffer(hdl_in_buf_y);
	ret = nvtmem_release_buffer(hdl_in_buf_u);
	ret = nvtmem_release_buffer(hdl_in_buf_v);

	hdl_in_buf_y = NULL;
	hdl_in_buf_u = NULL;
	hdl_in_buf_v = NULL;

	ret = nvtmem_release_buffer(hdl_ime_test_chl_buf);
	hdl_ime_test_chl_buf = NULL;

	kdrv_ime_buf_uninit();

	//for (i = 0; i < out_buf_p1_alloc_cnt; i ++) {
	//  frm_free_buf_ddr(out_buf_p1_info[i].va_addr);
	//}
	return 0;
#else
	//printk("ime d2d flow alloc memory and load test file\n");

	return 0;
#endif

}

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ime_hw_power_saving(PIME_INFO pmodule_info, unsigned char argc, char **pargv)
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
		if (clk_get_phase(ime_clk[0]) == 0) {
			clk_set_phase(ime_clk[0], 1);
		}

		if (clk_get_phase(ime_pclk[0]) == 0) {
			clk_set_phase(ime_pclk[0], 1);
		}

	} else {
		if (clk_get_phase(ime_clk[0]) == 1) {
			clk_set_phase(ime_clk[0], 0);
		}

		if (clk_get_phase(ime_pclk[0]) == 1) {
			clk_set_phase(ime_pclk[0], 0);
		}

	}

	DBG_DUMP("ime-mclk: 0xB0-bit3,  ime-pclk: 0xC0-bit3\r\n");
	debug_dumpmem(0xf00200B0, 0x10);
	debug_dumpmem(0xf00200C0, 0x10);

	DBG_IND("end\r\n");

	return 0;
}

int nvt_ime_fw_power_saving(PIME_INFO pmodule_info, unsigned char argc, char **pargv)
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
		fw_ime_power_saving_en = TRUE;
	} else {
		fw_ime_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");

	return 0;
}
#endif


#if 0
int kdrv_ipp_fastboot_mem(PIME_INFO pime_info, unsigned char argc, char **pargv)
{
	int     fd, len;
	char    tmp_buf[128] = "/mnt/sd/nvt-na51055-fastboot-ipp.dtsi";
	char    pre_fix[4] = "\t\t";

	UINT32 eng_idx = 0, eng_reg_idx = 0;
	UINT32 reg_val = 0;
	void __iomem *vaddr_base;

	CHAR *dtsi_reg_num_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_num",
		"dce_reg_num",
		"ipe_reg_num",
		"ime_reg_num",
		"ife2_reg_num",
	};

#if 1
	CHAR *dtsi_reg_cfg_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_cfg",
		"dce_reg_cfg",
		"ipe_reg_cfg",
		"ime_reg_cfg",
		"ife2_reg_cfg",
	};

	UINT32 eng_reg_base[KDRV_IPP_BUILTIN_ENG_MAX] = {
		0xF0C70000, /* IFE */
		0xF0C20000, /* DCE */
		0xF0C30000, /* IPE */
		0xF0C40000, /* IME */
		0xF0D00000, /* IFE2 */
	};
#endif

	UINT32 eng_reg_nums[KDRV_IPP_BUILTIN_ENG_MAX] = {
		470 - 1,    /* IFE */
		394 - 1,    /* DCE */
		576 - 1,    /* IPE */
		656 - 1,    /* IME */
		44 - 1,    /* IFE2 */
	};


	fd = vos_file_open(tmp_buf, O_CREAT | O_WRONLY | O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		printk("open %s failure\r\n", tmp_buf);
		return FALSE;
	}

	// begin
	len = snprintf(tmp_buf, sizeof(tmp_buf), "%sregister {\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	for (eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s  %s = <%d>;\r\n", pre_fix, dtsi_reg_num_tag[eng_idx], eng_reg_nums[eng_idx]);
		vos_file_write(fd, (void *)tmp_buf, len);
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	//ioread32((void*)(ofs))

	for (eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
		//if (eng_idx == KDRV_IPP_BUILTIN_IME) {
		{
			len = snprintf(tmp_buf, sizeof(tmp_buf), "%s  %s = \r\n", pre_fix, dtsi_reg_cfg_tag[eng_idx]);
			vos_file_write(fd, (void *)tmp_buf, len);

			vaddr_base = ioremap_nocache(eng_reg_base[eng_idx], ((eng_reg_nums[eng_idx] + 1) << 2));

			for (eng_reg_idx = 0; eng_reg_idx < eng_reg_nums[eng_idx]; eng_reg_idx++) {

				reg_val = *(volatile UINT32 *)(vaddr_base + ((eng_reg_idx + 1) << 2));

				if (eng_reg_idx == eng_reg_nums[eng_idx] - 1) {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "%s    <0x%04x 0x%08x>;\r\n", pre_fix, ((eng_reg_idx + 1) << 2), reg_val);
				} else {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "%s    <0x%04x 0x%08x>,\r\n", pre_fix, ((eng_reg_idx + 1) << 2), reg_val);
				}

				vos_file_write(fd, (void *)tmp_buf, len);
			}
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
			vos_file_write(fd, (void *)tmp_buf, len);
		}
	}

	// end
	len = snprintf(tmp_buf, sizeof(tmp_buf), "%s};\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	vos_file_close(fd);
	return TRUE;
}
#endif

#elif defined (__UITRON) || defined (__ECOS) || defined (__FREERTOS)

int nvt_ime_api_dump(unsigned char argc, char **pargv)
{
#if (IME_TOOL_KIT_EN == 1)

	DBG_IND("Dump kdriver\r\n");
	//kdrv_ime_dump_info(printf);
#endif

	return 0;
}



UINT32 *in_buf_y = NULL, *in_buf_u = NULL, *in_buf_v = NULL;
UINT32 *out_buf_p1_y = NULL, *out_buf_p1_u = NULL, *out_buf_p1_v = NULL;
UINT32 *out_buf_p2_y = NULL, *out_buf_p2_u = NULL, *out_buf_p2_v = NULL;

char in_raw_file_y[260] = "/mnt/sd/ime/input/img_in_y.raw";
char in_raw_file_u[260] = "/mnt/sd/ime/input/img_in_u.raw";
char in_raw_file_v[260] = "/mnt/sd/ime/input/img_in_v.raw";


char out_p1_raw_file_y[260] = "/mnt/sd/ime/output/img_out_p1_y.raw";
char out_p1_raw_file_u[260] = "/mnt/sd/ime/output/img_out_p1_u.raw";
char out_p1_raw_file_v[260] = "/mnt/sd/ime/output/img_out_p1_v.raw";

char out_p2_raw_file_y[260] = "/mnt/sd/ime/output/img_out_p2_y.raw";
char out_p2_raw_file_u[260] = "/mnt/sd/ime/output/img_out_p2_u.raw";
char out_p2_raw_file_v[260] = "/mnt/sd/ime/output/img_out_p2_v.raw";

char out_refout_raw_file_y[260] = "/mnt/sd/ime/output/img_out_refout_y.raw";
char out_refout_raw_file_u[260] = "/mnt/sd/ime/output/img_out_refout_u.raw";

char out_lcaout_raw_file_y[260] = "/mnt/sd/ime/output/img_out_lcaout_y.raw";



int nvt_kdrv_ime_test(unsigned char argc, char **pargv)
{
#if (IME_TOOL_KIT_EN == 1)

	//mm_segment_t old_fs;
	VOS_FILE fp;

	int len;

	//int ret = 0;

	INT32 handle;//, out_buf_p1_alloc_cnt;
	UINT32 id = 0;

	kdrv_ime_init();
	ime_platform_prepare_clk();

	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0, 0);


	DBG_DUMP("ime d2d flow alloc memory and load test file\n");

	// input Y buffer
	in_buf_size_y = (in_lofs_y * in_size_v);
	if (in_buf_y != NULL) {
		vPortFree((void *)in_buf_y);
		in_buf_y = NULL;
	}
	in_buf_y = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(in_buf_size_y));

	if (in_buf_y != NULL) {
		in_addr_y = (UINT32)in_buf_y;
	}
	nvt_dbg(IND, "in_buf_y.vaddr = 0x%08x\r\n", (unsigned int)in_addr_y);

	// input U buffer
	in_buf_size_u = (in_lofs_uv * in_size_v >> 1);
	if (in_buf_u != NULL) {
		vPortFree((void *)in_buf_u);
		in_buf_u = NULL;
	}
	in_buf_u = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(in_buf_size_u));

	if (in_buf_u != NULL) {
		in_addr_u = (UINT32)in_buf_u;
	}
	nvt_dbg(IND, "in_buf_u.vaddr = 0x%08x\r\n", (unsigned int)in_addr_u);

	// input V buffer
	in_buf_size_v = (in_lofs_uv * in_size_v >> 1);
	if (in_buf_v != NULL) {
		vPortFree((void *)in_buf_v);
		in_buf_v = NULL;
	}
	in_buf_v = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(in_buf_size_v));
	if (in_buf_v != NULL) {
		in_addr_v = (UINT32)in_buf_v;
	}
	nvt_dbg(IND, "in_buf_v.vaddr = 0x%08x\r\n", (unsigned int)in_addr_v);



	// load y file
	fp = vos_file_open(in_raw_file_y, O_RDONLY, 0);
	if (fp == (VOS_FILE) - 1) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_y);
		return -1;
	}
	len = vos_file_read(fp, (void *)in_buf_y, (vos_size_t)in_buf_size_y);
	nvt_dbg(IND, "ime: read file lenght: %d\r\n", (int)len);
	len = vos_file_close(fp);



	// load u file
	fp = vos_file_open(in_raw_file_u, O_RDONLY, 0);
	if (fp == (VOS_FILE) - 1) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_u);
		return -1;
	}
	len = vos_file_read(fp, (void *)in_buf_u, (vos_size_t)in_buf_size_u);
	nvt_dbg(IND, "ime: read file lenght: %d\r\n", (int)len);
	len = vos_file_close(fp);




	// load v file
	fp = vos_file_open(in_raw_file_v, O_RDONLY, 0);
	if (fp == (VOS_FILE) - 1) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_raw_file_v);
		return -1;
	}
	len = vos_file_read(fp, (void *)in_buf_v, (vos_size_t)in_buf_size_v);
	nvt_dbg(IND, "ime: read file lenght: %d\r\n", (int)len);
	len = vos_file_close(fp);




	/******** ime d2d flow *********/
	DBG_DUMP("ime d2d flow start\n");
	/* open config */
	ime_open_obj.ime_clock_sel = 240;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OPENCFG, (void *)&ime_open_obj);


	/*open*/
	//ime_open_obj.ime_clock_sel = 480;
	handle = kdrv_ime_open(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);
	if (handle == -1) {
		DBG_DUMP("ime open fail...\r\n");
		return -1;
	}



	//set input img parameters
	ime_in_img_info.in_src = KDRV_IME_IN_SRC_DRAM;
	ime_in_img_info.type = KDRV_IME_IN_FMT_YUV420;
	ime_in_img_info.ch[0].width = in_size_h;
	ime_in_img_info.ch[0].height = in_size_v;
	ime_in_img_info.ch[0].line_ofs = in_lofs_y;
	ime_in_img_info.ch[1].line_ofs = in_lofs_uv;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&ime_in_img_info);


	//set input address
	in_addr.addr_y  = (UINT32)in_buf_y;
	in_addr.addr_cb = (UINT32)in_buf_u;
	in_addr.addr_cr = (UINT32)in_buf_v;
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_IN_ADDR, (void *)&in_addr);

	//set output path parameters
	//for (i = 0; i < IME_EXAM_OUT_MAX; i ++) {
	{
		ime_out_img_info[0].path_id = KDRV_IME_PATH_ID_1;
		ime_out_img_info[0].path_en = ENABLE;
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

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = (out_lofs_p1_uv * out_size_p1_v);

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_lofs_p1_uv = out_lofs_p1_uv >> 1;
			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = (out_lofs_p1_uv * (out_size_p1_v >> 1));

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = (UINT32)out_buf_p1_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);
			out_buf_p1_size_u = (out_lofs_p1_uv * out_size_p1_v);
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p1_size_y = (out_lofs_p1_y * out_size_p1_v);

			out_buf_p1_size_u = (out_lofs_p1_uv * (out_size_p1_v >> 1));
			out_buf_p1_size_v = out_buf_p1_size_u;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = (UINT32)out_buf_p1_u.va_addr;
			//out_addr_p1_v = out_addr_p1_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p1_size_y = (out_lofs_p1_y * (out_size_p1_v >> 1));
			out_buf_p1_size_u = 0;
			out_buf_p1_size_v = 0;

			//out_addr_p1_y = (UINT32)out_buf_p1_y.va_addr;
			//out_addr_p1_u = out_addr_p1_y;
			//out_addr_p1_v = out_addr_p1_y;
			break;

		default:
			break;
		}

		out_addr.path_id = KDRV_IME_PATH_ID_1;

		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p1_size_y != 0) {
				if (out_buf_p1_y != NULL) {
					vPortFree((void *)out_buf_p1_y);
					out_buf_p1_y = NULL;
				}
				out_buf_p1_y = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_y));
				if (out_buf_p1_y != NULL) {
					out_addr_p1_y = (UINT32)out_buf_p1_y;
				}
				nvt_dbg(IND, "out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_y);
			}

			if (out_buf_p1_size_u != 0) {
				//
				if (out_buf_p1_u != NULL) {
					vPortFree((void *)out_buf_p1_u);
					out_buf_p1_u = NULL;
				}
				out_buf_p1_u = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_u));
				if (out_buf_p1_u != NULL) {
					out_addr_p1_u = (UINT32)out_buf_p1_u;
				}
				nvt_dbg(IND, "out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_u);
			}

			if (out_buf_p1_size_v != 0) {
				if (out_buf_p1_v != NULL) {
					vPortFree((void *)out_buf_p1_v);
					out_buf_p1_v = NULL;
				}
				out_buf_p1_v = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_v));
				if (out_buf_p1_v != NULL) {
					out_addr_p1_v = (UINT32)out_buf_p1_v;
				}
				nvt_dbg(IND, "out_buf_p1_v.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_v);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p1_size_y != 0) {
				if (out_buf_p1_y != NULL) {
					vPortFree((void *)out_buf_p1_y);
					out_buf_p1_y = NULL;
				}
				out_buf_p1_y = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_y));

				if (out_buf_p1_y != NULL) {
					out_addr_p1_y = (UINT32)out_buf_p1_y;
				}
				nvt_dbg(IND, "out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_y);
			}

			if (out_buf_p1_size_u != 0) {
				if (out_buf_p1_u != NULL) {
					vPortFree((void *)out_buf_p1_u);
					out_buf_p1_u = NULL;
				}
				out_buf_p1_u = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_u));
				if (out_buf_p1_u != NULL) {
					out_addr_p1_u = (UINT32)out_buf_p1_u;
				}
				nvt_dbg(IND, "out_buf_p1_u.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_u);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p1_size_y != 0) {

				if (out_buf_p1_y != NULL) {
					vPortFree((void *)out_buf_p1_y);
					out_buf_p1_y = NULL;
				}
				out_buf_p1_y = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(out_buf_p1_size_y));


				if (out_buf_p1_y != NULL) {
					out_addr_p1_y = (UINT32)out_buf_p1_y;
				}
				nvt_dbg(IND, "out_buf_p1_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p1_y);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p1_y;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p1_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&out_addr);
	}

	{
		ime_out_img_info[1].path_id = KDRV_IME_PATH_ID_2;
		ime_out_img_info[1].path_en = ENABLE;
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
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV422:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_YUV420:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);

			out_lofs_p2_uv = out_lofs_p2_uv >> 1;
			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = IME_ALIGN_CEIL_8(out_lofs_p2_uv * (out_size_p2_v >> 1));

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = (UINT32)out_buf_p2_v.va_addr;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);
			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * out_size_p2_v);
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * out_size_p2_v);

			out_buf_p2_size_u = IME_ALIGN_CEIL_8(out_lofs_p2_uv * (out_size_p2_v >> 1));
			out_buf_p2_size_v = out_buf_p2_size_u;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = (UINT32)out_buf_p2_u.va_addr;
			//out_addr_p2_v = out_addr_p2_u;
			break;

		case KDRV_IME_OUT_FMT_Y:
			out_buf_p2_size_y = IME_ALIGN_CEIL_8(out_lofs_p2_y * (out_size_p2_v >> 1));
			out_buf_p2_size_u = 0;
			out_buf_p2_size_v = 0;

			//out_addr_p2_y = (UINT32)out_buf_p2_y.va_addr;
			//out_addr_p2_u = out_addr_p2_y;
			//out_addr_p2_v = out_addr_p2_y;
			break;

		default:
			break;
		}

		out_addr.path_id = KDRV_IME_PATH_ID_2;

		switch (ime_out_img_info[1].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			if (out_buf_p2_size_y != 0) {
				if (out_buf_p2_y != NULL) {
					vPortFree((void *)out_buf_p2_y);
					out_buf_p2_y = NULL;
				}
				out_buf_p2_y = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_y);


				if (out_buf_p2_y != NULL) {
					out_addr_p2_y = (UINT32)out_buf_p2_y;
				}
				nvt_dbg(IND, "out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_y);
			}

			if (out_buf_p2_size_u != 0) {
				if (out_buf_p2_u != NULL) {
					vPortFree((void *)out_buf_p2_u);
					out_buf_p2_u = NULL;
				}
				out_buf_p2_u = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_u);

				if (out_buf_p2_u != NULL) {
					out_addr_p2_u = (UINT32)out_buf_p2_u;
				}
				nvt_dbg(IND, "out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_u);
			}

			if (out_buf_p2_size_v != 0) {
				if (out_buf_p2_v != NULL) {
					vPortFree((void *)out_buf_p2_v);
					out_buf_p2_v = NULL;
				}
				out_buf_p2_v = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_v);

				if (out_buf_p2_v != NULL) {
					out_addr_p2_v = (UINT32)out_buf_p2_v;
				}
				nvt_dbg(IND, "out_buf_p2_v.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_v);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:

			if (out_buf_p2_size_y != 0) {
				if (out_buf_p2_y != NULL) {
					vPortFree((void *)out_buf_p2_y);
					out_buf_p2_y = NULL;
				}
				out_buf_p2_y = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_y);


				if (out_buf_p2_y != NULL) {
					out_addr_p2_y = (UINT32)out_buf_p2_y;
				}
				nvt_dbg(IND, "out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_y);
			}

			if (out_buf_p2_size_u != 0) {
				if (out_buf_p2_u != NULL) {
					vPortFree((void *)out_buf_p2_u);
					out_buf_p2_u = NULL;
				}
				out_buf_p2_u = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_u);

				if (out_buf_p2_u != NULL) {
					out_addr_p2_u = (UINT32)out_buf_p2_u;
				}
				nvt_dbg(IND, "out_buf_p2_u.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_u);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_u;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_v;
			break;

		case KDRV_IME_OUT_FMT_Y:
			if (out_buf_p2_size_y != 0) {
				if (out_buf_p2_y != NULL) {
					vPortFree((void *)out_buf_p2_y);
					out_buf_p2_y = NULL;
				}
				out_buf_p2_y = (UINT32 *)pvPortMalloc((size_t)out_buf_p2_size_y);
				if (out_buf_p2_y != NULL) {
					out_addr_p2_y = (UINT32)out_buf_p2_y;
				}
				nvt_dbg(IND, "out_buf_p2_y.vaddr = 0x%08x\r\n", (unsigned int)out_addr_p2_y);
			}

			out_addr.addr_info.addr_y  = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cb = (UINT32)out_addr_p2_y;
			out_addr.addr_info.addr_cr = (UINT32)out_addr_p2_y;
			break;

		default:
			break;
		}

		kdrv_ime_set(id, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&out_addr);
	}



	/* set isr-callback */
	kdrv_ime_set(id, KDRV_IME_PARAM_IPL_ISR_CB, (void *)&kdrv_ime_isr_cb);

	/* trigger */
	DBG_DUMP("ime d2d flow trigger\r\n");
	kdrv_ime_fmd_flag = 0;
	//kdrv_ime_set(handle, KDRV_IME_PARAM_IPL_TRIG, NULL);
	kdrv_ime_trigger(id, NULL, NULL, NULL);

	/*wait end*/
	DBG_DUMP("ime d2d flow wait end\r\n");

	while (!kdrv_ime_fmd_flag) {
		//msleep(10);

		vos_util_delay_ms(10);
	}
	DBG_DUMP("ime d2d flow end\r\n");

	/* pause */
	kdrv_ime_pause(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);

#if 0
	/* save image */
	for (i = 0; i < IME_EXAM_OUT_MAX; i ++) {

		//get output path parameters
		out_addr.path_id = DAL_IME_PATH_ID_1 + i;
		dal_ime_get(handle, DAL_IME_ITEM_OUT_ADDR, (void *)&out_addr);

		DBG_DUMP("path(%d) save ime output image w:%d, h:%d, y_addr:0x%.8x size:0x%.8x\r\n", i, Out_Data[i].LineOfs, Out_Data[i].SizeV, (UINT32)out_buf_p1_info[i].va_addr, out_buf_p1_info[i].size);
		fp = vos_file_open(Out_Data[i].FileName, O_CREAT | O_WRONLY | O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			DBG_ERR("failed in file open:%s\n", Out_Data[i].FileName);
			goto IME_CLOSE;
		}
		len = vos_file_write(fp, out_buf_p1_info[i].va_addr, out_buf_p1_info[i].size);
		vos_file_close(fp);
	}
#endif

	if ((ime_out_img_info[0].path_id == KDRV_IME_PATH_ID_1) && (ime_out_img_info[0].path_en == ENABLE)) {
		switch (ime_out_img_info[0].type) {
		case KDRV_IME_OUT_FMT_YUV444:
		case KDRV_IME_OUT_FMT_YUV422:
		case KDRV_IME_OUT_FMT_YUV420:
		case KDRV_IME_OUT_FMT_RGB:

			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_y, out_buf_p1_size_y);
			nvt_dbg(IND, "out_p1_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y);
			nvt_dbg(IND, "out_p1_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p1_size_y);
			len = vos_file_close(fp);
			if (out_buf_p1_y != NULL) {
				vPortFree((void *)out_buf_p1_y);
				out_buf_p1_y = NULL;
			}


			// save u to file
			fp = vos_file_open(out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_u, out_buf_p1_size_u);
			nvt_dbg(IND, "out_p1_buf_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_u);
			nvt_dbg(IND, "out_p1_buf_u.size = 0x%08x\r\n", (unsigned int)out_buf_p1_size_u);
			len = vos_file_close(fp);
			if (out_buf_p1_u != NULL) {
				vPortFree((void *)out_buf_p1_u);
				out_buf_p1_u = NULL;
			}

			// save v to file
			fp = vos_file_open(out_p1_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_v, out_buf_p1_size_v);
			nvt_dbg(IND, "out_p1_buf_v.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_v);
			nvt_dbg(IND, "out_p1_buf_v.size = 0x%08x\r\n", (unsigned int)out_buf_p1_size_v);
			len = vos_file_close(fp);
			if (out_buf_p1_v != NULL) {
				vPortFree((void *)out_buf_p1_v);
				out_buf_p1_v = NULL;
			}

			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_y, out_buf_p1_size_y);
			nvt_dbg(IND, "out_p1_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y);
			nvt_dbg(IND, "out_p1_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p1_size_y);
			len = vos_file_close(fp);
			if (out_buf_p1_y != NULL) {
				vPortFree((void *)out_buf_p1_y);
				out_buf_p1_y = NULL;
			}


			// save u to file
			fp = vos_file_open(out_p1_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_u, out_buf_p1_size_u);
			nvt_dbg(IND, "out_p1_buf_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_u);
			nvt_dbg(IND, "out_p1_buf_u.size = 0x%08x\r\n", (unsigned int)out_buf_p1_size_u);
			len = vos_file_close(fp);
			if (out_buf_p1_u != NULL) {
				vPortFree((void *)out_buf_p1_u);
				out_buf_p1_u = NULL;
			}
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(out_p1_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p1_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p1_y, out_buf_p1_size_y);
			nvt_dbg(IND, "out_p1_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p1_y);
			nvt_dbg(IND, "out_p1_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p1_y);
			len = vos_file_close(fp);
			if (out_buf_p1_y != NULL) {
				vPortFree((void *)out_buf_p1_y);
				out_buf_p1_y = NULL;
			}
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
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_y, out_buf_p2_size_y);
			nvt_dbg(IND, "out_p2_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			nvt_dbg(IND, "out_p2_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			len = vos_file_close(fp);
			if (out_buf_p2_y != NULL) {
				vPortFree((void *)out_buf_p2_y);
				out_buf_p2_y = NULL;
			}

			// save u to file
			fp = vos_file_open(out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_u, out_buf_p2_size_u);
			nvt_dbg(IND, "out_p2_buf_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_u);
			nvt_dbg(IND, "out_p2_buf_u.size = 0x%08x\r\n", (unsigned int)out_buf_p2_u);
			len = vos_file_close(fp);
			if (out_buf_p2_u != NULL) {
				vPortFree((void *)out_buf_p2_u);
				out_buf_p2_u = NULL;
			}

			// save v to file
			fp = vos_file_open(out_p2_raw_file_v, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_v);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_v, out_buf_p2_size_v);
			nvt_dbg(IND, "out_p2_buf_v.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_v);
			nvt_dbg(IND, "out_p2_buf_v.size = 0x%08x\r\n", (unsigned int)out_buf_p2_v);
			len = vos_file_close(fp);
			if (out_buf_p2_v != NULL) {
				vPortFree((void *)out_buf_p2_v);
				out_buf_p2_v = NULL;
			}
			break;

		case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		case KDRV_IME_OUT_FMT_Y_PACK_UV420:
			// save y to file
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_y, out_buf_p2_size_y);
			nvt_dbg(IND, "out_p2_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			nvt_dbg(IND, "out_p2_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			len = vos_file_close(fp);
			if (out_buf_p2_y != NULL) {
				vPortFree((void *)out_buf_p2_y);
				out_buf_p2_y = NULL;
			}

			// save u to file
			fp = vos_file_open(out_p2_raw_file_u, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_u);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_u, out_buf_p2_size_u);
			nvt_dbg(IND, "out_p2_buf_u.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_u);
			nvt_dbg(IND, "out_p2_buf_u.size = 0x%08x\r\n", (unsigned int)out_buf_p2_u);
			len = vos_file_close(fp);
			if (out_buf_p2_u != NULL) {
				vPortFree((void *)out_buf_p2_u);
				out_buf_p2_u = NULL;
			}
			break;

		case KDRV_IME_OUT_FMT_Y:
			// save y to file
			fp = vos_file_open(out_p2_raw_file_y, O_CREAT | O_WRONLY | O_SYNC, 0);
			if (fp == (VOS_FILE) - 1) {
				DBG_ERR("failed in file open:%s\n", out_p2_raw_file_y);
				return FALSE;
			}
			len = vos_file_write(fp, (void *)out_buf_p2_y, out_buf_p2_size_y);
			nvt_dbg(IND, "out_p2_buf_y.vaddr = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			nvt_dbg(IND, "out_p2_buf_y.size = 0x%08x\r\n", (unsigned int)out_buf_p2_y);
			len = vos_file_close(fp);
			if (out_buf_p2_y != NULL) {
				vPortFree((void *)out_buf_p2_y);
				out_buf_p2_y = NULL;
			}
			break;

		default:
			break;
		}
	}

	/* close */
	kdrv_ime_close(KDRV_CHIP0, KDRV_VIDEOPROCS_IME_ENGINE0);

	kdrv_ime_uninit();

//IME_END:
	DBG_DUMP("ime d2d flow free memory\r\n");
	/* free buffer */
	if (in_buf_y != NULL) {
		vPortFree((void *)in_buf_y);
		in_buf_y = NULL;
	}

	if (in_buf_u != NULL) {
		vPortFree((void *)in_buf_u);
		in_buf_u = NULL;
	}

	if (in_buf_v != NULL) {
		vPortFree((void *)in_buf_v);
		in_buf_v = NULL;
	}

	return 0;
#else
	return 0;
#endif
}

#if !defined (CONFIG_NVT_SMALL_HDAL)

int nvt_ime_hw_power_saving(unsigned char argc, char **pargv)
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
		pll_setClkAutoGating(IME_M_GCLK);
		pll_setPclkAutoGating(IME_GCLK);
	} else {
		pll_clearClkAutoGating(IME_M_GCLK);
		pll_clearPclkAutoGating(IME_GCLK);
	}

	DBG_IND("end\r\n");
#else
	DBG_WRN("Not defined in this platform!\r\n");
#endif

	return 0;
}

int nvt_ime_fw_power_saving(unsigned char argc, char **pargv)
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
		fw_ime_power_saving_en = TRUE;
	} else {
		fw_ime_power_saving_en = FALSE;
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





