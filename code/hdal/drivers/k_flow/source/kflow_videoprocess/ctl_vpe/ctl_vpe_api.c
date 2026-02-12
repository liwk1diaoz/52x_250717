#include "kwrap/cpu.h"
#include "kwrap/type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kwrap/util.h"
#include "kwrap/file.h"
#include "comm/hwclock.h"
#include "ctl_vpe_api.h"
#include "ctl_vpe_drv.h"
#include "ctl_vpe_dbg.h"
#include "ctl_vpe_int.h"
#include "ctl_vpe_isp_int.h"
#include "kflow_videoprocess/ctl_vpe.h"
#include "kflow_videoprocess/ctl_vpe_isp.h"
#include "kflow_common/nvtmpp.h"

#define CTL_VPE_TEST_CMD_ENABLE (0)

unsigned int ctl_vpe_debug_level = NVT_DBG_ERR;

#if CTL_VPE_MODULE_ENABLE
BOOL nvt_ctl_vpe_api_test_d2d(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_test_cmd(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_test_cfg(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_dump(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_dump_kdrv_cfg(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_isp_debug(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_set_dbg_level(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_save_yuv(unsigned char argc, char **pargv);
BOOL nvt_ctl_vpe_api_eng_hang_cfg(unsigned char argc, char **pargv);

static SXCMD_BEGIN(ctl_vpe_cmd_tbl, "ctl_vpe")
#if CTL_VPE_TEST_CMD_ENABLE
SXCMD_ITEM("test_d2d",		nvt_ctl_vpe_api_test_d2d, 		"ctl vpe d2d test")
SXCMD_ITEM("test_cmd",		nvt_ctl_vpe_api_test_cmd, 		"ctl vpe d2d test step by step")
SXCMD_ITEM("test_cfg",		nvt_ctl_vpe_api_test_cfg, 		"ctl vpe d2d test config")
#endif
SXCMD_ITEM("dump",			nvt_ctl_vpe_api_dump, 			"ctl vpe dump")
SXCMD_ITEM("dump_jobcfg",	nvt_ctl_vpe_api_dump_kdrv_cfg,	"ctl vpe dump kdrv job config")
SXCMD_ITEM("isp_debug",		nvt_ctl_vpe_api_isp_debug,		"isp debug mode(enable/disable)")
SXCMD_ITEM("dbg_level",		nvt_ctl_vpe_api_set_dbg_level,	"ctl vpe debug msg level(lv)")
SXCMD_ITEM("saveyuv",		nvt_ctl_vpe_api_save_yuv,		"save yuv(hdl_name input output path)")
SXCMD_ITEM("eng_hang",		nvt_ctl_vpe_api_eng_hang_cfg,	"eng hang cfg(info, file, timeout_ms)")
SXCMD_END()

#if defined(__LINUX)
int ctl_vpe_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_vpe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], ctl_vpe_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = ctl_vpe_cmd_tbl[loop].p_func(argc-1, &argv[1]);
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
MAINFUNC_ENTRY(ctl_vpe, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_vpe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ctl_vpe_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = ctl_vpe_cmd_tbl[loop].p_func(argc-2, &argv[2]);
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

BOOL nvt_ctl_vpe_api_dump(unsigned char argc, char **pargv)
{
	if (argc ==0 || strcmp(pargv[0], "cfg") == 0) {
		ctl_vpe_dump_all(ctl_vpe_int_printf);
	} else if (strcmp(pargv[0], "reg") == 0) {
		debug_dumpmem(0xf0cd0000, 0x1200);
	} else if (strcmp(pargv[0], "time") == 0) {
		ctl_vpe_dump_job_ts(ctl_vpe_int_printf);
	} else if (strcmp(pargv[0], "all") == 0) {
		ctl_vpe_dump_job_ts(ctl_vpe_int_printf);
		ctl_vpe_dump_all(ctl_vpe_int_printf);
		debug_dumpmem(0xf0cd0000, 0x1200);
	} else {
		DBG_DUMP("MUST SET DUMP MODE\r\n");
		DBG_DUMP("cfg: ctl vpe config dump\r\n");
		DBG_DUMP("reg: vpe register dump\r\n");
		DBG_DUMP("time: job timestamp\r\n");
		return TRUE;
	}

	return TRUE;
}

BOOL nvt_ctl_vpe_api_dump_kdrv_cfg(unsigned char argc, char **pargv)
{
	UINT32 n;

	if (argc == 0) {
		n = 1;
	} else {
		sscanf(pargv[0], "%d", (int *)&n);
	}
	ctl_vpe_process_dbg_dump_cfg(1, n);

	return TRUE;
}

BOOL nvt_ctl_vpe_api_isp_debug(unsigned char argc, char **pargv)
{
	UINT32 en;

	if (argc == 0) {
		DBG_ERR("must select isp debug mode 1/0\r\n");
		return FALSE;
	}
	sscanf(pargv[0], "%d", (int *)&en);
	ctl_vpe_isp_evt_fp_dbg_mode(en);

	return TRUE;
}

BOOL nvt_ctl_vpe_api_set_dbg_level(unsigned char argc, char **pargv)
{
	UINT32 lv;

	if (argc == 0) {
		DBG_ERR("must select debug level\r\n");
		return FALSE;
	}
	sscanf(pargv[0], "%d", (int *)&lv);
	DBG_DUMP("set ctl_vpe dbg level to %d\r\n", lv);
	ctl_vpe_debug_level = lv;

	return TRUE;
}

BOOL nvt_ctl_vpe_api_save_yuv(unsigned char argc, char **pargv)
{
	CHAR dft_path[16] = "/tmp/";
	UINT32 save_input;
	UINT32 save_output;

	if (argc < 3) {
		DBG_ERR("illegel param, saveyuv (hdl_name) (input) (output) (file_path - optional)\r\n");
		return FALSE;
	}

	sscanf(pargv[1], "%d", (int *)&save_input);
	sscanf(pargv[2], "%x", (int *)&save_output);
	if (argc > 3) {
		DBG_DUMP("vpe save yuv cfg, %s %s %d 0x%x\r\n",
			pargv[0], pargv[3], (int)save_input, (unsigned int)save_output);
		ctl_vpe_save_yuv_cfg(pargv[0], pargv[3], save_input, save_output);
	} else {
		DBG_DUMP("vpe save yuv cfg, %s %s %d 0x%x\r\n",
			pargv[0], dft_path, (int)save_input, (unsigned int)save_output);
		ctl_vpe_save_yuv_cfg(pargv[0], dft_path, save_input, save_output);
	}

	return TRUE;
}

BOOL nvt_ctl_vpe_api_eng_hang_cfg(unsigned char argc, char **pargv)
{
	CTL_VPE_DBG_ENG_HANG cfg = {0};

	if (argc < 2) {
		DBG_ERR("illegel param, eng_hang cfg (dump_info) (dump_file) (timeout_ms - optional)\r\n");
		return FALSE;
	}

	sscanf(pargv[0], "%d", (int *)&cfg.dump_info);
	sscanf(pargv[1], "%d", (int *)&cfg.dump_file);
	if (argc > 2) {
		sscanf(pargv[2], "%d", (int *)&cfg.timeout_ms);
	}

	DBG_DUMP("vpe eng_hang debug cfg: dump_info(%d), dump_file(%d), timeout_ms(%d)\r\n",
				(int)cfg.dump_info, (int)cfg.dump_file, (int)cfg.timeout_ms);
	ctl_vpe_dbg_eng_hang_cfg(&cfg);

	return TRUE;
}

#if CTL_VPE_TEST_CMD_ENABLE
UINT32 g_kdrv_vpe_job_cb_num = 0;
UINT32 nvt_ctl_vpe_test_nvtmpp_wrapper(UINT32 do_init, UINT32 op, UINT32 data)
{
	#define YUV_420_BUFSIZE(w, h) ((w * h * 3) / 2)
	static UINT32 is_init = 0;
	NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('v', 'p', 'e', 't', 'e', 's', 't', 0);
	NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
	NVTMPP_DDR     ddr = NVTMPP_DDR_1;
	NVTMPP_VB_BLK  blk;
	NVTMPP_VB_CONF_S st_conf;

	if (do_init) {
		if (!is_init) {
			memset(&st_conf, 0, sizeof(NVTMPP_VB_CONF_S));
			st_conf.max_pool_cnt = 64;
		    st_conf.common_pool[0].blk_size = YUV_420_BUFSIZE(3840, 3840) + 1024;
			st_conf.common_pool[0].blk_cnt = 4;
			st_conf.common_pool[0].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[0].type = POOL_TYPE_COMMON;
			st_conf.common_pool[1].blk_size = YUV_420_BUFSIZE(2880, 2880) + 1024;
			st_conf.common_pool[1].blk_cnt = 4;
			st_conf.common_pool[1].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[1].type = POOL_TYPE_COMMON;
			st_conf.common_pool[2].blk_size = YUV_420_BUFSIZE(1920, 1080) + 1024;
			st_conf.common_pool[2].blk_cnt = 8;
			st_conf.common_pool[2].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[2].type = POOL_TYPE_COMMON;
			st_conf.common_pool[3].blk_size = YUV_420_BUFSIZE(1280, 720) + 1024;
			st_conf.common_pool[3].blk_cnt = 8;
			st_conf.common_pool[3].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[3].type = POOL_TYPE_COMMON;
			st_conf.common_pool[4].blk_size = YUV_420_BUFSIZE(640, 480) + 1024;
			st_conf.common_pool[4].blk_cnt = 8;
			st_conf.common_pool[4].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[4].type = POOL_TYPE_COMMON;
			st_conf.common_pool[5].blk_size = 4096;
			st_conf.common_pool[5].blk_cnt = 32;
			st_conf.common_pool[5].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[5].type = POOL_TYPE_COMMON;

			nvtmpp_vb_set_conf(&st_conf);
			nvtmpp_vb_init();
			nvtmpp_vb_add_module(module);

			is_init = 1;
		} else if (is_init) {
			nvtmpp_vb_exit();
			is_init = 0;
		}
	} else {
		if (op == CTL_VPE_BUF_NEW) {
			blk = nvtmpp_vb_get_block(module, pool, data, ddr);
			return (UINT32)nvtmpp_vb_blk2va(blk);
		}

		if (op == CTL_VPE_BUF_PUSH || op == CTL_VPE_BUF_UNLOCK) {
			blk = nvtmpp_vb_va2blk(data);
			nvtmpp_vb_unlock_block(module, blk);
		}

		if (op == CTL_VPE_BUF_LOCK) {
			blk = nvtmpp_vb_va2blk(data);
			nvtmpp_vb_lock_block(module, blk);
		}

	}

	return 0;
}

INT32 nvt_ctl_vpe_test_inbuf_cb(UINT32 event, void *p_in, void *p_out)
{
	CTL_VPE_EVT *p_evt;
	VDO_FRAME *p_frm;
	UINT64 ts_diff;

	p_evt = (CTL_VPE_EVT *)p_in;
	if (event == CTL_VPE_BUF_UNLOCK) {
		p_frm = (VDO_FRAME *)p_evt->data_addr;
		ts_diff = hwclock_get_longcounter() - p_frm->timestamp;

		nvt_ctl_vpe_test_nvtmpp_wrapper(0, event, p_evt->data_addr);
		g_kdrv_vpe_job_cb_num++;
		//DBG_DUMP("[INBUF] unl, buf_addr 0x%.8x, ts_diff %lld, err %d\r\n",
		//	(unsigned int)p_evt->data_addr, ts_diff, p_evt->err_msg);
	}

	return 0;
}

INT32 nvt_ctl_vpe_test_wait_inbuf_cb(UINT32 num)
{
	UINT32 timeout_cnt;

	timeout_cnt = 1000;
	while (g_kdrv_vpe_job_cb_num < num) {
		vos_util_delay_ms(10);
		timeout_cnt--;
		if (timeout_cnt == 0) {
			DBG_ERR("timeout\r\n");
			break;
		}
	}

	return timeout_cnt;
}

INT32 nvt_ctl_vpe_test_save_result(UINT32 op, UINT32 config, CTL_VPE_OUT_BUF_INFO *p_buf)
{
	static CTL_VPE_OUT_BUF_INFO buf_info[CTL_VPE_OUT_PATH_ID_MAX] = {0};
	VOS_FILE fp;
	CHAR *out_dir = "//mnt//sd//exam_vpe//";
	CHAR out_filename[256];

	if (op < CTL_VPE_OUT_PATH_ID_MAX) {
		/* save file */
		p_buf = &buf_info[op];
		if (p_buf->buf_addr) {
			snprintf(out_filename, 256, "%svpe_out_p%d_%dx%d_frm%d.raw",
						out_dir, p_buf->pid, p_buf->vdo_frm.size.w, p_buf->vdo_frm.size.h, (int)p_buf->vdo_frm.count);
			fp = vos_file_open(out_filename, O_CREAT|O_WRONLY|O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
			    DBG_ERR("failed in file open:%s\n", out_filename);
				return -1;
			}

			/* flush buffer and write file */
			vos_cpu_dcache_sync(p_buf->buf_addr, p_buf->buf_size, VOS_DMA_FROM_DEVICE);
			vos_file_write(fp, (void *)p_buf->buf_addr, p_buf->buf_size);
			vos_file_close(fp);
			DBG_DUMP("save file %s done\r\n", out_filename);
		} else {
			DBG_ERR("pid %d, buf addr 0\r\n", op);
		}
	}

	/* keep buffer info */
	if (p_buf != NULL) {
		buf_info[p_buf->pid] = *p_buf;
	}

	return 0;
}

INT32 nvt_ctl_vpe_test_outbuf_cb(UINT32 event, void *p_in, void *p_out)
{
	CTL_VPE_OUT_BUF_INFO *p_buf;

	p_buf = (CTL_VPE_OUT_BUF_INFO *)p_in;
	if (event == CTL_VPE_BUF_NEW) {
		p_buf->buf_addr = nvt_ctl_vpe_test_nvtmpp_wrapper(0, event, p_buf->buf_size);
		//DBG_DUMP("[OUTBUF] new, want size 0x%.8x, buf_addr 0x%.8x\r\n", p_buf->buf_size, (unsigned int)p_buf->buf_addr);

		// reset mem for debug
		#if 0
		if (p_buf->buf_addr) {
			memset((void *)p_buf->buf_addr, 0x5a, p_buf->buf_size);
			vos_cpu_dcache_sync(p_buf->buf_addr, p_buf->buf_size, VOS_DMA_TO_DEVICE);
		} else {
			DBG_ERR("new buffer failed, pid %d, want_size 0x%.8x\r\n",
						p_buf->pid, (unsigned int)p_buf->buf_size);
		}
		#endif
	}

	if (event == CTL_VPE_BUF_PUSH) {
		nvt_ctl_vpe_test_save_result(0xFF, 0, p_buf);
		nvt_ctl_vpe_test_nvtmpp_wrapper(0, event, p_buf->buf_addr);
		//DBG_DUMP("[OUTBUF] push, buf_addr 0x%.8x, pid %d, err %d\r\n",
		//	(unsigned int)p_buf->buf_addr, (int)p_buf->pid, (int)p_buf->err_msg);

		// test flow to sndevt in outbuf callback
		#if 0
		if (p_buf->pid == 0) {
			char *pargv[2] = {
				"sndevt", "1"
			};
			nvt_ctl_vpe_api_test_cmd(2, pargv);
		}
		#endif
	}

	if (event == CTL_VPE_BUF_UNLOCK) {
		nvt_ctl_vpe_test_nvtmpp_wrapper(0, event, p_buf->buf_addr);
		//DBG_DUMP("[OUTBUF] unlock, buf_addr 0x%.8x, pid %d, err %d\r\n",
		//	(unsigned int)p_buf->buf_addr, (int)p_buf->pid, (int)p_buf->err_msg);
	}

	return 0;
}

VDO_FRAME *nvt_ctl_vpe_test_get_in_vdofrm(UINT32 is_exit)
{
	static CHAR *in_frm_path = "//mnt//sd//exam_vpe//in.raw";
	static VDO_FRAME test_frm = {0};
	static UINT32 cnt = 0;
	UINT32 buf_size;
	UINT32 vdofrm_buffer;
	VDO_FRAME *p_frm;

	if (test_frm.addr[0] == 0) {
		/* read image from sd card */
		test_frm.size.w = 2880;
		test_frm.size.h = 2880;
		test_frm.loff[0] = 2880;
		test_frm.pxlfmt = VDO_PXLFMT_YUV420;

		buf_size = (test_frm.size.w * test_frm.size.h * 3) / 2;
		test_frm.addr[0] = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, buf_size);
		if (test_frm.addr[0]) {
			VOS_FILE fp;

			fp = vos_file_open(in_frm_path, O_RDONLY, 0);
			if (fp == (VOS_FILE)(-1)) {
			    DBG_ERR("failed in file open:%s\n", in_frm_path);
				return NULL;
			}
			vos_file_read(fp, (void *)test_frm.addr[0], buf_size);
			vos_file_close(fp);

			vos_cpu_dcache_sync(test_frm.addr[0], buf_size, VOS_DMA_TO_DEVICE);
		}
	}

	if (is_exit) {
		if (test_frm.addr[0]) {
			vdofrm_buffer = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, test_frm.addr[0]);
			test_frm.addr[0] = 0;
		}
	} else {
		vdofrm_buffer = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, sizeof(VDO_FRAME));
		if (vdofrm_buffer) {
			p_frm = (VDO_FRAME *)vdofrm_buffer;
			*p_frm = test_frm;
			p_frm->count = cnt++;
			p_frm->timestamp = hwclock_get_longcounter();
			//DBG_DUMP("[INBUF] new, buf_addr 0x%.8x\r\n", (unsigned int)vdofrm_buffer);

			return p_frm;
		}
	}

	return NULL;
}

static CTL_VPE_ISP_DCE_CTL g_isp_dce_ctl = {
	.enable = 0,
	.lsb_rand = 0,
	.dce_mode = 0,
};
static CTL_VPE_ISP_DCE_2DLUT_PARAM g_isp_2dlut = {
	.xofs_i = 0,
	.xofs_f = 0,
	.yofs_i = 0,
	.yofs_f = 0,
	.lut_sz = 0,
};
static CTL_VPE_ISP_DCE_GDC_PARAM g_isp_gdc = {
	.fovbound = 0,
	.boundy = 0,
	.boundu = 0,
	.boundv = 0,
	.xdist_a1 = 4095,
	.ydist_a1 = 4095,
	.fovgain = 1024,
	.geo_lut = {
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535},
	.cent_x_ratio = 500,
	.cent_y_ratio = 500,
	.cent_ratio_base = 1000
};
static CTL_VPE_ISP_SHARPEN_PARAM g_isp_sharpen = {
	.enable = 0,
	.edge_weight_src_sel = FALSE,
	.edge_weight_th = 2,
	.edge_weight_gain = 175,
	.noise_level = 15,
	.noise_curve = {
		0, 38, 46, 51, 54, 57, 59, 61, 62, 62,
		62, 62, 62, 62, 62, 62, 62},
	.blend_inv_gamma = 128,
	.edge_sharp_str1 = 40,
	.edge_sharp_str2 = 40,
	.flat_sharp_str = 20,
	.coring_th = 0,
	.bright_halo_clip = 128,
	.dark_halo_clip = 128,
	.sharpen_out_sel = 0
};
static CTL_VPE_ISP_YUV_CVT_PARAM g_isp_yuvcvt = {
	.cvt_sel = {CTL_VPE_ISP_YUV_CVT_NONE, CTL_VPE_ISP_YUV_CVT_TV, CTL_VPE_ISP_YUV_CVT_PC, CTL_VPE_ISP_YUV_CVT_NONE}
};

static CTL_VPE_ISP_DCTG_PARAM g_isp_dctg = {
	.enable = 0,
	.mount_type = 0,
	.lut2d_sz = 0,
	.lens_r = 0,
	.lens_x_st = 0,
	.lens_y_st = 0,
	.theta_st = 0,
	.theta_ed = 0,
	.phi_st = 0,
	.phi_ed = 0,
	.rot_z = 0,
	.rot_y = 0
};

INT32 nvt_ctl_vpe_test_isp_cb(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	static UINT32 is_init = 0;
	CTL_VPE_ISP_IQ_ALL iq_all = {NULL};
	INT32 rt;

	if (is_init == 0) {
		/* load 2dlut from file */
		CHAR *lut_path = "//mnt//sd//exam_vpe//thermalchanbers_2880x2880_2dlut.bin";
		UINT32 lut_size = 17680; /* 68*65 table */
		//CHAR *lut_path = "//mnt//sd//exam_vpe//thermalchamber_3840x3840_257_vpe.bin";
		//UINT32 lut_size = 267280; /* 260*257 table */
		VOS_FILE fp;

		fp = vos_file_open(lut_path, O_RDONLY, 0);
		if (fp == (VOS_FILE)(-1)) {
		    DBG_ERR("failed in file open:%s\n", lut_path);
			return -1;
		}
		vos_file_read(fp, (void *)&g_isp_2dlut.lut[0], lut_size);
		vos_file_close(fp);
		is_init = 1;

		/* init 2dlut param */
		g_isp_2dlut.lut_sz = CTL_VPE_ISP_2DLUT_SZ_65X65;
	}

	if (evt == ISP_EVENT_PARAM_RST || evt == ISP_EVENT_VPE_CFG_IMM) {
		iq_all.p_dce_ctl = &g_isp_dce_ctl;
		iq_all.p_dce_gdc_param = &g_isp_gdc;
		iq_all.p_sharpen_param = &g_isp_sharpen;
		iq_all.p_yuv_cvt_param = &g_isp_yuvcvt;
		if (iq_all.p_dce_ctl->dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY || iq_all.p_dce_ctl->dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) {
			iq_all.p_dce_2dlut_param = &g_isp_2dlut;
			if (iq_all.p_dce_ctl->dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) {
				iq_all.p_dctg_param = &g_isp_dctg;
			}
		}

		rt = ctl_vpe_isp_set(id, CTL_VPE_ISP_ITEM_IQ_PARAM, (void *)&iq_all);
		if (rt != E_OK) {
			DBG_DUMP("vpe isp set failed %d, evt 0x%.8x\r\n", rt, (unsigned int)evt);
		}
	}

	return 0;
}

INT32 nvt_ctl_vpe_test_default_iocfg(UINT32 vpe_hdl)
{
	CTL_VPE_REG_CB_INFO cb_info = {0};
	CTL_VPE_IN_CROP in_crop = {0};
	UINT32 i;
	INT32 rt;

	/* register callback */
	cb_info.cbevt = CTL_VPE_CBEVT_IN_BUF;
	cb_info.fp = nvt_ctl_vpe_test_inbuf_cb;
	rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_REG_CB_IMM, (void *)&cb_info);
	if (rt != CTL_VPE_E_OK) {
		return FALSE;
	}

	cb_info.cbevt = CTL_VPE_CBEVT_OUT_BUF;
	cb_info.fp = nvt_ctl_vpe_test_outbuf_cb;
	rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_REG_CB_IMM, (void *)&cb_info);
	if (rt != CTL_VPE_E_OK) {
		return FALSE;
	}

	/* config vpe */
	in_crop.mode = CTL_VPE_IN_CROP_NONE;
	rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_IN_CROP, (void *)&in_crop);
	if (rt != CTL_VPE_E_OK) {
		return FALSE;
	}

	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		CTL_VPE_OUT_PATH out_path = {0};

		out_path.pid = i;
		if (i == 0) {
			out_path.enable = ENABLE;
			out_path.bg_size.w = 3840;
			out_path.bg_size.h = 2160;
			out_path.fmt = VDO_PXLFMT_YUV420;
		} else if (i == 1) {
			out_path.enable = ENABLE;
			out_path.bg_size.w = 3840;
			out_path.bg_size.h = 2160;
			out_path.fmt = VDO_PXLFMT_YUV420_NVX2;
		} else {
			out_path.enable = ENABLE;
			out_path.bg_size.w = 1280;
			out_path.bg_size.h = 720;
			out_path.fmt = VDO_PXLFMT_YUV420;
		}

		out_path.scl_size = out_path.bg_size;
		out_path.pre_scl_crop.x = 0;
		out_path.pre_scl_crop.y = 0;
		out_path.pre_scl_crop.w = in_crop.crp_window.w;
		out_path.pre_scl_crop.h = in_crop.crp_window.h;
		out_path.out_window.x = 0;
		out_path.out_window.y = 0;
		out_path.out_window.w = out_path.scl_size.w;
		out_path.out_window.h = out_path.scl_size.h;
		out_path.dst_pos.x = 0;
		out_path.dst_pos.y = 0;
		out_path.hole_region.x = 0;
		out_path.hole_region.y = 0;
		out_path.hole_region.w = 0;
		out_path.hole_region.h = 0;
		rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		if (rt != CTL_VPE_E_OK) {
			return FALSE;
		}
	}

	rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_APPLY, NULL);
	if (rt != CTL_VPE_E_OK) {
		return FALSE;
	}

	return rt;
}

