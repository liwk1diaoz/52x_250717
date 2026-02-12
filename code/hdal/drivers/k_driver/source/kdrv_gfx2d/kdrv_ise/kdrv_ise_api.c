#if defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#include "plat/interrupt.h"
#endif
#include "kdrv_type.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/file.h"
#include "kwrap/cpu.h"
#include "kwrap/util.h"
#include "kwrap/mem.h"
#include "comm/hwclock.h"
#include "plat/nvt-sramctl.h"
#include "kdrv_type.h"
#include "kdrv_ise_int_api.h"
#include "kdrv_ise_int_dbg.h"
#include "kdrv_ise_int.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "ise_eng.h"

#if 0
#endif

#define KDRV_ISE_DEBUG_CMD_ENABLE (0)

BOOL nvt_kdrv_ise_cmd_w_reg(unsigned char argc, char **pargv);
BOOL nvt_kdrv_ise_cmd_dump(unsigned char argc, char **pargv);
BOOL nvt_kdrv_ise_cmd_dump_t(unsigned char argc, char **pargv);
BOOL nvt_kdrv_ise_cmd_set_dbg_level(unsigned char argc, char **pargv);

#if !defined (CONFIG_NVT_SMALL_HDAL)
#if (KDRV_ISE_DEBUG_CMD_ENABLE)
BOOL nvt_kdrv_ise_cmd_eng_d2d_test(unsigned char argc, char **pargv);
BOOL nvt_kdrv_ise_cmd_kdrv_d2d_test(unsigned char argc, char **pargv);
BOOL nvt_kdrv_ise_cmd_kdrv_obsolete_test(unsigned char argc, char **pargv);
#endif
#endif

//--------------------------------------------------------------
static SXCMD_BEGIN(kdrv_ise_cmd_tbl, "kdrv_ise")
SXCMD_ITEM("w_reg %",			nvt_kdrv_ise_cmd_w_reg, 			"write register(ofs, val)")
SXCMD_ITEM("dump",				nvt_kdrv_ise_cmd_dump,				"dump kdrv info")
SXCMD_ITEM("dump_t %",			nvt_kdrv_ise_cmd_dump_t,			"dump kdrv timestamp(times)")
SXCMD_ITEM("dbg_level %",		nvt_kdrv_ise_cmd_set_dbg_level,		"set debug level(lv)")

#if !defined (CONFIG_NVT_SMALL_HDAL)
#if (KDRV_ISE_DEBUG_CMD_ENABLE)
SXCMD_ITEM("eng_d2d %",			nvt_kdrv_ise_cmd_eng_d2d_test, 		"engine d2d test")
SXCMD_ITEM("kdrv_d2d %",		nvt_kdrv_ise_cmd_kdrv_d2d_test, 	"kdrv d2d test")
SXCMD_ITEM("kdrv_obsolete %",	nvt_kdrv_ise_cmd_kdrv_obsolete_test,"kdrv obsolete d2d test(backward compatible)")
#endif
#endif

SXCMD_END()

#if defined(__LINUX)
int kdrv_ise_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_ise_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], kdrv_ise_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = kdrv_ise_cmd_tbl[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_ise, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_ise_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], kdrv_ise_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = kdrv_ise_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		return -1;
	}
	return 0;
}
#endif

//-------------------------------------------------------------

BOOL nvt_kdrv_ise_cmd_dump(unsigned char argc, char **pargv)
{
	kdrv_ise_dump_all();
	return TRUE;
}

BOOL nvt_kdrv_ise_cmd_dump_t(unsigned char argc, char **pargv)
{
	UINT32 cnt;

	sscanf(pargv[0], "%d", (int *)&cnt);
	kdrv_ise_set_dump_ts(cnt);

	return TRUE;
}

BOOL nvt_kdrv_ise_cmd_set_dbg_level(unsigned char argc, char **pargv)
{
	UINT32 lv;

	sscanf(pargv[0], "%d", (int *)&lv);
	kdrv_ise_set_dbg_level(lv);

	return TRUE;
}

#if 0
#endif