BOOL nvt_ctl_vpe_api_test_cfg(unsigned char argc, char **pargv)
{
	UINT32 op;

	sscanf(pargv[0], "%d", (int *)&op);

	/* save result config */
	if (op == 0) {
		UINT32 path;
		UINT32 pid;

		sscanf(pargv[1], "%x", (int *)&path);
		DBG_DUMP("save_result path_bit %x\r\n", path);
		for (pid = 0; pid < CTL_VPE_OUT_PATH_ID_MAX; pid++) {
			if (path & (1 << pid)) {
				nvt_ctl_vpe_test_save_result(pid, 0, NULL);
			}
		}
		return TRUE;
	}

	/* isp dce_en, dce_mode */
	if (op == 1) {
		sscanf(pargv[1], "%d", (int *)&g_isp_dce_ctl.enable);
		sscanf(pargv[2], "%d", (int *)&g_isp_dce_ctl.dce_mode);
		DBG_DUMP("dce enable %d, mode %d\r\n", g_isp_dce_ctl.enable, g_isp_dce_ctl.dce_mode);
		return TRUE;
	}

	/* isp sharpen en */
	if (op == 2) {
		sscanf(pargv[1], "%d", (int *)&g_isp_sharpen.enable);
		DBG_DUMP("sharpen enable %d\r\n", g_isp_sharpen.enable);
		return TRUE;
	}

	/* isp yuv cvt config */
	if (op == 3) {
		UINT32 path;
		UINT32 sel;

		sscanf(pargv[1], "%d", (int *)&path);
		sscanf(pargv[2], "%d", (int *)&sel);
		g_isp_yuvcvt.cvt_sel[path] = sel;
		DBG_DUMP("yuvcvt path %d, sel %d\r\n", path, sel);
		return TRUE;
	}

	return TRUE;
}

BOOL nvt_ctl_vpe_api_test_d2d(unsigned char argc, char **pargv)
{
	CTL_VPE_CTX_BUF_CFG ctx_buf_cfg;
	CTL_VPE_PRIVATE_BUF pri_buf;
	CTL_VPE_EVT evt;
	VDO_FRAME *p_in_frm;
	void *p_ctx_buffer;
	UINT32 isp_id;
	UINT32 query_buf_size;
	UINT32 vpe_hdl;
	UINT32 job_num;
	UINT32 i;
	INT32 rt;

	sscanf(pargv[0], "%d", (int *)&job_num);

	DBG_DUMP("test start, job_num %d\r\n", job_num);

	/* stage 1 */
	g_kdrv_vpe_job_cb_num = 0;
	nvt_ctl_vpe_test_nvtmpp_wrapper(1, 0, 0);

	/* register isp callback, dummy callback to read 2dlut table */
	nvt_ctl_vpe_test_isp_cb(0, 0, 0, NULL);
	ctl_vpe_isp_evt_fp_reg("test_vpe_isp", nvt_ctl_vpe_test_isp_cb, (ISP_EVENT_VPE_CFG_IMM | ISP_EVENT_PARAM_RST), CTL_VPE_ISP_CB_MSG_NONE);

	/* init and open */
	ctx_buf_cfg.handle_num = 2;
	ctx_buf_cfg.queue_num = 8;
	query_buf_size = ctl_vpe_query(ctx_buf_cfg);
	if (query_buf_size) {
		p_ctx_buffer = (void *)nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, query_buf_size);
		if (p_ctx_buffer == NULL) {
			DBG_ERR("alloc ctx buffer failed\r\n");
			return FALSE;
		}

		DBG_DUMP("buffer init, addr 0x%.8x, size 0x%.8x\r\n", (unsigned int)p_ctx_buffer, (unsigned int)query_buf_size);
	}

	rt = ctl_vpe_init(ctx_buf_cfg, (UINT32)p_ctx_buffer, query_buf_size);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("init failed\r\n");
		return FALSE;
	}

	vpe_hdl = ctl_vpe_open("test_vpe");
	if (vpe_hdl == 0) {
		DBG_ERR("vpe open failed\r\n");
		return FALSE;
	}
	DBG_DUMP("vpe handle open 0x%.8x\r\n", (unsigned int)vpe_hdl);

	/* config isp id */
	isp_id = 10;
	rt = ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_ALGID_IMM, (void *)&isp_id);

	/* allocate private buffer */
	pri_buf.buf_size = 0;
	ctl_vpe_get(vpe_hdl, CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
	DBG_DUMP("vpe private buffer 0x%.8x\r\n", (unsigned int)pri_buf.buf_size);
	if (pri_buf.buf_size > 0) {
		pri_buf.buf_addr = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, pri_buf.buf_size);
		ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
	}

	/* default config */
	nvt_ctl_vpe_test_default_iocfg(vpe_hdl);

	/* snd event, test with dummy vdoframe */
	for (i = 0; i < job_num; i++) {
		p_in_frm = nvt_ctl_vpe_test_get_in_vdofrm(0);
		if (p_in_frm == NULL) {
			DBG_ERR("get test in frm failed\r\n");
			vos_util_delay_ms(100);
			job_num--;
			continue;
		}

		evt.buf_id = (0x5a5a0000 | i);
		evt.data_addr = (UINT32)p_in_frm;
		rt = ctl_vpe_ioctl(vpe_hdl, CTL_VPE_IOCTL_SNDEVT, (void *)&evt);
		if (rt != CTL_VPE_E_OK) {
			DBG_ERR("vpe sndevt failed\r\n");
			job_num--;
			nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, (UINT32)p_in_frm);
		}
		vos_util_delay_ms(100);
	}

	if (nvt_ctl_vpe_test_wait_inbuf_cb(job_num) == 0) {
		return FALSE;
	}
		ctl_vpe_dump_all(ctl_vpe_int_printf);
	//debug_dumpmem(0xf0cd0000, 0x1200);

	/* stage 2 */
	/* flush */
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		CTL_VPE_OUT_PATH out_path;
		CTL_VPE_FLUSH_CONFIG flush_cfg;

		out_path.pid = i;
		ctl_vpe_get(vpe_hdl, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);

		if (out_path.enable) {
			out_path.enable = DISABLE;
			ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
			ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_APPLY, NULL);
			flush_cfg.pid = i;
			ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_FLUSH, (void *)&flush_cfg);
		}
	}
	ctl_vpe_set(vpe_hdl, CTL_VPE_ITEM_FLUSH, NULL);

	rt = ctl_vpe_close(vpe_hdl);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("vpe close failed\r\n");
		return FALSE;
	}

	rt = ctl_vpe_uninit();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("vpe uninit failed\r\n");
		return FALSE;
	}

	/* unregister isp */
	ctl_vpe_isp_evt_fp_unreg("test_vpe_isp");

	/* release ctx buffer, in frm buffer and nvtmpp exit */
	if (pri_buf.buf_addr != 0) {
		nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, pri_buf.buf_addr);
	}
	nvt_ctl_vpe_test_get_in_vdofrm(1);
	nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, (UINT32)p_ctx_buffer);
	nvt_ctl_vpe_test_nvtmpp_wrapper(1, 0, 0);
	/* stage 2 close end */

	return TRUE;
}