BOOL nvt_kdrv_ise_cmd_w_reg(unsigned char argc, char **pargv)
{
	static UINT32 is_opened = 0;

	ISE_ENG_HANDLE *p_eng;
	UINT32 ofs, val;

	sscanf(pargv[0], "%x", (unsigned int *)&ofs);
	sscanf(pargv[1], "%x", (unsigned int *)&val);

	p_eng = ise_eng_get_handle(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	if (p_eng != NULL) {
		if (is_opened == 0) {
			if (ise_eng_open(p_eng) == E_OK) {
				is_opened = 1;
			} else {
				DBG_ERR("open engine failed\r\n");
			}
		}

		if (is_opened) {
			DBG_DUMP("write reg ofs 0x%.8x, val 0x%.8x\r\n", (unsigned int)ofs, (unsigned int)val);
			ise_eng_write_reg(p_eng, ofs, val);
		}
	}

	return TRUE;
}

#if 0
#endif

/*
	kdrv ise test cmd
*/

#if (KDRV_ISE_DEBUG_CMD_ENABLE)
typedef struct {
	struct vos_mem_cma_info_t info;
	VOS_MEM_CMA_HDL hdl;
} KDRV_ISE_EXAM_MEM;

typedef struct {
	CHAR *filename;
	UINT32 width;
	UINT32 height;
	UINT32 lofs;
	UINT32 fmt;
	UINT32 bufsize;
	KDRV_ISE_EXAM_MEM buf;
} KDRV_ISE_EXAM_IMG;

static UINT32 g_kdrv_ise_job_cb_num;

INT32 nvt_kdrv_ise_wait_job_cb(UINT32 num)
{
	UINT32 timeout_cnt;

	timeout_cnt = 1000;
	while (g_kdrv_ise_job_cb_num < num) {
		vos_util_delay_ms(10);
		timeout_cnt--;
		if (timeout_cnt == 0) {
			break;
		}
	}

	return timeout_cnt;
}

INT32 nvt_kdrv_ise_cb(VOID *callback_info, VOID *user_data)
{
	KDRV_ISE_CALLBACK_INFO *p_info;

	p_info = (KDRV_ISE_CALLBACK_INFO *)callback_info;
	if (p_info != NULL) {
		g_kdrv_ise_job_cb_num ++;
	}

	//DBG_DUMP("kdrv_ise_cb: job_id %d, job_num %d, done %d, err %d\r\n",
	//		p_info->job_id, p_info->job_num, p_info->done_num, p_info->err_num);

	return 0;
}

INT32 nvt_kdrv_ise_mem_alloc(KDRV_ISE_EXAM_MEM *p_mem, UINT32 size)
{
	if (vos_mem_init_cma_info(&p_mem->info, VOS_MEM_CMA_TYPE_CACHE, size) != 0) {
		DBG_ERR("init cma failed\r\n");
		return E_NOMEM;
	}

	p_mem->hdl = vos_mem_alloc_from_cma(&p_mem->info);
	if (p_mem->hdl == NULL) {
		DBG_ERR("alloc cma failed\r\n");
		return E_NOMEM;
	}

	return E_OK;
}

INT32 nvt_kdrv_ise_mem_free(KDRV_ISE_EXAM_MEM *p_mem)
{
	if (vos_mem_release_from_cma(p_mem->hdl) != 0) {
		DBG_ERR("release cma failed\r\n");
		return E_NOMEM;
	}
	return E_OK;
}

#if 0
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)
BOOL nvt_kdrv_ise_cmd_eng_d2d_test(unsigned char argc, char **pargv)
{
#if defined(__LINUX)
	CHAR *in_file = "//mnt//sd//exam_ise//in.raw";
	CHAR *out_file = "//mnt//sd//exam_ise//out.raw";
#elif defined (__FREERTOS)
	CHAR *in_file = "A:\\exam_ise\\in.raw";
	CHAR *out_file = "A:\\exam_ise\\out.raw";
#endif

	VOS_FILE fp;
	UINT32 in_width = 1920;
	UINT32 in_height = 1080;
	UINT32 in_buf_size = 0;
	KDRV_ISE_EXAM_MEM in_buf;

	UINT32 out_width = 1280;
	UINT32 out_height = 720;
	UINT32 out_buf_size = 0;
	KDRV_ISE_EXAM_MEM out_buf;

	/* ALLOCATE BUFFER
		allocate y8 buffer
	*/
	in_buf_size = in_width * in_height;
	if (nvt_kdrv_ise_mem_alloc(&in_buf, in_buf_size) != E_OK) {
		DBG_ERR("new input buffer failed, %d\r\n", in_buf_size);
		return -1;
	}
	DBG_DUMP("ise eng d2d input buffer addr 0x%.8x\r\n", (unsigned int)in_buf.info.vaddr);

	out_buf_size = out_width * out_height;
	if (nvt_kdrv_ise_mem_alloc(&out_buf, out_buf_size) != E_OK) {
		DBG_ERR("new output buffer failed, %d\r\n", out_buf_size);
		return -1;
	}
	DBG_DUMP("ise eng d2d output buffer addr 0x%.8x\r\n", (unsigned int)out_buf.info.vaddr);

	/* READ INPUT FILE TO BUFFER */
	fp = vos_file_open(in_file, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
	    DBG_ERR("failed in file open:%s\n", in_file);
		return -1;
	}
	vos_file_read(fp, (void *)in_buf.info.vaddr, in_buf_size);
	vos_file_close(fp);

	/* OPEN & CONFIG & TRIGGER ISE */
	{
		ISE_ENG_HANDLE *p_eng;
		ISE_ENG_REG reg;
		ISE_ENG_STRIPE_INFO strp_info;
		ISE_ENG_SCALE_FACTOR_INFO scl_factor;
		ISE_ENG_SCALE_FILTER_INFO scl_filter;
		UINT32 scl_method = ISE_ENG_SCALE_INTEGRATION;

		p_eng = ise_eng_get_handle(KDRV_CHIP0, KDRV_GFX2D_ISE0);
		ise_eng_open(p_eng);

		/* parameter calculation */
		ise_eng_cal_stripe(in_width, out_width, scl_method, &strp_info);
		ise_eng_cal_scale_factor(in_width, in_height, out_width, out_height, scl_method, &scl_factor);
		ise_eng_cal_scale_filter(in_width, in_height, out_width, out_height, &scl_filter);

		/* register configuration */
		/* interrupt enable */
		reg = ise_eng_gen_int_en_reg(ISE_ENG_INTERRUPT_FMD);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		/* input image */
		reg = ise_eng_gen_in_ctrl_reg(0, scl_method, 0, 0, 0, strp_info.overlap_mode);
		reg = ise_eng_gen_in_size_reg(in_width, in_height);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_in_strp_reg(strp_info.stripe_size, strp_info.overlap_size_sel);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_in_lofs_reg(in_width);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_in_addr_reg(in_buf.info.paddr);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		/* output image */
		reg = ise_eng_gen_out_size_reg(out_width, out_height);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_out_lofs_reg(out_width);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_out_addr_reg(out_buf.info.paddr);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_out_ctrl_reg0(&scl_factor, &scl_filter);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_out_ctrl_reg1(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		/* scale parameters */
		reg = ise_eng_gen_scl_ofs_reg0(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_ofs_reg1(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		reg = ise_eng_gen_scl_isd_base_reg(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_fact_reg0(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_fact_reg1(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_fact_reg2(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		reg = ise_eng_gen_scl_isd_coef_ctrl_reg(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg0(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg1(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg2(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg3(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg4(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg5(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_reg6(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		reg = ise_eng_gen_scl_isd_coef_v_reg0(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg1(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg2(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg3(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg4(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg5(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_reg6(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		reg = ise_eng_gen_scl_isd_coef_h_all(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_h_half(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		reg = ise_eng_gen_scl_isd_coef_v_all(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
		reg = ise_eng_gen_scl_isd_coef_v_half(&scl_factor);
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);

		/* trigger ise, wait frm end */
		DBG_DUMP("---- engine trigger ----\r\n");
		ise_eng_trig_single(p_eng);

		vos_util_delay_ms(100);

		/* close engine */
		ise_eng_close(p_eng);
	}

	/* SAVE OUTPUT BUFFER */
	fp = vos_file_open(out_file, O_CREAT|O_WRONLY|O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
	    DBG_ERR("failed in file open:%s\n", in_file);
		return -1;
	}
	vos_file_write(fp, (void *)out_buf.info.vaddr, out_buf_size);
	vos_file_close(fp);

	/* RELEASE BUFFER */
	nvt_kdrv_ise_mem_free(&in_buf);
	nvt_kdrv_ise_mem_free(&out_buf);

	return TRUE;
}

BOOL nvt_kdrv_ise_cmd_kdrv_d2d_test(unsigned char argc, char **pargv)
{
#if defined(__LINUX)
	KDRV_ISE_EXAM_IMG in_img = {
		"//mnt//sd//exam_ise//in.raw",
		1920, 1080, 1920, KDRV_ISE_Y8,
		0, {}
	};

	KDRV_ISE_EXAM_IMG out_img[2] = {
		{
			"//mnt//sd//exam_ise//out_1280_720.raw",
			1280, 720, 1280, KDRV_ISE_Y8,
			0, {}
		},
		{
			"//mnt//sd//exam_ise//out_640_480.raw",
			640, 480, 640, KDRV_ISE_Y8,
			0, {}
		}
	};
#elif defined (__FREERTOS)
	KDRV_ISE_EXAM_IMG in_img = {
		"A:\\exam_ise\\in.raw",
		1920, 1080, 1920, KDRV_ISE_Y8,
		0, {}
	};

	KDRV_ISE_EXAM_IMG out_img[2] = {
		{
			"A:\\exam_ise\\out_1280_720.raw",
			1280, 720, 1280, KDRV_ISE_Y8,
			0, {}
		},
		{
			"A:\\exam_ise\\out_640_480.raw",
			640, 480, 640, KDRV_ISE_Y8,
			0, {}
		}
	};
#endif

	VOS_FILE fp;
	KDRV_CALLBACK_FUNC callback_func;
	KDRV_ISE_TRIG_PARAM trig_param;
	KDRV_ISE_IO_CFG iocfg;
	KDRV_ISE_IO_CFG iocfg_2;
	UINT32 dev_id;
	UINT32 trig_times;
	UINT32 test_mode;
	UINT32 i;
	UINT32 wait_cb_num;

	sscanf(pargv[0], "%d", (int *)&trig_times);
	sscanf(pargv[1], "%d", (int *)&test_mode);
	sscanf(pargv[2], "%d", (int *)&trig_param.mode);

	/* ALLOCATE BUFFER
		allocate y8 buffer
	*/
	in_img.bufsize= in_img.width * in_img.height;
	if (nvt_kdrv_ise_mem_alloc(&in_img.buf, in_img.bufsize) != E_OK) {
		DBG_ERR("new input buffer failed, %d\r\n", in_img.bufsize);
		return -1;
	}
	DBG_DUMP("ise eng d2d input buffer addr 0x%.8x\r\n", (unsigned int)in_img.buf.info.vaddr);

	for (i = 0; i < 2; i++) {
		out_img[i].bufsize = out_img[i].width * out_img[i].height;
		if (nvt_kdrv_ise_mem_alloc(&out_img[i].buf, out_img[i].bufsize) != E_OK) {
			DBG_ERR("new output buffer failed, %d\r\n", out_img[i].bufsize);
			return -1;
		}
		DBG_DUMP("ise eng d2d output buffer[%d] addr 0x%.8x\r\n", (int)i, (unsigned int)out_img[i].buf.info.vaddr);
	}

	/* READ INPUT FILE TO BUFFER */
	fp = vos_file_open(in_img.filename, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
	    DBG_ERR("failed in file open:%s\n", in_img.filename);
		return -1;
	}
	vos_file_read(fp, (void *)in_img.buf.info.vaddr, in_img.bufsize);
	vos_file_close(fp);

	iocfg.io_pack_fmt = in_img.fmt;
	iocfg.argb_out_mode = KDRV_ISE_OUTMODE_ARGB8888;
	iocfg.scl_method = KDRV_ISE_INTEGRATION;
	iocfg.in_width = in_img.width;
	iocfg.in_height = in_img.height;
	iocfg.in_lofs = in_img.lofs;
	iocfg.in_addr = (UINT32)in_img.buf.info.vaddr;
	iocfg.in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
	iocfg.out_width = out_img[0].width;
	iocfg.out_height = out_img[0].height;
	iocfg.out_lofs = out_img[0].lofs;
	iocfg.out_addr = (UINT32)out_img[0].buf.info.vaddr;
	iocfg.out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
	iocfg.phy_addr_en = DISABLE;

	iocfg_2.io_pack_fmt = iocfg.io_pack_fmt;
	iocfg_2.argb_out_mode = iocfg.argb_out_mode;
	iocfg_2.scl_method = KDRV_ISE_BILINEAR;
	iocfg_2.in_width = iocfg.out_width;
	iocfg_2.in_height = iocfg.out_height;
	iocfg_2.in_lofs = iocfg.out_lofs;
	iocfg_2.in_addr = iocfg.out_addr;
	iocfg_2.in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
	iocfg_2.out_width = out_img[1].width;
	iocfg_2.out_height = out_img[1].height;
	iocfg_2.out_lofs = out_img[1].lofs;
	iocfg_2.out_addr = (UINT32)out_img[1].buf.info.vaddr;
	iocfg_2.out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
	iocfg_2.phy_addr_en = DISABLE;

	callback_func.callback = nvt_kdrv_ise_cb;
	callback_func.reserve_buf = NULL;
	callback_func.free_buf = NULL;

	kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0);

	dev_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0);
	wait_cb_num = g_kdrv_ise_job_cb_num;
	while (trig_times) {
		switch (test_mode) {
		case 0:
			if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
				kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg);
				kdrv_ise_trigger(dev_id, &trig_param, &callback_func, NULL);
				wait_cb_num++;
				trig_times--;
			}
			break;

		case 1:
			if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
				kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg_2);
				kdrv_ise_trigger(dev_id, &trig_param, &callback_func, NULL);
				wait_cb_num++;
				trig_times--;
			}
			break;

		case 2:
			if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
				kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg);

				if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
					kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg_2);
					kdrv_ise_trigger(dev_id, &trig_param, &callback_func, NULL);
					wait_cb_num++;
					trig_times--;
				} else {
					DBG_DUMP("gen second node failed, flush job\r\n");
					kdrv_ise_set(dev_id, KDRV_ISE_PARAM_FLUSH_JOB, NULL);
				}
			}
			break;

		case 3:
			if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
				kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg);
				kdrv_ise_trigger(dev_id, &trig_param, &callback_func, NULL);
				wait_cb_num++;
				trig_times--;
			}

			if (kdrv_ise_set(dev_id, KDRV_ISE_PARAM_GEN_NODE, NULL) == E_OK) {
				kdrv_ise_set(dev_id, KDRV_ISE_PARAM_IO_CFG, &iocfg_2);
				kdrv_ise_trigger(dev_id, &trig_param, &callback_func, NULL);
				wait_cb_num++;
				trig_times--;
			}
			break;

		default:
			break;
		}
		vos_util_delay_us(2000);
	}
	nvt_kdrv_ise_wait_job_cb(wait_cb_num);
	kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0);

	/* SAVE OUTPUT BUFFER */
	for (i = 0; i < 2; i++) {
		fp = vos_file_open(out_img[i].filename, O_CREAT|O_WRONLY|O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
		    DBG_ERR("failed in file open:%s\n", out_img[i].filename);
			return -1;
		}
		vos_file_write(fp, (void *)out_img[i].buf.info.vaddr, out_img[i].bufsize);
		vos_file_close(fp);

		DBG_DUMP("save 0x%.8x to file %s\r\n", (unsigned int)out_img[i].buf.info.vaddr, out_img[i].filename);
	}

	/* RELEASE BUFFER */
	nvt_kdrv_ise_mem_free(&in_img.buf);
	nvt_kdrv_ise_mem_free(&out_img[0].buf);
	nvt_kdrv_ise_mem_free(&out_img[1].buf);

	return TRUE;
}

BOOL nvt_kdrv_ise_cmd_kdrv_obsolete_test(unsigned char argc, char **pargv)
{
#if defined (__LINUX)
	CHAR *in_file = "//mnt//sd//exam_ise//in.raw";
	CHAR *out_file = "//mnt//sd//exam_ise//out.raw";
#elif defined (__FREERTOS)
	CHAR *in_file = "A:\\exam_ise\\in.raw";
	CHAR *out_file = "A:\\exam_ise\\out.raw";
#endif

	VOS_FILE fp;
	UINT32 in_width = 1920;
	UINT32 in_height = 1080;
	UINT32 in_buf_size = 0;
	KDRV_ISE_EXAM_MEM in_buf;

	UINT32 out_width = 1280;
	UINT32 out_height = 720;
	UINT32 out_buf_size = 0;
	KDRV_ISE_EXAM_MEM out_buf;

	KDRV_ISE_TRIGGER_PARAM trig_param;
	KDRV_ISE_OPENOBJ open_obj;
	KDRV_ISE_MODE ise_param = {0};
	UINT32 dev_id;
	UINT32 trig_times;
	UINT32 trig_mode;
	UINT32 phy_addr_en;

	sscanf(pargv[0], "%d", (int *)&trig_times);
	sscanf(pargv[1], "%d", (int *)&trig_mode);
	if (argc > 2) {
		sscanf(pargv[2], "%d", (int *)&phy_addr_en);
	}

	/* ALLOCATE BUFFER
		allocate y8 buffer
	*/
	in_buf_size = in_width * in_height;
	if (nvt_kdrv_ise_mem_alloc(&in_buf, in_buf_size) != E_OK) {
		DBG_ERR("new input buffer failed, %d\r\n", in_buf_size);
		return -1;
	}
	DBG_DUMP("ise eng d2d input buffer addr 0x%.8x\r\n", (unsigned int)in_buf.info.vaddr);

	out_buf_size = out_width * out_height;
	if (nvt_kdrv_ise_mem_alloc(&out_buf, out_buf_size) != E_OK) {
		DBG_ERR("new output buffer failed, %d\r\n", out_buf_size);
		return -1;
	}
	DBG_DUMP("ise eng d2d output buffer addr 0x%.8x\r\n", (unsigned int)out_buf.info.vaddr);

	/* READ INPUT FILE TO BUFFER */
	fp = vos_file_open(in_file, O_RDONLY, 0);
	if (fp == (VOS_FILE)(-1)) {
	    DBG_ERR("failed in file open:%s\n", in_file);
		return -1;
	}
	vos_file_read(fp, (void *)in_buf.info.vaddr, in_buf_size);
	vos_file_close(fp);

	/* ISE OBSOLETE TEST FLOW */
	open_obj.ise_clock_sel = 480;
	dev_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0);
	kdrv_ise_set(dev_id, KDRV_ISE_PARAM_OPENCFG, (void *)&open_obj);
	kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0);

	if (phy_addr_en) {
		ise_param.in_addr = vos_cpu_get_phy_addr((VOS_ADDR)in_buf.info.vaddr);
	} else {
		ise_param.in_addr = (UINT32)in_buf.info.vaddr;
	}
	ise_param.in_width = in_width;
	ise_param.in_height = in_height;
	ise_param.in_lofs = in_width;
	ise_param.in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
	ise_param.io_pack_fmt = KDRV_ISE_Y8;

	if (phy_addr_en) {
		ise_param.out_addr = vos_cpu_get_phy_addr((VOS_ADDR)out_buf.info.vaddr);
	} else {
		ise_param.out_addr = (UINT32)out_buf.info.vaddr;
	}
	ise_param.out_width = out_width;
	ise_param.out_height = out_height;
	ise_param.out_lofs = out_width;
	ise_param.out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;

	ise_param.scl_method = KDRV_ISE_INTEGRATION;
	ise_param.phy_addr_en = phy_addr_en;

	while (trig_times) {
		UINT32 ts = 0;

		if (trig_mode == 0) {
			kdrv_ise_set(dev_id, KDRV_ISE_PARAM_MODE, (void *)&ise_param);
			trig_param.wait_end = 1;
			trig_param.time_out_ms = 1000;

			ts = hwclock_get_counter();
			if (kdrv_ise_trigger(dev_id, &trig_param, NULL, NULL) != E_OK) {
				DBG_ERR("trigger failed\r\n");
			}
			ts = hwclock_get_counter() - ts;
		} else {
			KDRV_ISE_EXAM_MEM ise_ll_param_buf;
			UINT32 buf_size;
			KDRV_ISE_MODE *ise_ll_param;
			UINT32 i;

			ise_param.ll_job_nums = 20;
			buf_size = sizeof(KDRV_ISE_MODE) * 20;
			if (nvt_kdrv_ise_mem_alloc(&ise_ll_param_buf, buf_size) != E_OK) {
				DBG_ERR("new ll param buffer failed, %d\r\n", buf_size);
				return -1;
			}
			ise_ll_param = (KDRV_ISE_MODE *)ise_ll_param_buf.info.vaddr;

			for (i = 0; i < 20; i++) {
				memcpy((void *)&ise_ll_param[i], (void *)&ise_param, sizeof(KDRV_ISE_MODE));
			}

			kdrv_ise_set(dev_id, KDRV_ISE_LL_PARAM_MODE, (void *)ise_ll_param);
			ts = hwclock_get_counter();
			if (kdrv_ise_linked_list_trigger(dev_id) != E_OK) {
				DBG_ERR("trigger failed\r\n");
			}
			ts = hwclock_get_counter() - ts;

			nvt_kdrv_ise_mem_free(&ise_ll_param_buf);
		}
		DBG_DUMP("ise process time %d\r\n", ts);

		trig_times--;
	}

	kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0);

	/* SAVE OUTPUT BUFFER */
	fp = vos_file_open(out_file, O_CREAT|O_WRONLY|O_SYNC, 0);
	if (fp == (VOS_FILE)(-1)) {
	    DBG_ERR("failed in file open:%s\n", out_file);
		return -1;
	}
	vos_file_write(fp, (void *)out_buf.info.vaddr, out_buf_size);
	vos_file_close(fp);

	/* RELEASE BUFFER */
	nvt_kdrv_ise_mem_free(&in_buf);
	nvt_kdrv_ise_mem_free(&out_buf);

	return TRUE;
}
#endif //#if (KDRV_ISE_DEBUG_CMD_ENABLE)
#endif

#if 0
#endif

#if defined (__FREERTOS)
/* rtos drv_init api */
irqreturn_t nvt_kdrv_ise_drv_isr(int irq, void *p_eng_handle)
{
	ise_eng_isr(p_eng_handle);
	return IRQ_HANDLED;
}

void kdrv_ise_drv_init(void)
{
	ISE_ENG_HANDLE *p_eng_handle;
	ISE_ENG_HANDLE m_eng;
	ISE_ENG_CTL eng_ctl;
	KDRV_ISE_HANDLE m_kdrv_hdl;
	KDRV_ISE_CTL kdrv_ctl;
	KDRV_ISE_CTX_BUF_CFG init_buf_cfg;

	/* temporay, init one ise */
	m_eng.reg_io_base = 0xF0C90000;
	m_eng.pclk = NULL;
	m_eng.clock_rate = 480;
	m_eng.sram_id = ISE_SD;
	m_eng.isr_cb = NULL;
	m_eng.chip_id = KDRV_CHIP0;
	m_eng.eng_id = KDRV_GFX2D_ISE0;
	eng_ctl.p_eng = &m_eng;
	eng_ctl.chip_num = 1;
	eng_ctl.eng_num = 1;
	eng_ctl.total_ch = 1;
	ise_eng_init(&eng_ctl);

	p_eng_handle = ise_eng_get_handle(m_eng.chip_id, m_eng.eng_id);

	m_kdrv_hdl.chip_id = m_eng.chip_id;
	m_kdrv_hdl.eng_id = m_eng.eng_id;
	kdrv_ctl.p_hdl = &m_kdrv_hdl;
	kdrv_ctl.chip_num = eng_ctl.chip_num;
	kdrv_ctl.eng_num = eng_ctl.eng_num;
	kdrv_ctl.total_ch = eng_ctl.total_ch;
	kdrv_ise_sys_init(&kdrv_ctl);

	/* register IRQ here*/
	request_irq(INT_ID_ISE, nvt_kdrv_ise_drv_isr, IRQF_TRIGGER_HIGH, "ISE_INT", p_eng_handle);

	/* for backward compatible */
	init_buf_cfg.reserved = 0;
	kdrv_ise_init(init_buf_cfg, 0, 0);

	//nvt_dbg(IND, "ise initialization...\r\n");
}
#endif