THREAD_DECLARE(ctl_vpe_test_sndevt_tsk, p1)
{
	UINT32 hdl;
	INT32 rt;

	hdl = ((UINT32 *)p1)[0];

	while (!THREAD_SHOULD_STOP) {
		VDO_FRAME *p_in_frm;
		CTL_VPE_EVT evt;

		p_in_frm = nvt_ctl_vpe_test_get_in_vdofrm(0);
		if (p_in_frm == NULL) {
			DBG_ERR("get test in frm failed\r\n");
			return FALSE;
		}

		evt.buf_id = 0x5a5a0000;
		evt.data_addr = (UINT32)p_in_frm;
		rt = ctl_vpe_ioctl(hdl, CTL_VPE_IOCTL_SNDEVT, (void *)&evt);
		if (rt != CTL_VPE_E_OK) {
			nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, (UINT32)p_in_frm);
		}

		vos_util_delay_ms(33);
	}

	THREAD_RETURN(0);
}

THREAD_DECLARE(ctl_vpe_test_tsk, p1)
{
	CTL_VPE_OUT_PATH out_path;
	UINT32 i;
	UINT32 hdl;

	hdl = ((UINT32 *)p1)[0];

	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		out_path.pid = i;
		out_path.enable = 0;
		ctl_vpe_set(hdl, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
	}
	ctl_vpe_set(hdl, CTL_VPE_ITEM_APPLY, NULL);
	ctl_vpe_set(hdl, CTL_VPE_ITEM_FLUSH, NULL);
	CHKPNT;
	ctl_vpe_close(hdl);
	CHKPNT;

	THREAD_RETURN(0);
}

BOOL nvt_ctl_vpe_api_test_cmd(unsigned char argc, char **pargv)
{
	static UINT32 ctx_buffer;
	static UINT32 vpe_hdl[8];
	static CTL_VPE_PRIVATE_BUF pri_buf[8];
	static THREAD_HANDLE sndevt_tsk_hdl[8];
	INT32 rt;

	if (strcmp(pargv[0], "init") == 0) {
		CTL_VPE_CTX_BUF_CFG ctx_buf_cfg;
		UINT32 query_buf_size;

		/* nvtmpp init */
		nvt_ctl_vpe_test_nvtmpp_wrapper(1, 0, 0);

		/* register isp callback, dummy callback to read 2dlut table */
		nvt_ctl_vpe_test_isp_cb(0, 0, 0, NULL);
		ctl_vpe_isp_evt_fp_reg("test_vpe_isp", nvt_ctl_vpe_test_isp_cb, (ISP_EVENT_VPE_CFG_IMM | ISP_EVENT_PARAM_RST), CTL_VPE_ISP_CB_MSG_NONE);

		/* ctl_vpe init */
		ctx_buf_cfg.handle_num = 8;
		ctx_buf_cfg.queue_num = 8;
		query_buf_size = ctl_vpe_query(ctx_buf_cfg);
		if (query_buf_size) {
			ctx_buffer = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, query_buf_size);
			if (ctx_buffer == 0) {
				DBG_ERR("alloc ctx buffer failed\r\n");
				return FALSE;
			}

			DBG_DUMP("buffer init, addr 0x%.8x, size 0x%.8x\r\n", (unsigned int)ctx_buffer, (unsigned int)query_buf_size);
		}

		rt = ctl_vpe_init(ctx_buf_cfg, ctx_buffer, query_buf_size);
		if (rt != CTL_VPE_E_OK) {
			DBG_ERR("init failed\r\n");
			return FALSE;
		}
	} else if (strcmp(pargv[0], "exit") == 0) {
		/* exit */
		ctl_vpe_uninit();
		ctl_vpe_isp_evt_fp_unreg("test_vpe_isp");

		/* release buffer */
		nvt_ctl_vpe_test_get_in_vdofrm(1);
		nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, ctx_buffer);
		nvt_ctl_vpe_test_nvtmpp_wrapper(1, 0, 0);
	} else if (strcmp(pargv[0], "open") == 0) {
		CHAR name[16];
		UINT32 idx;

		sscanf(pargv[1], "%d", (int *)&idx);
		snprintf(name, 16, "test_vpe_%d", idx);
		vpe_hdl[idx] = ctl_vpe_open(name);
		if (vpe_hdl[idx] == 0) {
			DBG_ERR("vpe open failed\r\n");
			return FALSE;
		}
		DBG_DUMP("vpe handle open 0x%.8x\r\n", (unsigned int)vpe_hdl[idx]);

		/* allocate private buffer
			set isp id first for 2dlut buffer
		*/
		rt = ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_ALGID_IMM, (void *)&idx);

		pri_buf[idx].buf_size = 0;
		ctl_vpe_get(vpe_hdl[idx], CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf[idx]);
		DBG_DUMP("vpe private buffer 0x%.8x\r\n", (unsigned int)pri_buf[idx].buf_size);
		if (pri_buf[idx].buf_size > 0) {
			pri_buf[idx].buf_addr = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, pri_buf[idx].buf_size);
			ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf[idx]);
		}

		/* default config */
		nvt_ctl_vpe_test_default_iocfg(vpe_hdl[idx]);
	} else if (strcmp(pargv[0], "close") == 0) {
		CTL_VPE_OUT_PATH out_path;
		UINT32 idx;
		UINT32 i;

		sscanf(pargv[1], "%d", (int *)&idx);

		for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
			out_path.pid = i;
			out_path.enable = 0;
			ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		}
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_APPLY, NULL);
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_FLUSH, NULL);
		ctl_vpe_close(vpe_hdl[idx]);
		if (pri_buf[idx].buf_addr != 0) {
			nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, pri_buf[idx].buf_addr);
		}
	} else if (strcmp(pargv[0], "sndevt") == 0) {
		VDO_FRAME *p_in_frm;
		CTL_VPE_EVT evt;
		UINT32 idx;

		sscanf(pargv[1], "%d", (int *)&idx);

		p_in_frm = nvt_ctl_vpe_test_get_in_vdofrm(0);
		if (p_in_frm == NULL) {
			DBG_ERR("get test in frm failed\r\n");
			return FALSE;
		}

		evt.buf_id = 0x5a5a0000;
		evt.data_addr = (UINT32)p_in_frm;
		rt = ctl_vpe_ioctl(vpe_hdl[idx], CTL_VPE_IOCTL_SNDEVT, (void *)&evt);
		if (rt != CTL_VPE_E_OK) {
			nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, (UINT32)p_in_frm);
		}
	} else if (strcmp(pargv[0], "src_crop") == 0) {
		CTL_VPE_IN_CROP in_crop = {0};
		UINT32 idx;

		sscanf(pargv[1], "%d", (int *)&idx);
		sscanf(pargv[2], "%d", (int *)&in_crop.mode);
		if (in_crop.mode == CTL_VPE_IN_CROP_USER) {
			sscanf(pargv[3], "%d", (int *)&in_crop.crp_window.x);
			sscanf(pargv[4], "%d", (int *)&in_crop.crp_window.y);
			sscanf(pargv[5], "%d", (int *)&in_crop.crp_window.w);
			sscanf(pargv[6], "%d", (int *)&in_crop.crp_window.h);
		}
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_IN_CROP, (void *)&in_crop);
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_APPLY, NULL);

		DBG_DUMP("crop mode %d, window(%d %d %d %d)\r\n",
					in_crop.mode, in_crop.crp_window.x, in_crop.crp_window.y, in_crop.crp_window.w, in_crop.crp_window.h);
	} else if (strcmp(pargv[0], "sca_crop") == 0) {
		CTL_VPE_OUT_PATH out_path = {0};
		UINT32 idx;

		sscanf(pargv[1], "%d", (int *)&idx);
		sscanf(pargv[2], "%d", (int *)&out_path.pid);

		ctl_vpe_get(vpe_hdl[idx], CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		sscanf(pargv[3], "%d", (int *)&out_path.pre_scl_crop.x);
		sscanf(pargv[4], "%d", (int *)&out_path.pre_scl_crop.y);
		sscanf(pargv[5], "%d", (int *)&out_path.pre_scl_crop.w);
		sscanf(pargv[6], "%d", (int *)&out_path.pre_scl_crop.h);
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_APPLY, NULL);

		DBG_DUMP("path %d, sca_crop(%d %d %d %d)\r\n",
					out_path.pid, out_path.pre_scl_crop.x, out_path.pre_scl_crop.y, out_path.pre_scl_crop.w, out_path.pre_scl_crop.h);
	} else if (strcmp(pargv[0], "sca_size") == 0) {
		CTL_VPE_OUT_PATH out_path = {0};
		UINT32 idx;
		UINT32 w, h;

		sscanf(pargv[1], "%d", (int *)&idx);
		sscanf(pargv[2], "%d", (int *)&out_path.pid);
		sscanf(pargv[3], "%d", (int *)&w);
		sscanf(pargv[4], "%d", (int *)&h);

		ctl_vpe_get(vpe_hdl[idx], CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		out_path.bg_size.w = w;
		out_path.bg_size.h = h;
		out_path.out_window.w = w;
		out_path.out_window.h = h;
		out_path.scl_size.w = w;
		out_path.scl_size.h = h;
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_APPLY, NULL);

		DBG_DUMP("path %d. sca_size(%d %d)\r\n",
					out_path.pid, out_path.scl_size.w, out_path.scl_size.h);
	} else if (strcmp(pargv[0], "flush_test") == 0) {
		VDO_FRAME *p_in_frm;
		CTL_VPE_EVT evt;
		UINT32 idx;
		UINT32 i;

		sscanf(pargv[1], "%d", (int *)&idx);

		/*
			1. send multiple event
			2. immediately close to trigger flush flow
			3. re-open back to initial status
		*/
		for (i = 0; i < 5; i++) {
			p_in_frm = nvt_ctl_vpe_test_get_in_vdofrm(0);
			if (p_in_frm == NULL) {
				DBG_ERR("get test in frm failed\r\n");
				return FALSE;
			}

			evt.buf_id = 0x5a5a0000;
			evt.data_addr = (UINT32)p_in_frm;
			rt = ctl_vpe_ioctl(vpe_hdl[idx], CTL_VPE_IOCTL_SNDEVT, (void *)&evt);
			if (rt != CTL_VPE_E_OK) {
				nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_UNLOCK, (UINT32)p_in_frm);
			}
		}
		ctl_vpe_close(vpe_hdl[idx]);

		vpe_hdl[idx] = ctl_vpe_open("test_vpe");
		if (vpe_hdl[idx] == 0) {
			DBG_ERR("vpe open failed\r\n");
			return FALSE;
		}
		DBG_DUMP("vpe handle open 0x%.8x\r\n", (unsigned int)vpe_hdl[idx]);

		/* allocate private buffer
			set isp id first for 2dlut buffer
		*/
		rt = ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_ALGID_IMM, (void *)&idx);

		pri_buf[idx].buf_size = 0;
		ctl_vpe_get(vpe_hdl[idx], CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf[idx]);
		DBG_DUMP("vpe private buffer 0x%.8x\r\n", (unsigned int)pri_buf[idx].buf_size);
		if (pri_buf[idx].buf_size > 0) {
			pri_buf[idx].buf_addr = nvt_ctl_vpe_test_nvtmpp_wrapper(0, CTL_VPE_BUF_NEW, pri_buf[idx].buf_size);
			ctl_vpe_set(vpe_hdl[idx], CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf[idx]);
		}

		/* default config */
		nvt_ctl_vpe_test_default_iocfg(vpe_hdl[idx]);
	} else if (strcmp(pargv[0], "sndevt_thread") == 0) {
		UINT32 idx;
		UINT32 op;

		sscanf(pargv[1], "%d", (int *)&idx);
		sscanf(pargv[2], "%d", (int *)&op);

		if (op == 0) {
			THREAD_CREATE(sndevt_tsk_hdl[idx], ctl_vpe_test_sndevt_tsk, (void *)&vpe_hdl[idx], "exam_ctl_vpe_sndevt_task");
			THREAD_SET_PRIORITY(sndevt_tsk_hdl[idx], 4);
			THREAD_RESUME(sndevt_tsk_hdl[idx]);
		} else {
			THREAD_DESTROY(sndevt_tsk_hdl[idx]);
		}
	} else if (strcmp(pargv[0], "close_thread") == 0) {
		THREAD_HANDLE tmp_tsk_hdl;
		UINT32 idx;

		sscanf(pargv[1], "%d", (int *)&idx);
		THREAD_CREATE(tmp_tsk_hdl, ctl_vpe_test_tsk, (void *)&vpe_hdl[idx], "exam_ctl_vpe_close_task");
		THREAD_SET_PRIORITY(tmp_tsk_hdl, 4);
		THREAD_RESUME(tmp_tsk_hdl);
	} else {
		DBG_ERR("unknown test cmd\r\n");
	}

	return TRUE;
}
#endif
#endif	/* CTL_VPE_MODULE_ENABLE */
