#include "kwrap/cpu.h"
#include <kwrap/type.h>
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kwrap/util.h"
#include "kwrap/file.h"
#include "comm/ddr_arb.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "ipp_debug_int.h"
#include "ipp_event_int.h"
#include "ipp_event_id_int.h"
#include "ctl_ipp_api.h"
#include "ctl_ipp_drv.h"
#include "ctl_ipp_dbg.h"
#include "ctl_ipp_util_int.h"
#include "ctl_ipp_int.h"
#include "ctl_ipp_flow_task_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_id_int.h"
#include "kdf_ipp_int.h"
#include "kdf_ipp_id_int.h"
#include "kflow.h"
#include "kflow_videocapture/ctl_sie.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#define EXAM_CTL_IPP_ENABLE (0)

static SXCMD_BEGIN(ctl_ipp_cmd_tbl, "ctl_ipp")
SXCMD_ITEM("dumpinfo",			nvt_ctl_ipp_api_dump_hdl_all, 		"dump all handle")
SXCMD_ITEM("dumpt",				nvt_ctl_ipp_api_dump_ts, 			"dump ipp time")
SXCMD_ITEM("dbglv %",			nvt_ctl_ipp_api_dbg_level, 			"set dbg level(lv)")
SXCMD_ITEM("dumpdtsi %",		nvt_ctl_ipp_api_dump_dtsi, 			"dump ipp config to dtsi fmt for fastboot(file_name)")
SXCMD_ITEM("dbgmask %",			nvt_ctl_ipp_api_dbg_primask, 		"set dbg mask(en)")
SXCMD_ITEM("isp_debug %",		nvt_ctl_ipp_api_set_isp_dbg_mode, 	"set isp debug mode(on/off)")
SXCMD_ITEM("saveraw %",			nvt_ctl_ipp_api_saveraw, 			"save yuv(hdl_name/hdl_id)")
SXCMD_ITEM("savemem %",			nvt_ctl_ipp_api_savemem, 			"save mem(addr/[width/height/[fmt] or size])")
SXCMD_ITEM("fastboot %",		nvt_ctl_ipp_api_fastboot_debug, 	"fastboot debug(item)")
SXCMD_ITEM("mem %",				nvt_ctl_ipp_api_mem_rw, 			"mem (r/w, addr, len/val)")
SXCMD_ITEM("dmawp %",			nvt_ctl_ipp_api_dbg_dma_wp, 		"set dbg dma write protect(en, type, level)")
SXCMD_ITEM("dirtsk %",			nvt_ctl_ipp_api_dir_tsk_mode, 		"set direct task mode enable (en)")
SXCMD_ITEM("-------------",		NULL, 								"------------- backward compatible -------------")
SXCMD_ITEM("dump_t",			nvt_ctl_ipp_api_dump_ts, 			"same as dumpt")
SXCMD_ITEM("dump_hdl_all",		nvt_ctl_ipp_api_dump_hdl_all, 		"same as dumpinfo")
SXCMD_ITEM("dump_fastboot %",	nvt_ctl_ipp_api_dump_dtsi, 			"same as dumpdtsi")
SXCMD_ITEM("dbg_level %",		nvt_ctl_ipp_api_dbg_level, 			"same as dbglv")
SXCMD_ITEM("dbg_mask %",		nvt_ctl_ipp_api_dbg_primask, 		"same as dbgmask")
SXCMD_ITEM("saveyuv %",			nvt_ctl_ipp_api_saveyuv, 			"legacy save yuv(hdl_name, file_path, cnt)")
#if EXAM_CTL_IPP_ENABLE
SXCMD_ITEM("dump_isp_item %",	nvt_ctl_ipp_api_dump_isp_item, 		"dump isp item(isp_id, isp_item)")
SXCMD_ITEM("setoutpath %",		nvt_ctl_ipp_api_set_out_path, 		"set out path(hdl, path, en, w, h, fotmat)")
SXCMD_ITEM("setincrop %",		nvt_ctl_ipp_api_set_in_crop, 		"set in crop(hdl, mode, x, y, w, h)")
SXCMD_ITEM("setfuncen %",		nvt_ctl_ipp_api_set_func_en, 		"set func en(hdl, funcen)")
SXCMD_ITEM("setispid %",		nvt_ctl_ipp_api_set_ispid, 			"set isp id(hdl, id)")
SXCMD_ITEM("setflip %",			nvt_ctl_ipp_api_set_flip, 			"set flip(hdl, flip_type)")
SXCMD_ITEM("setbp %",			nvt_ctl_ipp_api_set_bp, 			"set bp(hdl, bp_line)")
SXCMD_ITEM("setstrprule %",		nvt_ctl_ipp_api_set_strp_rule, 		"set strp rule(hdl, rule)")
SXCMD_ITEM("setorder %",		nvt_ctl_ipp_api_set_path_order, 	"set path order(hdl, path, order)")
SXCMD_ITEM("setregion %",		nvt_ctl_ipp_api_set_path_region, 	"set path region(hdl, path, en, bg_w, bg_h, ofs_x, ofs_y)")
SXCMD_ITEM("eng_hang %",		nvt_ctl_ipp_api_set_eng_hang_handle,"set handle mode when engine hang")
SXCMD_ITEM("ctltest %",			nvt_ctl_ipp_api_ctrl_testcmd, 		"ctl ipp exam cmd")
#endif
SXCMD_END()

#if defined(__LINUX)
int ctl_ipp_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_ipp_cmd_tbl);
	UINT32 loop;
	int    ret = 0;

	if (argc < 1 || strncmp(argv[0], "?", 2) == 0 || strncmp(argv[0], "", 1) == 0) {
		for (loop = 1 ; loop <= cmd_num ; loop++) {
			DBG_DUMP("%15s : %s\r\n", ctl_ipp_cmd_tbl[loop].p_name, ctl_ipp_cmd_tbl[loop].p_desc);
		}
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], ctl_ipp_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			if (ctl_ipp_cmd_tbl[loop].p_func) {
				ret = ctl_ipp_cmd_tbl[loop].p_func(argc-1, &argv[1]);
			}
			return ret;
		}
	}
	if (loop > cmd_num) {
		CTL_IPP_DBG_ERR("Invalid CMD !!\r\n");
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(ctl_ipp, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_ipp_cmd_tbl);
	UINT32 loop;
	int    ret = 0;

	if (argc < 2 || strncmp(argv[1], "?", 2) == 0 || strncmp(argv[1], "", 1) == 0) {
		for (loop = 1 ; loop <= cmd_num ; loop++) {
			DBG_DUMP("%15s : %s\r\n", ctl_ipp_cmd_tbl[loop].p_name, ctl_ipp_cmd_tbl[loop].p_desc);
		}
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ctl_ipp_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			if (ctl_ipp_cmd_tbl[loop].p_func) {
				ret = ctl_ipp_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			}
			return ret;
		}
	}
	if (loop > cmd_num) {
		CTL_IPP_DBG_ERR("Invalid CMD !!\r\n");
		return -1;
	}
	return 0;
}
#endif


#if (EXAM_CTL_IPP_ENABLE)
typedef struct {
	CHAR    filename[128];
	UINT32  width;
	UINT32  height;
	UINT32  lofs;
	UINT32  fmt;
} EXAM_CTL_IPP_IMGFILE;

typedef struct frammap_buf_info {
	UINT32      phy_addr;   ///< physical address
	void        *va_addr;   ///< virtual address
	UINT32      size;	    ///< The size you want to allocate
	int         align;      ///< address alignment
	char        *name;      ///< max is NAME_SZ (20bytes)
} EXAM_CTL_IPP_BUF;

#define EXAM_IPP_BUF_NUM (5)
static EXAM_CTL_IPP_BUF in_raw_buf_info = {0};
static EXAM_CTL_IPP_BUF in_yuv_buf_info = {0};
static EXAM_CTL_IPP_BUF in_yout_buf_info = {0};
static EXAM_CTL_IPP_BUF eth_buf_info[2];
static EXAM_CTL_IPP_BUF eth_subsample_buf_info[2];
static UINT32 proc_end_cnt;
static UINT32 drop_cnt;

#define EXAM_CTRL_HDL_MAX_NUM (4)
#define EXAM_CTRL_CB_LOG (0)
#define EXAM_CTRL_USE_NVTMPP (1)

typedef enum {
	EXAM_CTRL_IPP_TASK_OP_SNDEVT		= 0x0001,
	EXAM_CTRL_IPP_TASK_OP_FLUSH			= 0x0002,
	EXAM_CTRL_IPP_TASK_OP_EXIT			= 0x0004,
} EXAM_CTRL_IPP_TASK_OP;

static UINT32 g_ctl_ipp_api_dbg_log;
static UINT32 g_ctl_ipp_load_img_flg;
static UINT32 g_ctl_ipp_clr_buf = 0;
static UINT32 g_ctl_ipp_hdl_list[EXAM_CTRL_HDL_MAX_NUM];
static UINT32 g_ctl_ipp_low_delay_path = CTL_IPP_OUT_PATH_ID_MAX;
static THREAD_HANDLE g_ctl_ipp_sndevt_tsk_id[EXAM_CTRL_HDL_MAX_NUM];
static UINT32 g_ctl_ipp_sndevt_tsk_op[EXAM_CTRL_HDL_MAX_NUM];
static UINT32 g_ctl_ipp_flow_sel[EXAM_CTRL_HDL_MAX_NUM] = {
	CTL_IPP_FLOW_RAW,
	CTL_IPP_FLOW_CCIR,
	CTL_IPP_FLOW_RAW,
	CTL_IPP_FLOW_CCIR,
};
#if defined(__LINUX)
static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_raw_input = {
	"//mnt//sd//EXAM//CTRL//RAW2YUV//SRC//IN.RAW",
	1920, 1080, 1920*12/8, 12
	//3840, 2160, 3840*12/8, 12
	//2016, 1508, 2016*12/8, 12
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_yuv_input = {
	"//mnt//sd//EXAM//CTRL//RAW2YUV//SRC//YUV_IN.RAW",
	1920, 1080, 1920, VDO_PXLFMT_YUV420
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_yout_input = {
	"//mnt//sd//EXAM//CTRL//RAW2YUV//SRC//SUBIN.RAW",
	// 1920x1080 src, 64x64 yout --> blk_size = 30x16
	64, 64, 64*12/8, 0x1E104040
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_ime_output[CTL_IPP_OUT_PATH_ID_MAX] = {
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT0", 1920, 1080, 1920, VDO_PXLFMT_YUV420},
	//{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT0", 2560, 1440, 2560, VDO_PXLFMT_YUV420},
	//{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT0", 3840, 2160, 3840, VDO_PXLFMT_YUV420},
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT1", 1280, 720, 1280, VDO_PXLFMT_YUV420},
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT2",  720, 480,  720, VDO_PXLFMT_YUV420},
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT3",  640, 360,  640, VDO_PXLFMT_Y8},
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT4",  160,  90,  160, VDO_PXLFMT_YUV420},
	{"//mnt//sd//EXAM//CTRL//RAW2YUV//TAR//OUT5", 1920, 1080, 1920, VDO_PXLFMT_520_IR8},
};
#elif defined(__FREERTOS)
static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_raw_input = {
	"A:\\EXAM\\CTRL\\RAW2YUV\\SRC\\IN.RAW",
	1920, 1080, 1920*12/8, 12
	//3840, 2160, 3840*12/8, 12
	//2016, 1508, 2016*12/8, 12
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_yuv_input = {
	"A:\\EXAM\\CTRL\\RAW2YUV\\SRC\\YUV_IN.RAW",
	1920, 1080, 1920, VDO_PXLFMT_YUV420
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_yout_input = {
	"A:\\EXAM\\CTRL\\RAW2YUV\\SRC\\SUBIN.RAW",
	// 1920x1080 src, 64x64 yout --> blk_size = 30x16
	64, 64, 64*12/8, 0x1E104040
};

static EXAM_CTL_IPP_IMGFILE g_ctl_ipp_ime_output[CTL_IPP_OUT_PATH_ID_MAX] = {
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT0", 1920, 1080, 1920, VDO_PXLFMT_YUV420},
	//{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT0", 2560, 1440, 2560, VDO_PXLFMT_YUV420},
	//{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT0", 3840, 2160, 3840, VDO_PXLFMT_YUV420},
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT1", 1280, 720, 1280, VDO_PXLFMT_YUV420},
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT2",  720, 480,  720, VDO_PXLFMT_YUV420},
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT3",  640, 360,  640, VDO_PXLFMT_Y8},
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT4",  160,  90,  160, VDO_PXLFMT_YUV420},
	{"A:\\EXAM\\CTRL\\RAW2YUV\\TAR\\OUT5", 1920, 1080, 1920, VDO_PXLFMT_520_IR8},
};
#endif

static UINT32 g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_MAX] = {
	ENABLE,
	ENABLE,
	DISABLE,
	DISABLE,
	DISABLE,
	ENABLE,
};

static CTL_IPP_OUT_BUF_INFO g_ctl_ipp_last_push_buf[CTL_IPP_OUT_PATH_ID_MAX];


static void exam_ctl_ipp_isp_set_ife_iq(ISP_ID id)
{
	static CTL_IPP_ISP_IFE_VIG_CENT_RATIO ife_vig_cent = {
		1000,
		{500, 500},
		{500, 500},
		{500, 500},
		{500, 500},
	};

	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IFE_VIG_CENT, (void *)&ife_vig_cent);
}

static void exam_ctl_ipp_isp_set_dce_iq(ISP_ID id)
{
	static KDRV_DCE_WDR_PARAM dce_wdr_param;
	static KDRV_DCE_WDR_SUBIMG_PARAM dce_wdr_subimg = {32, 32, 32*8, 32*8};
	static CTL_IPP_ISP_DCE_DC_CENT_RATIO dce_dc_cent = {1000, {500, 500}};
	static KDRV_DCE_HIST_PARAM dce_hist_param = {ENABLE, 0, 16, 16};
	static KDRV_DCE_PARAM_IQ_ALL_PARAM dce_iq;
	CTL_IPP_ISP_KDRV_PARAM kdrv_get_info;

	kdrv_get_info.eng = CTL_IPP_ISP_ENG_DCE;
	kdrv_get_info.param_id = KDRV_DCE_PARAM_IQ_WDR;
	kdrv_get_info.data = (void *)&dce_wdr_param;
	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_KDRV_PARAM, (void *)&kdrv_get_info);
	dce_wdr_param.wdr_enable = ENABLE;

	dce_hist_param.hist_sel = (dce_hist_param.hist_sel + 1) % 2;

	dce_iq.p_wdr_param = &dce_wdr_param;
	dce_iq.p_wdr_subimg_param = &dce_wdr_subimg;
	dce_iq.p_hist_param = &dce_hist_param;

	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_DCE_IQ_PARAM, (void *)&dce_iq);
	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_DCE_DC_CENT, (void *)&dce_dc_cent);
}

static void exam_ctl_ipp_isp_set_ipe_iq(ISP_ID id)
{
	static KDRV_IPE_DEFOG_PARAM defog_param;
	static KDRV_IPE_SUBIMG_PARAM defog_subimg = {{32, 32}, 32*4, 32*4, {1, 2, 1}};
	static KDRV_IPE_VA_PARAM va_param;
	static KDRV_IPE_LCE_PARAM lce_param;
	static KDRV_IPE_PARAM_IQ_ALL_PARAM ipe_iq;
	static CTL_IPP_ISP_VA_WIN_SIZE_RATIO va_win;
	CTL_IPP_ISP_KDRV_PARAM kdrv_get_info;
	UINT32 i;

	kdrv_get_info.eng = CTL_IPP_ISP_ENG_IPE;
	kdrv_get_info.param_id = KDRV_IPE_PARAM_IQ_DEFOG;
	kdrv_get_info.data = (void *)&defog_param;
	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_KDRV_PARAM, (void *)&kdrv_get_info);
	defog_param.enable = ENABLE;

	kdrv_get_info.eng = CTL_IPP_ISP_ENG_IPE;
	kdrv_get_info.param_id = KDRV_IPE_PARAM_IQ_LCE;
	kdrv_get_info.data = (void *)&lce_param;
	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_KDRV_PARAM, (void *)&kdrv_get_info);
	lce_param.enable = ENABLE;

	va_param.enable = ENABLE;
	va_param.indep_va_enable = ENABLE;
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
	va_param.win_num.h = 8;
	va_param.va_out_grp1_2 = 1;
	va_param.va_lofs = va_param.win_num.w * 2 * 8;
	va_param.group_1.h_filt.symmetry = 0;
	va_param.group_1.h_filt.filter_size = 1;
	va_param.group_1.h_filt.tap_a = 2;
	va_param.group_1.h_filt.tap_b = -1;
	va_param.group_1.h_filt.tap_c = 0;
	va_param.group_1.h_filt.tap_d = 0;
	va_param.group_1.h_filt.div = 3;
	va_param.group_1.h_filt.th_l = 4;
	va_param.group_1.h_filt.th_u = 200;
	va_param.group_1.v_filt.symmetry = 0;
	va_param.group_1.v_filt.filter_size = 1;
	va_param.group_1.v_filt.tap_a = 2;
	va_param.group_1.v_filt.tap_b = -1;
	va_param.group_1.v_filt.tap_c = 0;
	va_param.group_1.v_filt.tap_d = 0;
	va_param.group_1.v_filt.div = 3;
	va_param.group_1.v_filt.th_l = 4;
	va_param.group_1.v_filt.th_u = 200;
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

	va_win.ratio_base = 1000;
	va_win.winsz_ratio.w = 1000;
	va_win.winsz_ratio.h = 1000;
	for (i = 0; i < 5; i++){
		va_win.indep_roi_ratio[i].x = 100;
		va_win.indep_roi_ratio[i].y = 100;
		va_win.indep_roi_ratio[i].w = 200;
		va_win.indep_roi_ratio[i].h = 200;
	}

	ipe_iq.p_defog = &defog_param;
	ipe_iq.p_subimg = &defog_subimg;
	ipe_iq.p_va = &va_param;
	ipe_iq.p_lce = &lce_param;
	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IPE_IQ_PARAM, (void *)&ipe_iq);
	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_VA_WIN_SIZE, (void *)&va_win);
}

static void exam_ctl_ipp_isp_set_ime_iq(ISP_ID id)
{
	static CTL_IPP_ISP_IME_LCA_SIZE_RATIO ime_lca_size = {1000, 250};
	static KDRV_IME_TMNR_PARAM tmnr_param_test;
	static KDRV_IME_PARAM_IQ_ALL_PARAM ime_iq;
	CTL_IPP_ISP_KDRV_PARAM kdrv_get_info;

	kdrv_get_info.eng = CTL_IPP_ISP_ENG_IME;
	kdrv_get_info.param_id = KDRV_IME_PARAM_IQ_TMNR;
	kdrv_get_info.data = (void *)&tmnr_param_test;
	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_KDRV_PARAM, (void *)&kdrv_get_info);
	tmnr_param_test.enable = ENABLE;

	ime_iq.p_tmnr = &tmnr_param_test;
	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IME_IQ_PARAM, (void *)&ime_iq);
	ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IME_LCA_SIZE, (void *)&ime_lca_size);
}

static void exam_ctl_ipp_isp_set_eth(ISP_ID id)
{
	static BOOL buf_alloc;
	static UINT32 eth_cnt;
	static CTL_IPP_ISP_ETH eth_param = {DISABLE, 0, 2, 0, 0, {0, 0}, {0, 0}, 16, 48, 128, {0, 0}};
	CTL_IPP_HANDLE *p_hdl;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL) {
		CTL_IPP_DBG_WRN("p_hdl null \r\n");
		return;
	}

	if (buf_alloc == 0 && eth_param.enable) {
		NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('i', 'p', 'p', 't', 'e', 's', 't', 0);
		NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
		NVTMPP_DDR     ddr = NVTMPP_DDR_1;
		NVTMPP_VB_BLK  blk;
		UINT32 i;
		CTL_IPP_ISP_IOSIZE iosize;

		ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_IOSIZE, (void *)&iosize);
		for (i = 0; i < 2; i++) {
			if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
				/* original */
				eth_buf_info[i].size = ctl_ipp_util_ethsize(iosize.max_in_sz.w, iosize.max_in_sz.h, eth_param.out_bit_sel, FALSE);
				eth_buf_info[i].align = 64;      ///< address alignment
				eth_buf_info[i].name = "eth_out";

				blk = nvtmpp_vb_get_block(module, pool, eth_buf_info[i].size, ddr);
				if (blk == NVTMPP_VB_INVALID_BLK) {
					CTL_IPP_DBG_DUMP("get eth buffer fail\n");
					return ;
				}
				eth_buf_info[i].va_addr = (void *)nvtmpp_vb_blk2va(blk);
				eth_buf_info[i].phy_addr = nvtmpp_vb_blk2pa(blk);
				CTL_IPP_DBG_DUMP("eth buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)eth_buf_info[i].va_addr, (unsigned int)eth_buf_info[i].phy_addr, (unsigned int)eth_buf_info[i].size);

				/* subsample */
				eth_subsample_buf_info[i].size = ctl_ipp_util_ethsize(iosize.max_in_sz.w, iosize.max_in_sz.h, eth_param.out_bit_sel, TRUE);
				eth_subsample_buf_info[i].align = 64;      ///< address alignment
				eth_subsample_buf_info[i].name = "eth_out";

				blk = nvtmpp_vb_get_block(module, pool, eth_buf_info[i].size, ddr);
				if (blk == NVTMPP_VB_INVALID_BLK) {
					CTL_IPP_DBG_DUMP("get eth buffer fail\n");
					return ;
				}
				eth_subsample_buf_info[i].va_addr = (void *)nvtmpp_vb_blk2va(blk);
				eth_subsample_buf_info[i].phy_addr = nvtmpp_vb_blk2pa(blk);
				CTL_IPP_DBG_DUMP("eth buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)eth_subsample_buf_info[i].va_addr, (unsigned int)eth_subsample_buf_info[i].phy_addr, (unsigned int)eth_subsample_buf_info[i].size);
			} else {
				eth_buf_info[i].size = ctl_ipp_util_ethsize(iosize.max_in_sz.w, iosize.max_in_sz.h, eth_param.out_bit_sel, FALSE);
				eth_buf_info[i].align = 64;      ///< address alignment
				eth_buf_info[i].name = "eth_out";


				eth_buf_info[i].va_addr = ctl_ipp_util_os_malloc_wrap(eth_buf_info[i].size);
				if (eth_buf_info[i].va_addr == NULL) {
					DBG_DUMP("get eth buffer1 fail");
					return ;
				}
				eth_buf_info[i].phy_addr = vos_cpu_get_phy_addr((UINT32)eth_buf_info[i].va_addr);
				CTL_IPP_DBG_DUMP("eth buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)eth_buf_info[i].va_addr, (unsigned int)eth_buf_info[i].phy_addr, (unsigned int)eth_buf_info[i].size);

				/* subsample */
				eth_subsample_buf_info[i].size = ctl_ipp_util_ethsize(iosize.max_in_sz.w, iosize.max_in_sz.h, eth_param.out_bit_sel, TRUE);
				eth_subsample_buf_info[i].align = 64;      ///< address alignment
				eth_subsample_buf_info[i].name = "eth_out";

				eth_subsample_buf_info[i].va_addr = ctl_ipp_util_os_malloc_wrap(eth_buf_info[i].size);
				if (eth_subsample_buf_info[i].va_addr == NULL) {
					DBG_DUMP("get eth buffer1 fail");
					return ;
				}
				eth_subsample_buf_info[i].phy_addr = vos_cpu_get_phy_addr((UINT32)eth_subsample_buf_info[i].va_addr);
				CTL_IPP_DBG_DUMP("eth buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)eth_subsample_buf_info[i].va_addr, (unsigned int)eth_subsample_buf_info[i].phy_addr, (unsigned int)eth_subsample_buf_info[i].size);


			}
		}
		buf_alloc = 1;
	}

	/* get ready eth and set next eth param */
	{
		CTL_IPP_ISP_ETH rdy_eth;

		ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&rdy_eth);

		if (eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL || eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
			eth_param.buf_addr[0] = (UINT32)eth_buf_info[eth_cnt % 2].va_addr;
			eth_param.buf_size[0] = eth_buf_info[eth_cnt % 2].size;
		}

		if (eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE || eth_param.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
			eth_param.buf_addr[1] = (UINT32)eth_subsample_buf_info[eth_cnt % 2].va_addr;
			eth_param.buf_size[1] = eth_subsample_buf_info[eth_cnt % 2].size;
		}
		eth_cnt++;
		ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&eth_param);

		#if EXAM_CTRL_CB_LOG
		CTL_IPP_DBG_DUMP("rdy_eth: en %d, buf(0x%.8x, 0x%.8x)(0x%.8x, 0x%.8x), bit_sel %d, out_sel %d, size(%d %d), th(%d %d %d), frm(%d), ts(%lld)\r\n",
			(int)rdy_eth.enable, (unsigned int)rdy_eth.buf_addr[0], (unsigned int)rdy_eth.buf_size[0],
			(unsigned int)rdy_eth.buf_addr[1], (unsigned int)rdy_eth.buf_size[1],
			(int)rdy_eth.out_bit_sel, (int)rdy_eth.out_sel,
			(int)rdy_eth.out_size.w, (int)rdy_eth.out_size.h, (int)rdy_eth.th_low, (int)rdy_eth.th_mid, (int)rdy_eth.th_high,
			(int)rdy_eth.frm_cnt, rdy_eth.timestamp);


		CTL_IPP_DBG_DUMP("cur_eth: en %d, buf(0x%.8x, 0x%.8x)(0x%.8x, 0x%.8x), bit_sel %d, out_sel %d, size(%d %d), th(%d %d %d)\r\n",
			(int)eth_param.enable, (unsigned int)eth_param.buf_addr[0], (unsigned int)eth_param.buf_size[0],
			(unsigned int)eth_param.buf_addr[1], (unsigned int)eth_param.buf_size[1],
			(int)eth_param.out_bit_sel, (int)eth_param.out_sel,
			(int)eth_param.out_size.w, (int)eth_param.out_size.h, (int)eth_param.th_low, (int)eth_param.th_mid, (int)eth_param.th_high);
		#endif
	}
}

static INT32 exam_ctl_ipp_isp_cb(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	#if EXAM_CTRL_CB_LOG
	CTL_IPP_DBG_DUMP("---- ISP CB, id %d, evt 0x%.8x, fc %d ----\r\n", (int)id, (unsigned int)evt, (int)frame_cnt);
	if (evt == ISP_EVENT_IPP_CFGSTART) {
		if (proc_end_cnt < 8) {
			UINT32 i;
			ISP_FUNC_EN func_en;
			CTL_IPP_ISP_IOSIZE iosize;
			CTL_IPP_ISP_IFE_VIG_CENT_RATIO vig_cent;
			CTL_IPP_ISP_DCE_DC_CENT_RATIO dc_cent;
			CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size;
			CTL_IPP_ISP_DEFOG_SUBOUT defog_subout;
			CTL_IPP_ISP_3DNR_STA tmnr_sta_subout;

			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_FUNC_EN, (void *)&func_en);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_IOSIZE, (void *)&iosize);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_IFE_VIG_CENT, (void *)&vig_cent);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_DCE_DC_CENT, (void *)&dc_cent);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_IME_LCA_SIZE, (void *)&lca_size);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_DEFOG_SUBOUT, (void *)&defog_subout);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_3DNR_STA, (void *)&tmnr_sta_subout);

			CTL_IPP_DBG_DUMP("isp func_en 0x%.8x\r\n", (unsigned int)func_en);
			CTL_IPP_DBG_DUMP("isp in_sz = (%d %d)\r\n", (int)(iosize.in_sz.w), (int)(iosize.in_sz.h));
			for (i = 0; i < CTL_IPP_ISP_OUT_CH_MAX_NUM; i++) {
				CTL_IPP_DBG_DUMP("isp out_ch[%d] = (%d %d)\r\n", (int)(i), (int)(iosize.out_ch[i].w), (int)(iosize.out_ch[i].h));
			}

			CTL_IPP_DBG_DUMP("isp vig cent, ratio (%d, %d) / %d\r\n", (int)(vig_cent.ch0.x), (int)(vig_cent.ch0.y), (int)(vig_cent.ratio_base));
			CTL_IPP_DBG_DUMP("isp dc cent, ratio (%d, %d) / %d\r\n", (int)(dc_cent.center.x), (int)(dc_cent.center.y), (int)(dc_cent.ratio_base));
			CTL_IPP_DBG_DUMP("isp lca size, ratio %d / %d\r\n", (int)(lca_size.ratio), (int)(lca_size.ratio_base));
			CTL_IPP_DBG_DUMP("isp defog subout addr 0x%.8x\r\n", (unsigned int)defog_subout.addr);
			CTL_IPP_DBG_DUMP("isp 3dnr sta addr 0x%.8x, sampel_num %d %d\r\n", (unsigned int)tmnr_sta_subout.buf_addr, (int)tmnr_sta_subout.sample_num.w, (int)tmnr_sta_subout.sample_num.h);
		}
	}
	#endif

	if (evt == ISP_EVENT_IPP_CFGSTART) {
		exam_ctl_ipp_isp_set_ife_iq(id);
		exam_ctl_ipp_isp_set_dce_iq(id);
		exam_ctl_ipp_isp_set_ipe_iq(id);
		exam_ctl_ipp_isp_set_ime_iq(id);
		exam_ctl_ipp_isp_set_eth(id);
	}

	return E_OK;
}

static INT32 exam_ctl_ipp_isp_reg_cb(CHAR *name, UINT32 evt)
{
	ctl_ipp_isp_evt_fp_reg(name, exam_ctl_ipp_isp_cb, evt, CTL_IPP_ISP_CB_MSG_NONE);

	return E_OK;
}

static INT32 exam_ctl_ipp_isp_saveyuv(UINT32 id, UINT32 pid)
{
	CHAR *str_state[CTL_IPP_ISP_STS_MAX] = {
		"CLOSE",
		"READY",
		"RUN",
		"ID_NOT_FOUND",
	};
	CHAR *str_flow[CTL_IPP_ISP_FLOW_MAX] = {
		"UNKNOWN",
		"RAW",
		"DIRECT_RAW",
		"CCIR",
		"IME_D2D",
		"IPE_D2D",
	};
	CTL_IPP_ISP_STATUS_INFO status = {0};
	CTL_IPP_ISP_YUV_OUT img = {0};
	UINT32 size;
	CHAR file_name[128];

	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_STATUS_INFO, (void *)&status);
	CTL_IPP_DBG_DUMP("isp state %s, flow %s\r\n", str_state[status.sts], str_flow[status.flow]);

	if (status.sts != CTL_IPP_ISP_STS_READY &&
		status.sts != CTL_IPP_ISP_STS_RUN) {
		return E_OK;
	}

	img.pid = pid;
	if (ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_YUV_OUT, (void *)&img) == E_OK) {
		CTL_IPP_DBG_DUMP("isp_id %d, pid %d, get_yuv y_addr 0x%.8x, size(%d %d), fmt 0x%.8x\r\n",
			(int)id, (int)pid, (unsigned int)img.vdo_frm.addr[0], (int)img.vdo_frm.size.w, (int)img.vdo_frm.size.h, (unsigned int)img.vdo_frm.pxlfmt);

		snprintf(file_name, 128, CTL_IPP_INT_ROOT_PATH"yuv_fmt0x%.8x_size%dx%d_cnt%4lld.RAW", (unsigned int)img.vdo_frm.pxlfmt, (int)img.vdo_frm.size.w, (int)img.vdo_frm.size.h, img.vdo_frm.count);
		size = ctl_ipp_util_yuvsize(img.vdo_frm.pxlfmt, img.vdo_frm.size.w, img.vdo_frm.size.h);
		ctl_ipp_dbg_savefile(file_name, img.vdo_frm.addr[0], size);

		ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_YUV_OUT, (void *)&img);
	} else {
		CTL_IPP_DBG_DUMP("get yuv timeout %d %d\r\n", (int)(id), (int)(pid));
	}

	return E_OK;
}

static INT32 exam_ctl_ipp_in_buf_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_EVT *evt = (CTL_IPP_EVT *)in;

	#if EXAM_CTRL_CB_LOG
	CTL_IPP_DBG_DUMP("---- EXAM CTRL IN_BUF CB, msg %d, buf_id %d, err_msg %d ----\r\n", (int)(msg), (int)(evt->buf_id), (int)(evt->err_msg));
	#else
	if (g_ctl_ipp_api_dbg_log) {
		CTL_IPP_DBG_DUMP("---- EXAM CTRL IN_BUF CB, msg %d, buf_id %d, err_msg %d ----\r\n", (int)(msg), (int)(evt->buf_id), (int)(evt->err_msg));
	}
	#endif

	if (msg == CTL_IPP_CBEVT_IN_BUF_PROCEND) {
		proc_end_cnt += 1;
	}

	if (msg == CTL_IPP_CBEVT_IN_BUF_DROP) {
		drop_cnt += 1;
	}

	return 0;
}

static INT32 exam_ctl_ipp_ime_isr_cb(UINT32 msg, void *in, void *out)
{
	#if EXAM_CTRL_CB_LOG
	CTL_IPP_DBG_DUMP("---- EXAM CTRL IME ISR msg = %x ----\r\n", (unsigned int)(*(UINT32 *)in));
	#endif

	return 0;
}

static INT32 exam_ctl_ipp_bufio_cb(UINT32 msg, void *in, void *out)
{
	static UINT32 buf_cnt[CTL_IPP_OUT_PATH_ID_MAX] = {0};
	NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('i', 'p', 'p', 't', 'e', 's', 't', 0);
	NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
	NVTMPP_DDR     ddr = NVTMPP_DDR_1;
	NVTMPP_VB_BLK  blk;
	CTL_IPP_OUT_BUF_INFO *buf_info;
	UINT32 i, query_size;

	switch (msg) {
	case CTL_IPP_BUF_IO_NEW:
	{
		buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
		query_size = buf_info->buf_size;

		blk = nvtmpp_vb_get_block(module, pool, query_size, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			//CTL_IPP_DBG_DUMP("get output buffer fail %d\n", (int)(i));
			buf_info->buf_addr = 0;
			break;
		}

		i = buf_info->pid;
		buf_info->buf_addr = (UINT32)nvtmpp_vb_blk2va(blk);
		buf_info->buf_id = buf_cnt[i];
		buf_cnt[i] += 1;

		if (g_ctl_ipp_clr_buf) {
			memset((void *)buf_info->buf_addr, 0, buf_info->buf_size);
		}

		if (g_ctl_ipp_api_dbg_log) {
			CTL_IPP_DBG_DUMP("NEW Buffer, pid %d, Query size = 0x%.8x, Addr 0x%.8x, buf_id %d\r\n", (int)buf_info->pid,
								(unsigned int)query_size, (unsigned int)buf_info->buf_addr, (int)buf_info->buf_id);
		}
	}
	break;

	case CTL_IPP_BUF_IO_PUSH:
	{
		buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
		g_ctl_ipp_last_push_buf[buf_info->pid] = *buf_info;
		blk = nvtmpp_vb_va2blk(buf_info->buf_addr);
		nvtmpp_vb_unlock_block(module, blk);

		if (g_ctl_ipp_api_dbg_log) {
			CTL_IPP_DBG_DUMP("PUSH Buffer, Addr 0x%.8x, buf_id %d, err_msg %d\r\n", (unsigned int)buf_info->buf_addr, (int)buf_info->buf_id, (int)buf_info->err_msg);
		}
	}
	break;

	case CTL_IPP_BUF_IO_LOCK:
	{
		buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
		blk = nvtmpp_vb_va2blk(buf_info->buf_addr);
		if (nvtmpp_vb_lock_block(module, blk) != NVTMPP_ER_OK) {
			CTL_IPP_DBG_ERR("nvtmpp lock failed\r\n");
		}

		if (g_ctl_ipp_api_dbg_log) {
			CTL_IPP_DBG_DUMP("LOCK Buffer, Addr 0x%.8x, buf_id %d, err_msg %d\r\n", (unsigned int)buf_info->buf_addr, (int)buf_info->buf_id, (int)buf_info->err_msg);
		}
	}
	break;

	case CTL_IPP_BUF_IO_UNLOCK:
	{
		buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
		blk = nvtmpp_vb_va2blk(buf_info->buf_addr);
		nvtmpp_vb_unlock_block(module, blk);

		if (g_ctl_ipp_api_dbg_log) {
			CTL_IPP_DBG_DUMP("UNLOCK Buffer, pid %d, Addr 0x%.8x, buf_id %d, err_msg %d\r\n",
				(int)buf_info->pid, (unsigned int)buf_info->buf_addr, (int)buf_info->buf_id, (int)buf_info->err_msg);
		}
	}
	break;

	case CTL_IPP_BUF_IO_START:
	{
		if (g_ctl_ipp_api_dbg_log) {
			buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
			CTL_IPP_DBG_DUMP("START %d\r\n", (int)(buf_info->pid));
		}
	}
	break;

	case CTL_IPP_BUF_IO_STOP:
	{
		if (g_ctl_ipp_api_dbg_log) {
			buf_info = (CTL_IPP_OUT_BUF_INFO *)in;
			CTL_IPP_DBG_DUMP("STOP %d\r\n", (int)(buf_info->pid));
		}
	}
	break;


	default:
		break;
	}

	return 0;
}

static INT32 exam_ctl_ipp_datastamp_cb(UINT32 msg, void *in, void *out)
{
	/* blank line for style check */
	static BOOL is_init = FALSE;

	static UINT32 stamp_img_addr;
	static UINT32 stamp_buf_addr;
	CTL_IPP_DS_CB_INPUT_INFO *p_in;
	CTL_IPP_DS_CB_OUTPUT_INFO *p_out;
	CTL_IPP_DS_INFO *p_stamp;
	UINT32 stamp_size;
	UINT32 i;

	p_in = (CTL_IPP_DS_CB_INPUT_INFO *)in;
	p_out = (CTL_IPP_DS_CB_OUTPUT_INFO *)out;

	if (!is_init) {
		stamp_size = ALIGN_CEIL(((sizeof(UINT16) * 1600) + VOS_ALIGN_BYTES), VOS_ALIGN_BYTES);
		stamp_buf_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(stamp_size);
		if (stamp_buf_addr == 0) {
			CTL_IPP_DBG_ERR("data stamp buffer alloc failed\r\n");
			return 0;
		}

		stamp_img_addr = ALIGN_CEIL(stamp_buf_addr, VOS_ALIGN_BYTES);
		for (i = 0; i < 1600; i++) {
			((UINT16*)stamp_img_addr)[i] = 0xF000;
		}
		vos_cpu_dcache_sync(stamp_buf_addr, stamp_size, VOS_DMA_TO_DEVICE);
		is_init = TRUE;
	}

	for (i = 0; i < CTL_IPP_DS_SET_ID_MAX; i++) {
		p_stamp = &p_out->stamp[i];
		p_stamp->func_en = ENABLE;
		p_stamp->img_info.size.w = 40;
		p_stamp->img_info.size.h = 40;
		p_stamp->img_info.fmt = VDO_PXLFMT_RGB565;
		p_stamp->img_info.pos.x = 20 + (80 * i);
		p_stamp->img_info.pos.y = 20 + (80 * i);
		p_stamp->img_info.lofs = 80;
		p_stamp->img_info.addr = stamp_img_addr;

		p_stamp->iq_info.color_key_en = DISABLE;
		p_stamp->iq_info.color_key_val = 0;
		p_stamp->iq_info.bld_wt_0 = 0xf;
		p_stamp->iq_info.bld_wt_1 = 0xf;

		if (p_stamp->img_info.pos.x + p_stamp->img_info.size.w > p_in->img_size.w ||
			p_stamp->img_info.pos.y + p_stamp->img_info.size.h > p_in->img_size.h) {
			p_stamp->func_en = DISABLE;
		}

		#if EXAM_CTRL_CB_LOG
		CTL_IPP_DBG_DUMP("ds_set_%d: en %d, pos(%d %d), size(%d %d)\r\n", (int)i, (int)p_stamp->func_en,
			(int)p_stamp->img_info.pos.x, (int)p_stamp->img_info.pos.y,
			(int)p_stamp->img_info.size.w, (int)p_stamp->img_info.size.w);
		#endif
	}

	p_out->cst_info.func_en = ENABLE;
	p_out->cst_info.auto_mode_en = ENABLE;

	#if EXAM_CTRL_CB_LOG
	CTL_IPP_DBG_DUMP("ctl_handle 0x%.8x, img_size(%d %d), stamp_addr 0x%.8x\r\n",
		(unsigned int)p_in->ctl_ipp_handle, (int)p_in->img_size.w, (int)p_in->img_size.h, stamp_img_addr);
	#endif

	return 0;
}

static INT32 exam_ctl_ipp_primask_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_PM_CB_INPUT_INFO *p_in;
	CTL_IPP_PM_CB_OUTPUT_INFO *p_out;
	CTL_IPP_PM *p_mask;
	UINT32 i;

	p_in = (CTL_IPP_PM_CB_INPUT_INFO *)in;
	p_out = (CTL_IPP_PM_CB_OUTPUT_INFO *)out;

	for (i = 0; i < CTL_IPP_PM_SET_ID_MAX; i++) {
		/*
			coord need to be sorted due to ime driver not ready
		*/
		p_mask = &p_out->mask[i];
		p_mask->func_en = ENABLE;
		p_mask->pm_coord[0].x = 60 + (80 * i);
		p_mask->pm_coord[0].y = 60 + (80 * i);
		p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + 40;
		p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
		p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + 40;
		p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + 40;
		p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
		p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + 40;
		if (i >= 4) {
			p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
		} else {
			p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_PXL;
			p_out->pxl_blk_size = CTL_IPP_PM_PXL_BLK_64;
		}
		p_mask->color[0] = 127;
		p_mask->color[1] = 255;
		p_mask->color[2] = 127;
		p_mask->alpha_weight = 255;

		if (p_mask->pm_coord[2].x > (INT32)p_in->img_size.w ||
			p_mask->pm_coord[2].y > (INT32)p_in->img_size.h) {
			p_mask->func_en = DISABLE;
		}

		#if EXAM_CTRL_CB_LOG
		CTL_IPP_DBG_DUMP("pm_set_%d: en %d, p0(%d %d), p1(%d %d), p2(%d %d), p3(%d %d)\r\n", (int)i, (int)p_mask->func_en,
			(int)p_mask->pm_coord[0].x, (int)p_mask->pm_coord[0].y, (int)p_mask->pm_coord[1].x, (int)p_mask->pm_coord[1].y,
			(int)p_mask->pm_coord[2].x, (int)p_mask->pm_coord[2].y, (int)p_mask->pm_coord[3].x, (int)p_mask->pm_coord[3].y);
		#endif
	}

	return 0;
}

static void exam_ctl_ipp_config(UINT32 hdl, UINT32 isp_id)
{
	UINT32 i, apply_id;
	UINT32 reference_path;
	UINT32 low_delay_path;
	UINT32 low_delay_bp;
	CTL_IPP_FUNC func_en;
	CTL_IPP_IN_CROP rhe_crop;
	CTL_IPP_FLIP_TYPE rhe_flip;
	CTL_IPP_OUT_PATH ime_out;
	CTL_IPP_SCL_METHOD_SEL scl_method_sel;
	CTL_IPP_ALGID algid;
	CTL_IPP_OUT_PATH_MD ime_out_md;

	/* ALG ID */
	algid.type = CTL_IPP_ALGID_IQ;
	algid.id = isp_id;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_ALGID_IMM, (void *)&algid);

	/* Function enable */
	func_en = CTL_IPP_FUNC_3DNR;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

	/* Crop size reference input header */
	rhe_crop.mode = CTL_IPP_IN_CROP_USER;//CTL_IPP_IN_CROP_NONE;
	rhe_crop.crp_window.x = 0;
	rhe_crop.crp_window.y = 0;
	rhe_crop.crp_window.w = 1920;
	rhe_crop.crp_window.h = 1080;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_IN_CROP, (void *)&rhe_crop);

	/* Input flip */
	rhe_flip = CTL_IPP_FLIP_NONE;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_FLIP, (void *)&rhe_flip);

	/* IME Output path */
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		ime_out.pid = i;
		ime_out.enable = g_ctl_ipp_ime_output_enable[i];
		ime_out.fmt = g_ctl_ipp_ime_output[i].fmt;
		ime_out.lofs = g_ctl_ipp_ime_output[i].lofs;
		ime_out.size.w = g_ctl_ipp_ime_output[i].width;
		ime_out.size.h = g_ctl_ipp_ime_output[i].height;
		ime_out.crp_window.x = 0;
		ime_out.crp_window.y = 0;
		ime_out.crp_window.w = ime_out.size.w;
		ime_out.crp_window.h = ime_out.size.h;
		ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);

		if (func_en & CTL_IPP_FUNC_3DNR) {
			if (ime_out.enable) {
				ime_out_md.pid = i;
				ime_out_md.enable = ENABLE;
				ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH_MD, (void *)&ime_out_md);
			}
		}
	}

	/* IME Output scale method select */
	scl_method_sel.scl_th = 0x00010001;
	scl_method_sel.method_l = CTL_IPP_SCL_AUTO;
	scl_method_sel.method_h = CTL_IPP_SCL_AUTO;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_SCL_METHOD, (void *)&scl_method_sel);

	/* IME Output Low Delay Path */
	low_delay_path = g_ctl_ipp_low_delay_path;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_LOW_DELAY_PATH_SEL, (void *)&low_delay_path);

	low_delay_bp = 50;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_LOW_DELAY_BP, (void *)&low_delay_bp);

	/* IME 3DNR Reference path select */
	if (func_en & CTL_IPP_FUNC_3DNR) {
		reference_path = CTL_IPP_OUT_PATH_ID_1;
		ctl_ipp_set(hdl, CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&reference_path);
	}

	/* private buf calculate */
	{
		CTL_IPP_PRIVATE_BUF buf_info;
		CTL_IPP_BUFCFG buf_cfg;

		memset((void *)&buf_info, 0, sizeof(CTL_IPP_PRIVATE_BUF));
		buf_info.func_en = CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_SHDR | CTL_IPP_FUNC_3DNR;
		buf_info.max_size.w = g_ctl_ipp_raw_input.width;
		buf_info.max_size.h = g_ctl_ipp_raw_input.height;
		buf_info.pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 1, VDO_PIX_RGGB_R);
		ctl_ipp_get(hdl, CTL_IPP_ITEM_BUFQUY, (void *)&buf_info);

		if (buf_info.buf_size > 0) {
			buf_cfg.start_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(buf_info.buf_size);
			buf_cfg.size = buf_info.buf_size;
			ctl_ipp_set(hdl, CTL_IPP_ITEM_BUFCFG, (void *)&buf_cfg);
		}
	}

	/* set apply */
	ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
}

static INT32 exam_ctl_ipp_buf_alloc(void)
{
	/* allocate buffer */
	UINT32 fail_flg;

	CTL_IPP_DBG_TRC("ctrl r2y flow alloc memory\n");

	{
		NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('i', 'p', 'p', 't', 'e', 's', 't', 0);
		NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
		NVTMPP_DDR     ddr = NVTMPP_DDR_1;
		NVTMPP_VB_BLK  blk;
		NVTMPP_VB_CONF_S st_conf;

		memset(&st_conf, 0, sizeof(NVTMPP_VB_CONF_S));
		st_conf.max_pool_cnt = 64;
	    st_conf.common_pool[0].blk_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV420, 3840, 2160) + 1024;
		st_conf.common_pool[0].blk_cnt = EXAM_IPP_BUF_NUM;
		st_conf.common_pool[0].ddr = NVTMPP_DDR_1;
		st_conf.common_pool[0].type = POOL_TYPE_COMMON;
		st_conf.common_pool[1].blk_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV420, 2560, 1080) + 1024;
		st_conf.common_pool[1].blk_cnt = EXAM_IPP_BUF_NUM;
		st_conf.common_pool[1].ddr = NVTMPP_DDR_1;
		st_conf.common_pool[1].type = POOL_TYPE_COMMON;
		st_conf.common_pool[2].blk_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV420, 1920, 1080) + 1024;
		st_conf.common_pool[2].blk_cnt = EXAM_IPP_BUF_NUM;
		st_conf.common_pool[2].ddr = NVTMPP_DDR_1;
		st_conf.common_pool[2].type = POOL_TYPE_COMMON;
		st_conf.common_pool[3].blk_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV420, 1280, 720) + 1024;
		st_conf.common_pool[3].blk_cnt = EXAM_IPP_BUF_NUM;
		st_conf.common_pool[3].ddr = NVTMPP_DDR_1;
		st_conf.common_pool[3].type = POOL_TYPE_COMMON;
		st_conf.common_pool[4].blk_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV420, 640, 480) + 1024;
		st_conf.common_pool[4].blk_cnt = 8;
		st_conf.common_pool[4].ddr = NVTMPP_DDR_1;
		st_conf.common_pool[4].type = POOL_TYPE_COMMON;

		nvtmpp_vb_set_conf(&st_conf);
		nvtmpp_vb_init();
		nvtmpp_vb_add_module(module);

		/*get in buffer*/
		fail_flg = FALSE;

		in_raw_buf_info.size = g_ctl_ipp_raw_input.lofs * g_ctl_ipp_raw_input.height;
		in_raw_buf_info.align = 64;      ///< address alignment
		in_raw_buf_info.name = "ctl_t_in";
		blk = nvtmpp_vb_get_block(module, pool, in_raw_buf_info.size, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			CTL_IPP_DBG_DUMP("get input buffer fail\n");
			fail_flg = TRUE;
		}
		in_raw_buf_info.va_addr = (void *)nvtmpp_vb_blk2va(blk);
		in_raw_buf_info.phy_addr = nvtmpp_vb_blk2pa(blk);
		CTL_IPP_DBG_TRC("input raw buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)in_raw_buf_info.va_addr, (unsigned int)in_raw_buf_info.phy_addr, (unsigned int)in_raw_buf_info.size);

		in_yuv_buf_info.size = ctl_ipp_util_yuvsize(g_ctl_ipp_yuv_input.fmt, g_ctl_ipp_yuv_input.width, g_ctl_ipp_yuv_input.height);
		in_yuv_buf_info.align = 64;      ///< address alignment
		in_yuv_buf_info.name = "ctl_t_in";
		blk = nvtmpp_vb_get_block(module, pool, in_yuv_buf_info.size, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			CTL_IPP_DBG_DUMP("get input buffer fail\n");
			fail_flg = TRUE;
		}
		in_yuv_buf_info.va_addr = (void *)nvtmpp_vb_blk2va(blk);
		in_yuv_buf_info.phy_addr = nvtmpp_vb_blk2pa(blk);
		CTL_IPP_DBG_TRC("input yuv buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)in_yuv_buf_info.va_addr, (unsigned int)in_yuv_buf_info.phy_addr, (unsigned int)in_yuv_buf_info.size);


		in_yout_buf_info.size = g_ctl_ipp_yout_input.lofs * g_ctl_ipp_yout_input.height;
		in_yout_buf_info.align = 64;      ///< address alignment
		in_yout_buf_info.name = "ctl_t_yout_in";
		blk = nvtmpp_vb_get_block(module, pool, in_yout_buf_info.size, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			CTL_IPP_DBG_DUMP("get yout input buffer fail\n");
			fail_flg = TRUE;
		}
		in_yout_buf_info.va_addr = (void *)nvtmpp_vb_blk2va(blk);
		in_yout_buf_info.phy_addr = nvtmpp_vb_blk2pa(blk);
		CTL_IPP_DBG_TRC("yout input buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)in_yout_buf_info.va_addr, (unsigned int)in_yout_buf_info.phy_addr, (unsigned int)in_yout_buf_info.size);

		/*get out buffer*/
		#if 0
		out_alloc_cnt = 0;
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (g_ctl_ipp_ime_output_enable[i] == DISABLE) {
				continue;
			}

			for (j = 0; j < EXAM_IPP_BUF_NUM; j++) {
			    out_buf_info[i][j].size = ctl_ipp_util_yuvsize(g_ctl_ipp_ime_output[i].fmt, g_ctl_ipp_ime_output[i].width, g_ctl_ipp_ime_output[i].height);
			    out_buf_info[i][j].align = 64;      ///< address alignment
			    out_buf_info[i][j].name = "ctl_t_out";
				blk = nvtmpp_vb_get_block(module, pool, out_buf_info[i][j].size, ddr);
				if (blk == NVTMPP_VB_INVALID_BLK) {
					CTL_IPP_DBG_DUMP("get output buffer fail %d\n", (int)(i));
					fail_flg = TRUE;
					break;
				}
				out_buf_info[i][j].va_addr = (void *)nvtmpp_vb_blk2va(blk);
				out_buf_info[i][j].phy_addr = nvtmpp_vb_blk2pa(blk);
				CTL_IPP_DBG_TRC("output buffer%d (%d, %d, %d, %d) addr 0x%.8x(0x%.8x) size 0x%.8x\r\n",
					(int)i, (int)g_ctl_ipp_ime_output[i].width, (int)g_ctl_ipp_ime_output[i].height,
					(int)g_ctl_ipp_ime_output[i].lofs, (int)g_ctl_ipp_ime_output[i].fmt,
					(unsigned int)out_buf_info[i][j].va_addr, (unsigned int)out_buf_info[i][j].phy_addr, (unsigned int)out_buf_info[i][j].size);
				out_alloc_cnt += 1;
			}
		}
		#endif
		if (fail_flg) {
			nvtmpp_vb_exit();
		}
	}
	g_ctl_ipp_load_img_flg = 0;

	return 0;
}

static INT32 exam_ctl_ipp_buf_release(void)
{
	NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('i', 'p', 'p', 't', 'e', 's', 't', 0);
	NVTMPP_VB_BLK  blk;

	/* release input buffer */
	blk = nvtmpp_vb_va2blk((UINT32)in_raw_buf_info.va_addr);
	nvtmpp_vb_unlock_block(module, blk);

	blk = nvtmpp_vb_va2blk((UINT32)in_yuv_buf_info.va_addr);
	nvtmpp_vb_unlock_block(module, blk);

	blk = nvtmpp_vb_va2blk((UINT32)in_yout_buf_info.va_addr);
	nvtmpp_vb_unlock_block(module, blk);

	nvtmpp_vb_exit();

	return 0;
}

static INT32 exam_ctl_ipp_tsk_pause(BOOL wait_end, BOOL flush)
{
	if (ctl_ipp_set_pause(wait_end, flush) != E_OK) {
		CTL_IPP_DBG_DUMP("Pause ctl tsk fail\r\n");
		return -1;
	}

	return 0;
}

static INT32 exam_ctl_ipp_tsk_resume(BOOL flush)
{
	if (ctl_ipp_set_resume(flush) != E_OK) {
		CTL_IPP_DBG_DUMP("Resume ctl tsk fail\r\n");
		return -1;
	}

	return 0;
}

static INT32 exam_ctl_ipp_create_hdl(UINT32 hdl_idx)
{
	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
		CTL_IPP_REG_CB_INFO cb_info = {0};
		CHAR hdl_name[16];

		if (g_ctl_ipp_hdl_list[hdl_idx] != 0) {
			CTL_IPP_DBG_DUMP("hdl_list[%d] 0x%.8x != 0\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);
			return -1;
		}

		snprintf(hdl_name, 16, "exam_hdl_%d", (int)hdl_idx);
		g_ctl_ipp_hdl_list[hdl_idx] = ctl_ipp_open(hdl_name, g_ctl_ipp_flow_sel[hdl_idx]);
		if (g_ctl_ipp_hdl_list[hdl_idx] == 0) {
			CTL_IPP_DBG_DUMP("Open ctrl hdl fail\r\n");
			return -1;
		}

		cb_info.cbevt = CTL_IPP_CBEVT_IN_BUF;
		cb_info.fp = exam_ctl_ipp_in_buf_cb;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

		cb_info.cbevt = CTL_IPP_CBEVT_ENG_IME_ISR;
		cb_info.fp = exam_ctl_ipp_ime_isr_cb;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

		cb_info.cbevt = CTL_IPP_CBEVT_OUT_BUF;
		cb_info.fp = exam_ctl_ipp_bufio_cb;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

		cb_info.cbevt = CTL_IPP_CBEVT_DATASTAMP;
		cb_info.fp = exam_ctl_ipp_datastamp_cb;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

		cb_info.cbevt = CTL_IPP_CBEVT_PRIMASK;
		cb_info.fp = exam_ctl_ipp_primask_cb;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

		/* config setting */
		exam_ctl_ipp_config(g_ctl_ipp_hdl_list[hdl_idx], hdl_idx);

		CTL_IPP_DBG_DUMP("Open ctrl hdl to idx %d success 0x%.8x\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);
	}

	return 0;
}

static INT32 exam_ctl_ipp_release_hdl(UINT32 hdl_idx)
{
	CTL_IPP_HANDLE *p_hdl;

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
		if (g_ctl_ipp_hdl_list[hdl_idx] == 0) {
			CTL_IPP_DBG_DUMP("hdl_list[%d] = 0\r\n", (int)(hdl_idx));
			return FALSE;
		}

		/* free pri buffer */
		p_hdl = (CTL_IPP_HANDLE *)g_ctl_ipp_hdl_list[hdl_idx];
		if (p_hdl->bufcfg.start_addr != 0) {
			ctl_ipp_util_os_mfree_wrap((void *)p_hdl->bufcfg.start_addr);
		}

		if (ctl_ipp_close(g_ctl_ipp_hdl_list[hdl_idx]) == CTL_IPP_E_OK) {
			g_ctl_ipp_hdl_list[hdl_idx] = 0;
		} else {
			CTL_IPP_DBG_DUMP("close failed\r\n");
			return FALSE;
		}

		CTL_IPP_DBG_DUMP("Close ctrl hdl_idx %d success\r\n", (int)(hdl_idx));
	}

	return 0;
}

static INT32 exam_ctl_ipp_load_input_image(void)
{
	VOS_FILE fp;

	if (g_ctl_ipp_load_img_flg == 0) {
		fp = vos_file_open(g_ctl_ipp_raw_input.filename, O_RDONLY, 0);
		if (fp == (VOS_FILE)(-1)) {
		    CTL_IPP_DBG_DUMP("failed in file open:%s\n", g_ctl_ipp_raw_input.filename);
			return -1;
		}
		vos_file_read(fp, in_raw_buf_info.va_addr, in_raw_buf_info.size);
		vos_file_close(fp);

		fp = vos_file_open(g_ctl_ipp_yuv_input.filename, O_RDONLY, 0);
		if (fp == (VOS_FILE)(-1)) {
		    CTL_IPP_DBG_DUMP("failed in file open:%s\n", g_ctl_ipp_yuv_input.filename);
			return -1;
		}
		vos_file_read(fp, in_yuv_buf_info.va_addr, in_yuv_buf_info.size);
		vos_file_close(fp);

		g_ctl_ipp_load_img_flg = 1;
	}
	return 0;
}

static VDO_FRAME *exam_ctl_ipp_get_vdoframe(UINT32 flow_sel)
{
	static VDO_FRAME vdoin_info_raw;
	static VDO_FRAME vdoin_info_yuv;

	if (flow_sel == CTL_IPP_FLOW_DCE_D2D ||
		flow_sel == CTL_IPP_FLOW_IPE_D2D ||
		flow_sel == CTL_IPP_FLOW_IME_D2D ||
		flow_sel == CTL_IPP_FLOW_CCIR ||
		flow_sel == CTL_IPP_FLOW_CAPTURE_CCIR) {
		vdoin_info_yuv.sign = MAKEFOURCC('V', 'F', 'R', 'M');
		vdoin_info_yuv.pxlfmt = g_ctl_ipp_yuv_input.fmt;
		vdoin_info_yuv.size.w = g_ctl_ipp_yuv_input.width;
		vdoin_info_yuv.size.h = g_ctl_ipp_yuv_input.height;
		vdoin_info_yuv.loff[VDO_PINDEX_Y] = g_ctl_ipp_yuv_input.lofs;
		vdoin_info_yuv.loff[VDO_PINDEX_UV] = ctl_ipp_util_y2uvlof(vdoin_info_yuv.pxlfmt, g_ctl_ipp_yuv_input.lofs);
		vdoin_info_yuv.addr[VDO_PINDEX_Y] = (UINT32)in_yuv_buf_info.va_addr;
		vdoin_info_yuv.addr[VDO_PINDEX_UV] = vdoin_info_yuv.addr[VDO_PINDEX_Y] + (vdoin_info_yuv.loff[VDO_PINDEX_Y] * vdoin_info_yuv.size.h);
		vdoin_info_yuv.count += 1;
		return &vdoin_info_yuv;
	} else {
		vdoin_info_raw.sign = MAKEFOURCC('V', 'F', 'R', 'M');
		vdoin_info_raw.pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 1, VDO_PIX_RGGB_R);
		vdoin_info_raw.size.w = g_ctl_ipp_raw_input.width;
		vdoin_info_raw.size.h = g_ctl_ipp_raw_input.height;
		vdoin_info_raw.loff[VDO_PINDEX_RAW] = g_ctl_ipp_raw_input.lofs;
		vdoin_info_raw.addr[VDO_PINDEX_RAW] = (UINT32)in_raw_buf_info.va_addr;
		vdoin_info_raw.addr[1] = (UINT32)&vdoin_info_raw;
		vdoin_info_raw.addr[2] = (UINT32)&vdoin_info_raw;
		vdoin_info_raw.addr[3] = (UINT32)&vdoin_info_raw;
		vdoin_info_raw.reserved[5] = 0;
		vdoin_info_raw.reserved[6] = (vdoin_info_raw.size.w << 16) | vdoin_info_raw.size.h;
		vdoin_info_raw.count += 1;
		return &vdoin_info_raw;
	}
}

static INT32 exam_ctl_ipp_sndevt(UINT32 hdl_idx, UINT32 cnt)
{
	static UINT32 buf_id_cnt;
	VDO_FRAME *p_vdofrm;
	UINT32 i;

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM && cnt > 0) {
		CTL_IPP_EVT evt;

		/* load image */
		if (exam_ctl_ipp_load_input_image() != 0) {
			return -1;
		}

		/* get vdo frame info */
		p_vdofrm = exam_ctl_ipp_get_vdoframe(g_ctl_ipp_flow_sel[hdl_idx]);

		evt.buf_id = buf_id_cnt;
		evt.data_addr = (UINT32)p_vdofrm;
		evt.rev = 0;
		for (i = 0; i < cnt; i++) {
			buf_id_cnt += 1;
			evt.buf_id = buf_id_cnt;
			p_vdofrm->timestamp = ctl_ipp_util_get_syst_timestamp();

			if (i > 0) {
				vos_util_delay_us(33000);
			}

			if (ctl_ipp_ioctl(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
				return -1;
			}
		}
	}

	return 0;
}

static INT32 exam_ctl_ipp_sndevt_hdr(UINT32 hdl_idx, UINT32 cnt)
{
	static UINT32 buf_id_cnt;
	VDO_FRAME *p_vdofrm;
	UINT32 i;

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM && cnt > 0) {
		CTL_IPP_EVT_HDR evt;

		/* load image */
		if (exam_ctl_ipp_load_input_image() != 0) {
			return -1;
		}

		/* get vdo frame info */
		p_vdofrm = exam_ctl_ipp_get_vdoframe(g_ctl_ipp_flow_sel[hdl_idx]);
		p_vdofrm->pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 2, VDO_PIX_RGGB_R);

		evt.buf_id[0] = buf_id_cnt;
		evt.buf_id[1] = buf_id_cnt;
		evt.data_addr[0] = (UINT32)p_vdofrm;
		evt.data_addr[1] = (UINT32)p_vdofrm;
		evt.rev = 0;
		for (i = 0; i < cnt; i++) {
			buf_id_cnt += 1;
			evt.buf_id[0] = buf_id_cnt;
			evt.buf_id[1] = buf_id_cnt;
			p_vdofrm->timestamp = ctl_ipp_util_get_syst_timestamp();

			if (ctl_ipp_ioctl(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDEVT_HDR, (void *)&evt) != CTL_IPP_E_OK) {
				CTL_IPP_DBG_DUMP("Send ctl evt fail\r\n");
				return -1;
			}
			vos_util_delay_ms(40);
		}
	}

	return 0;
}

static INT32 exam_ctl_ipp_sndevt_pattern_paste(UINT32 hdl_idx, UINT32 cnt, UINT32 addr, UINT32 width, UINT32 height, UINT32 lofs)
{
	static UINT32 buf_id_cnt;
	static VDO_FRAME vdoin_info_yuv;
	VDO_FRAME *p_vdofrm;
	UINT32 i;

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM && cnt > 0) {
		CTL_IPP_EVT_PATTERN_PASTE evt;

		/* load image */
		if (exam_ctl_ipp_load_input_image() != 0) {
			return -1;
		}

		/* get vdo frame info (only y plane, one line) */
		vdoin_info_yuv.sign = MAKEFOURCC('V', 'F', 'R', 'M');
		vdoin_info_yuv.pxlfmt = VDO_PXLFMT_YUV420;
		vdoin_info_yuv.size.w = width;
		vdoin_info_yuv.size.h = height;
		vdoin_info_yuv.loff[VDO_PINDEX_Y] = lofs;
		vdoin_info_yuv.loff[VDO_PINDEX_UV] = lofs;
		vdoin_info_yuv.addr[VDO_PINDEX_Y] = addr;
		vdoin_info_yuv.addr[VDO_PINDEX_UV] = addr;
		vdoin_info_yuv.count += 1;
		p_vdofrm = &vdoin_info_yuv;

		evt.buf_id = buf_id_cnt;
		evt.data_addr = (UINT32)p_vdofrm;
		evt.rev = 0;
		for (i = 0; i < cnt; i++) {
			buf_id_cnt += 1;
			evt.buf_id = buf_id_cnt;
			p_vdofrm->timestamp = ctl_ipp_util_get_syst_timestamp();

			if (i > 0) {
				vos_util_delay_us(33000);
			}

			if (ctl_ipp_ioctl(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDEVT_PATTERN_PASTE, (void *)&evt) != CTL_IPP_E_OK) {
				return -1;
			}
		}
	}

	return 0;
}

static INT32 exam_ctl_ipp_saverst(UINT32 hdl_idx)
{
	UINT32 i;
	CHAR out_filename[256];
	CTL_IPP_OUT_BUF_INFO *p_buf;

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
		VOS_FILE fp;

		if (g_ctl_ipp_hdl_list[hdl_idx] == 0) {
			CTL_IPP_DBG_DUMP("hdl_list[%d] = 0\r\n", (int)(hdl_idx));
			return FALSE;
		}

		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			p_buf = &g_ctl_ipp_last_push_buf[i];
			if (p_buf->buf_addr != 0) {
				snprintf(out_filename, 256, "%s_%d.RAW", g_ctl_ipp_ime_output[i].filename, (int)p_buf->buf_id);
				fp = vos_file_open(out_filename, O_CREAT|O_WRONLY|O_SYNC, 0);
				if (fp == (VOS_FILE)(-1)) {
				    CTL_IPP_DBG_DUMP("failed in file open:%s\n", g_ctl_ipp_ime_output[i].filename);
					return FALSE;
				}

				vos_file_write(fp, (void *)p_buf->buf_addr, p_buf->buf_size);
				vos_file_close(fp);

				CTL_IPP_DBG_DUMP("SAVE IME PATH_%d, Size(%d %d, 0x%.8x) Type(0x%.8x), Output Addr(0x%.8x), buf_id(%d)\r\n", (int)i+1,
						(int)p_buf->vdo_frm.size.w, (int)p_buf->vdo_frm.size.h,
						(unsigned int)p_buf->buf_size, (unsigned int)p_buf->vdo_frm.pxlfmt, (unsigned int)p_buf->buf_addr, (int)p_buf->buf_id);
			}
		}
	}

	return 0;
}

THREAD_DECLARE(exam_ctl_ipp_sndevt_task, p1)
{
	ID ctl_ipp_tmp_flg_id;
	UINT32 hdl_idx;
	UINT32 interval;
	UINT32 op;

	hdl_idx = ((UINT32 *)p1)[0];
	interval = ((UINT32 *)p1)[1];
	if (hdl_idx >= EXAM_CTRL_HDL_MAX_NUM) {
		CTL_IPP_DBG_ERR("Illegal handle index %d\r\n", (int)hdl_idx);
		THREAD_RETURN(0);
	}

	DBG_DUMP("exam_ctl_ipp task start sndevt to hdlidx %d, interval %d(us)\r\n", (int)hdl_idx, (int)interval);

	if (OS_CONFIG_FLAG(ctl_ipp_tmp_flg_id) != E_OK) {
		CTL_IPP_DBG_ERR("exam_ctl_ipp task create flag failed\r\n");
		THREAD_RETURN(0);
	}

	g_ctl_ipp_sndevt_tsk_op[hdl_idx] = EXAM_CTRL_IPP_TASK_OP_SNDEVT;
	vos_flag_clr(ctl_ipp_tmp_flg_id, 0xFFFFFFFF);
	while (1) {
		op = g_ctl_ipp_sndevt_tsk_op[hdl_idx];
		if (op & EXAM_CTRL_IPP_TASK_OP_EXIT) {
			break;
		}

		if (op & EXAM_CTRL_IPP_TASK_OP_SNDEVT) {
			exam_ctl_ipp_sndevt(hdl_idx, 1);
		}

		if (op & EXAM_CTRL_IPP_TASK_OP_FLUSH) {
			UINT32 path, apply_id;
			CTL_IPP_OUT_PATH path_cfg;

			DBG_DUMP("sndevt task, set all path to disable and flush\r\n");
			g_ctl_ipp_sndevt_tsk_op[hdl_idx] &= ~EXAM_CTRL_IPP_TASK_OP_FLUSH;
			for (path = CTL_IPP_OUT_PATH_ID_1; path < CTL_IPP_OUT_PATH_ID_MAX; path++) {
				memset((void *)&path_cfg, 0, sizeof(CTL_IPP_OUT_PATH));
				path_cfg.pid = path;
				ctl_ipp_get(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);

				if (path_cfg.enable == ENABLE) {
					path_cfg.enable = DISABLE;
					ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
				}
			}
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, &apply_id);
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FLUSH, NULL);
		}

		vos_util_delay_us(interval);
	}
	rel_flg(ctl_ipp_tmp_flg_id);
	THREAD_RETURN(0);
}

static void exam_ctl_ipp_sndevt_timer_op(UINT32 op, UINT32 hdl_idx, UINT32 data)
{
	if (op == 0) {
		g_ctl_ipp_sndevt_tsk_op[hdl_idx] = EXAM_CTRL_IPP_TASK_OP_EXIT;
	} else if (op == 1) {
		static UINT32 param[2];

		param[0] = hdl_idx;
		param[1] = data;

		THREAD_CREATE(g_ctl_ipp_sndevt_tsk_id[hdl_idx], exam_ctl_ipp_sndevt_task, (void *)&param[0], "exam_ctl_ipp_sndevt_task");
		THREAD_SET_PRIORITY(g_ctl_ipp_sndevt_tsk_id[hdl_idx], CTL_IPP_TSK_PRI);
		THREAD_RESUME(g_ctl_ipp_sndevt_tsk_id[hdl_idx]);
	} else if (op == 2) {
		g_ctl_ipp_sndevt_tsk_op[hdl_idx] = data;
	} else {
		DBG_DUMP("Unknown op for sndevt_auto\r\n");
		DBG_DUMP("sndevt 0 hdl_idx:          close timer\r\n");
		DBG_DUMP("sndevt 1 hdl_idx interval: open timer\r\n");
		DBG_DUMP("sndevt 2 hdl_idx data: 	 set task operation\r\n");
	}
}

static INT32 exam_ctl_ipp_direct_set_cb(UINT32 sie_hdl)
{
	CTL_SIE_REG_CB_INFO cb_info;

	DBG_DUMP("cb set %x \r\n",sie_hdl);
	cb_info.fp = (CTL_SIE_EVENT_FP)ctl_ipp_get_dir_fp(0); //register direct mode cb to SIE
	cb_info.cbevt = CTL_SIE_CBEVT_DIRECT;
	cb_info.sts = CTL_SIE_DIRECT_CFG_ALL;
	ctl_sie_set((UINT32) sie_hdl , CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

	return 0;
}

static INT32 exam_ctl_ipp_direct_start(UINT32 hdl_idx)
{
	static UINT32 is_opened = FALSE;

	if (is_opened == TRUE) {
		exam_ctl_ipp_release_hdl(hdl_idx);
	}

	if (is_opened == FALSE) {
		exam_ctl_ipp_buf_alloc();
	}

	exam_ctl_ipp_create_hdl(hdl_idx);

	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
		ctl_ipp_ioctl(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDSTART, NULL);
	}
	is_opened = TRUE;
	return 0;
}

static INT32 exam_ctl_ipp_direct_stop(UINT32 hdl_idx)
{
	if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
		ctl_ipp_ioctl(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDSTOP, NULL);
	}
	return 0;
}

static INT32 exam_ctl_ipp_direct_wakeup(UINT32 hdl_addr)
{
	INT32 rt;

	rt = ctl_ipp_ioctl((UINT32) hdl_addr, CTL_IPP_IOCTL_SNDWAKEUP, NULL);
	if(rt != E_OK) {
		DBG_DUMP("wakeup fail %d 0x%x\r\n", rt, hdl_addr);
	}
	return 0;
}

static INT32 exam_ctl_ipp_direct_sleep(UINT32 hdl_addr)
{
	INT32 rt;

	rt = ctl_ipp_ioctl((UINT32) hdl_addr, CTL_IPP_IOCTL_SNDSLEEP, NULL);
	if(rt != E_OK) {
		DBG_DUMP("sleep fail %d 0x%x\r\n", rt, hdl_addr);
	}
	return 0;
}

static INT32 exam_ctl_ipp_direct_cb(UINT32 hdl_idx, UINT32 cnt, UINT32 event)
{
	static VDO_FRAME vdoin_info;
	UINT32 i;

	/* config vdo frame info */
	vdoin_info.sign = MAKEFOURCC('V', 'F', 'R', 'M');
	vdoin_info.pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 1, VDO_PIX_RGGB_R);
	vdoin_info.size.w = g_ctl_ipp_raw_input.width;
	vdoin_info.size.h = g_ctl_ipp_raw_input.height;
	vdoin_info.loff[VDO_PINDEX_RAW] = g_ctl_ipp_raw_input.lofs;
	vdoin_info.addr[VDO_PINDEX_RAW] = (UINT32)in_raw_buf_info.va_addr;
	vdoin_info.addr[1] = (UINT32)&vdoin_info;
	vdoin_info.addr[2] = (UINT32)&vdoin_info;
	vdoin_info.addr[3] = (UINT32)&vdoin_info;
	vdoin_info.reserved[5] = 0;
	vdoin_info.reserved[6] = (vdoin_info.size.w << 16) | vdoin_info.size.h;

	/* load image */
	if (exam_ctl_ipp_load_input_image() != 0) {
		return -1;
	}

	if (event == CTL_IPP_DIRECT_START) {
		if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM) {
			vdoin_info.timestamp = ctl_ipp_util_get_syst_timestamp();

			if (ctl_ipp_direct_flow_cb(event, (void *) &vdoin_info, NULL) != CTL_IPP_E_OK) {
				CTL_IPP_DBG_DUMP("vd callback fail\r\n");
				return -1;
			}
			vos_util_delay_ms(40);

		}
	} else if (event == CTL_IPP_DIRECT_PROCESS) {
		if (hdl_idx < EXAM_CTRL_HDL_MAX_NUM && cnt > 0) {
			for (i = 0; i < cnt; i++) {
				vdoin_info.timestamp = ctl_ipp_util_get_syst_timestamp();

				if (ctl_ipp_direct_flow_cb(event, (void *) &vdoin_info, NULL) != CTL_IPP_E_OK) {
					CTL_IPP_DBG_DUMP("vd callback fail\r\n");
					return -1;
				}
				vos_util_delay_ms(40);
			}
		}
	} else if (event == CTL_IPP_DIRECT_STOP) {
	} else {
		CTL_IPP_DBG_DUMP("vd callback type unknow %d \r\n", (int)(event));
	}

	return 0;
}

static INT32 exam_ctl_ipp_test_r2y(UINT32 hdl_idx, UINT32 evt_cnt)
{
	UINT32 timeout_cnt;

	if (exam_ctl_ipp_buf_alloc() != E_OK) {
		goto CTRL_R2Y_END;
	}

	exam_ctl_ipp_create_hdl(hdl_idx);
	exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);

	proc_end_cnt = 0;
	timeout_cnt = 0;
	while (proc_end_cnt != evt_cnt) {
		vos_util_delay_ms(10);

		timeout_cnt += 1;
		if (timeout_cnt > evt_cnt * 5) {
			CTL_IPP_DBG_DUMP("timeout\r\n");
			break;
		}
	}

	exam_ctl_ipp_saverst(hdl_idx);
	exam_ctl_ipp_release_hdl(hdl_idx);

CTRL_R2Y_END:
	exam_ctl_ipp_buf_release();

	return 0;
}

static void exam_ctl_ipp_test_script_rtc_incrop(UINT32 interval)
{
	UINT32 rt;
	UINT32 apply_id;
	UINT32 i, n;
	CTL_IPP_IN_CROP crop_cfg_dft, crop_cfg;

	CTL_IPP_DBG_DUMP("---- RUNTIME CHANGE CROP TEST ----");
	vos_util_delay_ms(interval);
	rt = ctl_ipp_get(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_IN_CROP, (void *)&crop_cfg_dft);

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		crop_cfg.mode = CTL_IPP_IN_CROP_AUTO;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_IN_CROP, (void *)&crop_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	}
	vos_util_delay_ms(interval);

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		crop_cfg.mode = CTL_IPP_IN_CROP_NONE;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_IN_CROP, (void *)&crop_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	}
	vos_util_delay_ms(interval);

	for (n = 0; n < 16; n++) {
		for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
			crop_cfg.mode = CTL_IPP_IN_CROP_USER;
			crop_cfg.crp_window.w = ALIGN_FLOOR_32((crop_cfg_dft.crp_window.x + crop_cfg_dft.crp_window.w) * ((n % 4) + 1) / 4);
			crop_cfg.crp_window.h = ALIGN_FLOOR_32((crop_cfg_dft.crp_window.y + crop_cfg_dft.crp_window.h) * ((n % 4) + 1) / 4);
			crop_cfg.crp_window.x = (crop_cfg_dft.crp_window.x + crop_cfg_dft.crp_window.w - crop_cfg.crp_window.w) >> 1;
			crop_cfg.crp_window.y = (crop_cfg_dft.crp_window.y + crop_cfg_dft.crp_window.h - crop_cfg.crp_window.h) >> 1;
			rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_IN_CROP, (void *)&crop_cfg);
			rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		}
		vos_util_delay_ms(interval);
	}

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		crop_cfg.mode = CTL_IPP_IN_CROP_NONE;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_IN_CROP, (void *)&crop_cfg_dft);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	}
	vos_util_delay_ms(interval);
}

static void exam_ctl_ipp_test_script_rtc_outpath(UINT32 interval)
{
	#define SIZE_LIST_NUM (7)
	UINT32 rt;
	UINT32 apply_id;
	UINT32 i, n, pid;
	CTL_IPP_OUT_PATH path_cfg;
	USIZE size_list[SIZE_LIST_NUM] = {
		{3840, 2160},
		{2592, 1944},
		{2112, 1584},
		{1920, 1080},
		{1280, 720},
		{640, 480},
		{360, 240},
	};

	CTL_IPP_DBG_DUMP("---- RUNTIME CHANGE OUT_PATH TEST ----\r\n");
	vos_util_delay_ms(interval);
	rt = 0;

	/* compress format test */
	for (n = 0; n < 2; n++) {
		UINT32 idx;

		idx = (n % SIZE_LIST_NUM);
		path_cfg.pid = CTL_IPP_OUT_PATH_ID_1;
		rt |= ctl_ipp_get(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);

		path_cfg.enable = ENABLE;
		path_cfg.size = size_list[idx];
		path_cfg.lofs = path_cfg.size.w;
		path_cfg.crp_window.x = 0;
		path_cfg.crp_window.y = 0;
		path_cfg.crp_window.w = path_cfg.size.w;
		path_cfg.crp_window.h = path_cfg.size.h;
		path_cfg.fmt = VDO_PXLFMT_YUV420_NVX1_H264;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		vos_util_delay_ms(interval);
	}

	for (n = 0; n < 2; n++) {
		UINT32 idx;

		idx = (n % SIZE_LIST_NUM);
		path_cfg.pid = CTL_IPP_OUT_PATH_ID_1;
		rt |= ctl_ipp_get(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);

		path_cfg.enable = ENABLE;
		path_cfg.size = size_list[idx];
		path_cfg.lofs = path_cfg.size.w;
		path_cfg.crp_window.x = 0;
		path_cfg.crp_window.y = 0;
		path_cfg.crp_window.w = path_cfg.size.w;
		path_cfg.crp_window.h = path_cfg.size.h;
		path_cfg.fmt = VDO_PXLFMT_YUV420_NVX1_H265;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		vos_util_delay_ms(interval);
	}

	/* sacle size test */
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		for (n = 0; n < 16; n++) {
			for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
				UINT32 idx;

				idx = (n % SIZE_LIST_NUM);
				path_cfg.pid = pid;
				rt |= ctl_ipp_get(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);

				path_cfg.size = size_list[idx];
				path_cfg.lofs = path_cfg.size.w;
				path_cfg.crp_window.x = 0;
				path_cfg.crp_window.y = 0;
				path_cfg.crp_window.w = path_cfg.size.w;
				path_cfg.crp_window.h = path_cfg.size.h;
				path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
				rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
				rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
			}
			vos_util_delay_ms(interval);
		}

		/* reset to original size */
		for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
			path_cfg.pid = pid;
			rt |= ctl_ipp_get(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);

			path_cfg.enable = ENABLE;
			path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
			path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
			path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
			path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
			path_cfg.crp_window.x = 0;
			path_cfg.crp_window.y = 0;
			path_cfg.crp_window.w = path_cfg.size.w;
			path_cfg.crp_window.h = path_cfg.size.h;

			rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
			rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		}
		vos_util_delay_ms(interval);
	}
	vos_util_delay_ms(interval);
}

static void exam_ctl_ipp_test_script_rtc_func(UINT32 interval)
{
	#define EXAM_FUNC_NUM (4)
	UINT32 func_map[EXAM_FUNC_NUM] = {
		2,	/* DEFOG */
		3,	/* 3DNR */
		4,	/* DATASTAMP */
		5	/* PRIVACY MASK */
	};
	UINT32 rt;
	UINT32 apply_id;
	UINT32 i, j, hdl_idx;
	CTL_IPP_FUNC func_en;

	CTL_IPP_DBG_DUMP("---- RUNTIME CHANGE FUNC_EN TEST ----\r\n");

	hdl_idx = 0;
	rt = 0;
	for (i = 0; i < (1 << EXAM_FUNC_NUM); i++) {
		UINT32 tmp;

		/* test of all func combination with Gray code order */
		tmp = i ^ (i >> 1);
		func_en = 0;
		for (j = 0; j < EXAM_FUNC_NUM; j++) {
			if (tmp & (1 << j)) {
				func_en |= 1 << func_map[j];
			}
		}

		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		vos_util_delay_ms(interval);
	}
}

static void exam_ctl_ipp_test_script_rtc_flush(UINT32 interval)
{
	UINT32 rt;
	UINT32 apply_id;
	UINT32 i, pid;
	CTL_IPP_OUT_PATH path_cfg;
	CTL_IPP_FLUSH_CONFIG flush_cfg;

	rt = E_OK;
	i = 0;
	apply_id = 0;

	/* reset to default */
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		path_cfg.pid = pid;
		path_cfg.enable = g_ctl_ipp_ime_output_enable[pid];
		path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
		path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
		path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
		path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
		path_cfg.crp_window.x = 0;
		path_cfg.crp_window.y = 0;
		path_cfg.crp_window.w = path_cfg.size.w;
		path_cfg.crp_window.h = path_cfg.size.h;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	}
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	vos_util_delay_ms(interval);

	g_ctl_ipp_api_dbg_log = 1;
	pid = CTL_IPP_OUT_PATH_ID_1;

	/* disable and flush */
	CTL_IPP_DBG_DUMP("---- Disable path %d and flush start ----\r\n", (int)(pid));
	path_cfg.pid = pid;
	path_cfg.enable = DISABLE;
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	flush_cfg.pid = pid;
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_FLUSH, (void *)&flush_cfg);
	CTL_IPP_DBG_DUMP("---- Disable path %d and flush end apply_id %d ----\r\n", (int)(pid), (int)(apply_id));

	vos_util_delay_ms(interval);

	/* enable path, and flush again */
	CTL_IPP_DBG_DUMP("---- Enable path path %d start ----\r\n", (int)(pid));
	path_cfg.pid = pid;
	path_cfg.enable = ENABLE;
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	vos_util_delay_ms(interval);

	CTL_IPP_DBG_DUMP("---- Disable path %d and flush start ----\r\n", (int)(pid));
	path_cfg.enable = DISABLE;
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	flush_cfg.pid = pid;
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_FLUSH, (void *)&flush_cfg);
	CTL_IPP_DBG_DUMP("---- Disable path %d and flush end apply_id %d ----\r\n", (int)(pid), (int)(apply_id));

	vos_util_delay_ms(interval);

	/* reset to default */
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		path_cfg.pid = pid;
		path_cfg.enable = g_ctl_ipp_ime_output_enable[pid];
		path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
		path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
		path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
		path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
		path_cfg.crp_window.x = 0;
		path_cfg.crp_window.y = 0;
		path_cfg.crp_window.w = path_cfg.size.w;
		path_cfg.crp_window.h = path_cfg.size.h;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	}
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	vos_util_delay_ms(interval);

	/* flush all */
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		path_cfg.pid = pid;
		path_cfg.enable = DISABLE;
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	}
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	CTL_IPP_DBG_DUMP("---- Flush all start ----\r\n");
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_FLUSH, NULL);
	CTL_IPP_DBG_DUMP("---- Flush all end ----\r\n");

	vos_util_delay_ms(interval);
	g_ctl_ipp_api_dbg_log = 0;

	CTL_IPP_DBG_DUMP("---- FLUSH TEST END, rt 0x%x ----\r\n", (unsigned int)(rt));
}

static void exam_ctl_ipp_test_script_bufio_sequence(void)
{
	UINT32 rt;
	UINT32 apply_id;
	UINT32 i, pid;
	CTL_IPP_OUT_PATH path_cfg;
	//CTL_IPP_FLUSH_CONFIG flush_cfg;

	rt = E_OK;
	i = 0;
	apply_id = 0;

	CTL_IPP_DBG_DUMP("---- bufio test sequence start ----\r\n");

	/* only enable path 1 */
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		if (pid == CTL_IPP_OUT_PATH_ID_1) {
			path_cfg.pid = pid;
			path_cfg.enable = g_ctl_ipp_ime_output_enable[pid];
			path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
			path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
			path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
			path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
			path_cfg.crp_window.x = 0;
			path_cfg.crp_window.y = 0;
			path_cfg.crp_window.w = path_cfg.size.w;
			path_cfg.crp_window.h = path_cfg.size.h;
		} else {
			path_cfg.pid = pid;
			path_cfg.enable = DISABLE;
		}
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
	}
	rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	exam_ctl_ipp_sndevt(0, 3);
	vos_util_delay_ms(500);

	/* enable -> disable -> enable -> disable -> enable, change at each frame */
	pid = CTL_IPP_OUT_PATH_ID_1;
	for (i = 0; i < 5; i++) {
		if (i % 2 == 0) {
			path_cfg.pid = pid;
			path_cfg.enable = DISABLE;
		} else {
			path_cfg.pid = pid;
			path_cfg.enable = g_ctl_ipp_ime_output_enable[pid];
			path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
			path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
			path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
			path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
			path_cfg.crp_window.x = 0;
			path_cfg.crp_window.y = 0;
			path_cfg.crp_window.w = path_cfg.size.w;
			path_cfg.crp_window.h = path_cfg.size.h;
		}
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		CTL_IPP_DBG_DUMP("set path 1 enable = %d, apply id %d\r\n", (int)(path_cfg.enable), (int)(apply_id));

		exam_ctl_ipp_sndevt(0, 1);

		/* wait proc end */
		vos_util_delay_ms(100);
	}
	exam_ctl_ipp_sndevt(0, 1);
	exam_ctl_ipp_sndevt(0, 1);

	/* enable -> disable -> enable -> disable -> enable, change at two frame */
	pid = CTL_IPP_OUT_PATH_ID_1;
	for (i = 0; i < 5; i++) {
		if (i % 2 == 0) {
			path_cfg.pid = pid;
			path_cfg.enable = DISABLE;
		} else {
			path_cfg.pid = pid;
			path_cfg.enable = g_ctl_ipp_ime_output_enable[pid];
			path_cfg.fmt = g_ctl_ipp_ime_output[pid].fmt;
			path_cfg.lofs = g_ctl_ipp_ime_output[pid].lofs;
			path_cfg.size.w = g_ctl_ipp_ime_output[pid].width;
			path_cfg.size.h = g_ctl_ipp_ime_output[pid].height;
			path_cfg.crp_window.x = 0;
			path_cfg.crp_window.y = 0;
			path_cfg.crp_window.w = path_cfg.size.w;
			path_cfg.crp_window.h = path_cfg.size.h;
		}
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_OUT_PATH, (void *)&path_cfg);
		rt |= ctl_ipp_set(g_ctl_ipp_hdl_list[0], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		CTL_IPP_DBG_DUMP("set path 1 enable = %d, apply id %d\r\n", (int)(path_cfg.enable), (int)(apply_id));

		exam_ctl_ipp_sndevt(0, 2);

		/* wait proc end */
		vos_util_delay_ms(200);
	}
	exam_ctl_ipp_sndevt(0, 1);
	exam_ctl_ipp_sndevt(0, 1);

	CTL_IPP_DBG_DUMP("---- bufio test sequence end ----\r\n");
}

static INT32 exam_ctl_ipp_test_script(void)
{
	UINT32 i, time_out_cnt;

	/* ipl ctrl layer, test script */
	if (exam_ctl_ipp_buf_alloc() != E_OK) {
		goto CTRL_TEST_SCRIPT_END;
	}
	exam_ctl_ipp_tsk_pause(1, 0);

	/* open/close handle */
	CTL_IPP_DBG_DUMP("---- OPEN/CLOSE TEST ----\r\n");
	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_create_hdl(i);
	}

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_release_hdl(i);
	}

	/* single handle trigger */
	CTL_IPP_DBG_DUMP("---- SINGLE HANDLE SNDEVT TEST ----\r\n");
	proc_end_cnt = 0;
	time_out_cnt = 0;

	exam_ctl_ipp_create_hdl(0);
	exam_ctl_ipp_sndevt(0, 3);
	exam_ctl_ipp_tsk_resume(0);
	exam_ctl_ipp_sndevt(0, 3);

	while (1) {
		if (proc_end_cnt == 6) {
			break;
		}

		vos_util_delay_ms(10);
		time_out_cnt += 1;

		if (time_out_cnt > 4 * 6) {
			CTL_IPP_DBG_DUMP("Fail\r\n");
			return FALSE;
		}
	}

	exam_ctl_ipp_release_hdl(0);
	exam_ctl_ipp_tsk_pause(1, 0);

	/* multi handle trigger */
	CTL_IPP_DBG_DUMP("---- MULTI HANDLE SNDEVT TEST ----\r\n");
	proc_end_cnt = 0;
	time_out_cnt = 0;
	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_create_hdl(i);
	}

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_sndevt(i, 3);
	}

	exam_ctl_ipp_tsk_resume(0);

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_sndevt(i, 3);
	}

	while (1) {
		if (proc_end_cnt == 6 * EXAM_CTRL_HDL_MAX_NUM) {
			break;
		}

		vos_util_delay_ms(10);
		time_out_cnt += 1;

		if (time_out_cnt > 4 * 6 * EXAM_CTRL_HDL_MAX_NUM) {
			CTL_IPP_DBG_DUMP("Fail\r\n");
			return FALSE;
		}
	}

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		exam_ctl_ipp_release_hdl(i);
	}
	exam_ctl_ipp_tsk_pause(1, 0);

	/* drop test */
	CTL_IPP_DBG_DUMP("---- DROP FRAME TEST ----\r\n");
	drop_cnt = 0;
	time_out_cnt = 0;

	exam_ctl_ipp_create_hdl(0);
	exam_ctl_ipp_sndevt(0, 6);
	exam_ctl_ipp_release_hdl(0);
	exam_ctl_ipp_tsk_resume(0);

	while (1) {
		if (drop_cnt == 6) {
			break;
		}

		vos_util_delay_ms(10);
		time_out_cnt += 1;

		if (time_out_cnt > 4 * 6) {
			CTL_IPP_DBG_DUMP("Fail\r\n");
			return FALSE;
		}
	}

	/* runtime change test
		enable auto send event timer then runtime change ipp config
	*/
	CTL_IPP_DBG_DUMP("---- RUNTIME CHANGE TEST ----");
	exam_ctl_ipp_tsk_resume(0);
	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		UINT32 apply_id;
		CTL_IPP_FUNC func;

		/* create handle and start sndevt timer */
		exam_ctl_ipp_create_hdl(i);
		exam_ctl_ipp_sndevt_timer_op(1, i, 33000);

		/* disable 3dnr due to yuv ring buffer not protected */
		ctl_ipp_get(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_FUNCEN, (void *)&func);
		func &= ~(CTL_IPP_FUNC_3DNR);
		ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_FUNCEN, (void *)&func);
		ctl_ipp_set(g_ctl_ipp_hdl_list[i], CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	}

	exam_ctl_ipp_test_script_rtc_incrop(400);
	exam_ctl_ipp_test_script_rtc_outpath(400);
	exam_ctl_ipp_test_script_rtc_func(1000);

	ctl_ipp_dbg_hdl_dump_all(ctl_ipp_int_printf);

	for (i = 0; i < EXAM_CTRL_HDL_MAX_NUM; i++) {
		/* stop sndevt timer and release handle */
		exam_ctl_ipp_sndevt_timer_op(0, i, 33000);
		exam_ctl_ipp_release_hdl(i);
	}

	CTL_IPP_DBG_DUMP("^---- TEST END ----\r\n");

CTRL_TEST_SCRIPT_END:
	exam_ctl_ipp_buf_release();
	return 0;

}

static INT32 exam_ctl_ipp_test_region(void)
{
	CTL_IPP_REG_CB_INFO cb_info = {0};
	CTL_IPP_OUT_PATH ime_out;
	CTL_IPP_OUT_PATH_REGION region;
	CTL_IPP_ALGID algid;
	CTL_IPP_IN_CROP rhe_crop;
	CTL_IPP_SCL_METHOD_SEL scl_method_sel;
	CTL_IPP_FUNC func_en;
	UINT32 reference_path;
	CTL_IPP_OUT_PATH_MD md_info;
	CHAR hdl_name[16];
	UINT32 i, i_region, hdl_idx = 0, evt_cnt = 1, timeout_cnt, apply_id;
	typedef struct {
		struct {
			URECT win;
		} path[CTL_IPP_OUT_PATH_ID_MAX];
	} EXAM_CTL_IPP_REGION;
	EXAM_CTL_IPP_REGION region_tbl[] = {
	//    p1	x			y			w			h			p2	x			y			w			h			p3		p4		p5	x			y			w			h
		{{{{	0			,0			,1920/2		,1080/2}}	,{{	0			,0			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},
		{{{{	1920/2		,1080/2		,1920/2		,1080/2}}	,{{	1280/2		,720/2		,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},
		{{{{	1920/4		,1080/4		,1920/2		,1080/2}}	,{{	1280/4		,720/4		,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},
		{{{{	1920/2		,1080/4		,1920/2		,1080/2}}	,{{	1280/2		,720/4		,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},
		{{{{	100			,200		,1280		,720}}		,{{	40			,88			,720		,480}}		,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},
	};
	EXAM_CTL_IPP_REGION region_tbl_err[] = {
	//    p1	x			y			w			h			p2	x			y			w			h			p3		p4		p5	x			y			w			h
		{{{{	1920/2		,0			,1920		,1080}}		,{{	0			,0			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p1 x out of boundary
		{{{{	0			,1080/2		,1920		,1080}}		,{{	0			,0			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p1 y out of boundary
		{{{{	0			,0			,1920/2		,1080/2}}	,{{	1280/2		,720/2		,1280		,720}}		,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p2 out of boundary
		{{{{	101			,0			,1920/2		,1080/2}}	,{{	0			,0			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p1 ofs_x 2x alignment
		{{{{	10			,101		,1920/2		,1080/2}}	,{{	0			,0			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p1 ofs_y 2x alignment
		{{{{	0			,0			,1920/2		,1080/2}}	,{{	21			,51			,1280/2		,720/2}}	,{{0}}	,{{0}}	,{{	0			,0			,1920		,1080}}}},	// p2 ofs 2x alignment
	};

	CTL_IPP_DBG_DUMP("---- REGION TEST ----\r\n");

	g_ctl_ipp_api_dbg_log = 1;
	g_ctl_ipp_clr_buf = 1;

	if (exam_ctl_ipp_buf_alloc() != E_OK) {
		goto CTRL_TEST_REGION_END;
	}

	// create handle
	if (g_ctl_ipp_hdl_list[hdl_idx] != 0) {
		CTL_IPP_DBG_DUMP("hdl_list[%d] 0x%.8x != 0\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);
		return -1;
	}

	snprintf(hdl_name, 16, "exam_hdl_%d", (int)hdl_idx);
	g_ctl_ipp_hdl_list[hdl_idx] = ctl_ipp_open(hdl_name, g_ctl_ipp_flow_sel[hdl_idx]);
	if (g_ctl_ipp_hdl_list[hdl_idx] == 0) {
		CTL_IPP_DBG_DUMP("Open ctrl hdl fail\r\n");
		return -1;
	}

	cb_info.cbevt = CTL_IPP_CBEVT_IN_BUF;
	cb_info.fp = exam_ctl_ipp_in_buf_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_ENG_IME_ISR;
	cb_info.fp = exam_ctl_ipp_ime_isr_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_OUT_BUF;
	cb_info.fp = exam_ctl_ipp_bufio_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_DATASTAMP;
	cb_info.fp = exam_ctl_ipp_datastamp_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_PRIMASK;
	cb_info.fp = exam_ctl_ipp_primask_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	/* config setting */
	//exam_ctl_ipp_config(g_ctl_ipp_hdl_list[hdl_idx], hdl_idx);
	algid.type = CTL_IPP_ALGID_IQ;
	algid.id = hdl_idx;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_ALGID_IMM, (void *)&algid);

	/* IME Output scale method select */
	scl_method_sel.scl_th = 0x00010001;
	scl_method_sel.method_l = CTL_IPP_SCL_AUTO;
	scl_method_sel.method_h = CTL_IPP_SCL_AUTO;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_SCL_METHOD, (void *)&scl_method_sel);

	/* private buf calculate */
	{
		CTL_IPP_PRIVATE_BUF buf_info;
		CTL_IPP_BUFCFG buf_cfg;

		memset((void *)&buf_info, 0, sizeof(CTL_IPP_PRIVATE_BUF));
		buf_info.func_en = CTL_IPP_FUNC_3DNR;
		buf_info.max_size.w = g_ctl_ipp_raw_input.width;
		buf_info.max_size.h = g_ctl_ipp_raw_input.height;
		buf_info.pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 1, VDO_PIX_RGGB_R);
		ctl_ipp_get(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_BUFQUY, (void *)&buf_info);

		if (buf_info.buf_size > 0) {
			buf_cfg.start_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(buf_info.buf_size);
			buf_cfg.size = buf_info.buf_size;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_BUFCFG, (void *)&buf_cfg);
		}
	}

	/* Crop size reference input header */
	rhe_crop.mode = CTL_IPP_IN_CROP_USER;//CTL_IPP_IN_CROP_NONE;
	rhe_crop.crp_window.x = 0;
	rhe_crop.crp_window.y = 0;
	rhe_crop.crp_window.w = 1920;
	rhe_crop.crp_window.h = 1080;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_IN_CROP, (void *)&rhe_crop);

	/* IME output enable */
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_1] = 1;
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_2] = 1;
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_3] = 0;
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_4] = 0;
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_5] = 1;
	g_ctl_ipp_ime_output_enable[CTL_IPP_OUT_PATH_ID_6] = 0;

	/* Region test case */
	{
		CTL_IPP_DBG_DUMP("---- BASIC TEST ----\r\n");
		for (i_region = 0; i_region < sizeof(region_tbl) / sizeof(typeof(region_tbl[0])); i_region++) {
			CTL_IPP_DBG_DUMP("Region test %d / %d\r\n", (int)i_region+1, (int)(sizeof(region_tbl) / sizeof(typeof(region_tbl[0]))));
			/* IME Output path */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				ime_out.pid = i;
				ime_out.enable = g_ctl_ipp_ime_output_enable[i];
				ime_out.fmt = g_ctl_ipp_ime_output[i].fmt;
				ime_out.lofs = (i == CTL_IPP_OUT_PATH_ID_5) ? g_ctl_ipp_ime_output[i].lofs : 0x100; // don't care, test ignore path lofs
				ime_out.size.w = region_tbl[i_region].path[i].win.w;
				ime_out.size.h = region_tbl[i_region].path[i].win.h;
				ime_out.crp_window.x = 0;
				ime_out.crp_window.y = 0;
				ime_out.crp_window.w = ime_out.size.w;
				ime_out.crp_window.h = ime_out.size.h;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);

				region.pid = ime_out.pid;
				region.enable = (i == CTL_IPP_OUT_PATH_ID_5) ? DISABLE : ime_out.enable; // ref path cant use region
				region.bgn_lofs = g_ctl_ipp_ime_output[i].lofs;
				region.bgn_size.w = g_ctl_ipp_ime_output[i].width;
				region.bgn_size.h = g_ctl_ipp_ime_output[i].height;
				region.region_ofs.x = region_tbl[i_region].path[i].win.x;
				region.region_ofs.y = region_tbl[i_region].path[i].win.y;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);

				md_info.pid = i;
				md_info.enable = 0;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_info);
			}

			/* Function enable */
			func_en = CTL_IPP_FUNC_3DNR;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

			/* 3DNR ref path */
			reference_path = CTL_IPP_OUT_PATH_ID_5;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&reference_path);

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			CTL_IPP_DBG_DUMP("Open ctrl hdl to idx %d success 0x%.8x\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);

			exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);

			proc_end_cnt = 0;
			timeout_cnt = 0;
			while (proc_end_cnt != evt_cnt) {
				vos_util_delay_ms(10);

				timeout_cnt += 1;
				if (timeout_cnt > evt_cnt * 5) {
					CTL_IPP_DBG_DUMP("timeout\r\n");
					break;
				}
			}

			exam_ctl_ipp_saverst(hdl_idx);
		}
	}

	/* Region size error handling test case */
	{
		CTL_IPP_DBG_DUMP("---- SIZE ERROR TEST ----\r\n");
		for (i_region = 0; i_region < sizeof(region_tbl_err) / sizeof(typeof(region_tbl_err[0])); i_region++) {
			CTL_IPP_DBG_DUMP("Region error test %d / %d\r\n", (int)i_region+1, (int)(sizeof(region_tbl_err) / sizeof(typeof(region_tbl_err[0]))));
			/* IME Output path */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				ime_out.pid = i;
				ime_out.enable = g_ctl_ipp_ime_output_enable[i];
				ime_out.fmt = g_ctl_ipp_ime_output[i].fmt;
				ime_out.lofs = (i == CTL_IPP_OUT_PATH_ID_5) ? g_ctl_ipp_ime_output[i].lofs : 0x100; // don't care, test ignore path lofs
				ime_out.size.w = region_tbl_err[i_region].path[i].win.w;
				ime_out.size.h = region_tbl_err[i_region].path[i].win.h;
				ime_out.crp_window.x = 0;
				ime_out.crp_window.y = 0;
				ime_out.crp_window.w = ime_out.size.w;
				ime_out.crp_window.h = ime_out.size.h;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);

				region.pid = ime_out.pid;
				region.enable = (i == CTL_IPP_OUT_PATH_ID_5) ? DISABLE : ime_out.enable; // ref path cant use region
				region.bgn_lofs = g_ctl_ipp_ime_output[i].lofs;
				region.bgn_size.w = g_ctl_ipp_ime_output[i].width;
				region.bgn_size.h = g_ctl_ipp_ime_output[i].height;
				region.region_ofs.x = region_tbl_err[i_region].path[i].win.x;
				region.region_ofs.y = region_tbl_err[i_region].path[i].win.y;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);

				md_info.pid = i;
				md_info.enable = 0;
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_info);
			}

			/* Function enable */
			func_en = CTL_IPP_FUNC_3DNR;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

			/* 3DNR ref path */
			reference_path = CTL_IPP_OUT_PATH_ID_5;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&reference_path);

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			CTL_IPP_DBG_DUMP("Open ctrl hdl to idx %d success 0x%.8x\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);

			exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);

			proc_end_cnt = 0;
			timeout_cnt = 0;
			while (proc_end_cnt != evt_cnt) {
				vos_util_delay_ms(10);

				timeout_cnt += 1;
				if (timeout_cnt > evt_cnt * 5) {
					CTL_IPP_DBG_DUMP("timeout\r\n");
					break;
				}
			}
		}
	}

	/* Region 3dnr error handling test case */
	{
		CTL_IPP_DBG_DUMP("---- 3DNR ERROR TEST ----\r\n");
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			ime_out.pid = i;
			ime_out.enable = g_ctl_ipp_ime_output_enable[i];
			ime_out.fmt = g_ctl_ipp_ime_output[i].fmt;
			ime_out.lofs = 0x100; // don't care, test ignore path lofs
			ime_out.size.w = region_tbl[0].path[i].win.w;
			ime_out.size.h = region_tbl[0].path[i].win.h;
			ime_out.crp_window.x = 0;
			ime_out.crp_window.y = 0;
			ime_out.crp_window.w = ime_out.size.w;
			ime_out.crp_window.h = ime_out.size.h;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);

			region.pid = ime_out.pid;
			region.enable = ime_out.enable; // test ref path use regions error handle
			region.bgn_lofs = g_ctl_ipp_ime_output[i].lofs;
			region.bgn_size.w = g_ctl_ipp_ime_output[i].width;
			region.bgn_size.h = g_ctl_ipp_ime_output[i].height;
			region.region_ofs.x = region_tbl[0].path[i].win.x;
			region.region_ofs.y = region_tbl[0].path[i].win.y;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);

			md_info.pid = i;
			md_info.enable = 0;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_info);
		}

		/* Function enable */
		func_en = CTL_IPP_FUNC_3DNR;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

		/* 3DNR ref path */
		reference_path = CTL_IPP_OUT_PATH_ID_5;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&reference_path);

		/* set apply */
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

		CTL_IPP_DBG_DUMP("Open ctrl hdl to idx %d success 0x%.8x\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);

		exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);

		proc_end_cnt = 0;
		timeout_cnt = 0;
		while (proc_end_cnt != evt_cnt) {
			vos_util_delay_ms(10);

			timeout_cnt += 1;
			if (timeout_cnt > evt_cnt * 5) {
				CTL_IPP_DBG_DUMP("timeout\r\n");
				break;
			}
		}
	}

	/* Region md error handling test case */
	{
		CTL_IPP_DBG_DUMP("---- MD ERROR TEST ----\r\n");
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			ime_out.pid = i;
			ime_out.enable = g_ctl_ipp_ime_output_enable[i];
			ime_out.fmt = g_ctl_ipp_ime_output[i].fmt;
			ime_out.lofs = 0x100; // don't care, test ignore path lofs
			ime_out.size.w = region_tbl[0].path[i].win.w;
			ime_out.size.h = region_tbl[0].path[i].win.h;
			ime_out.crp_window.x = 0;
			ime_out.crp_window.y = 0;
			ime_out.crp_window.w = ime_out.size.w;
			ime_out.crp_window.h = ime_out.size.h;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);

			region.pid = ime_out.pid;
			region.enable = ime_out.enable;
			region.bgn_lofs = g_ctl_ipp_ime_output[i].lofs;
			region.bgn_size.w = g_ctl_ipp_ime_output[i].width;
			region.bgn_size.h = g_ctl_ipp_ime_output[i].height;
			region.region_ofs.x = region_tbl[0].path[i].win.x;
			region.region_ofs.y = region_tbl[0].path[i].win.y;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);

			md_info.pid = i;
			md_info.enable = 1;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_info);
		}

		/* Function enable */
		func_en = CTL_IPP_FUNC_NONE;
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

		/* set apply */
		ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

		CTL_IPP_DBG_DUMP("Open ctrl hdl to idx %d success 0x%.8x\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);

		exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);

		proc_end_cnt = 0;
		timeout_cnt = 0;
		while (proc_end_cnt != evt_cnt) {
			vos_util_delay_ms(10);

			timeout_cnt += 1;
			if (timeout_cnt > evt_cnt * 5) {
				CTL_IPP_DBG_DUMP("timeout\r\n");
				break;
			}
		}
	}

	//exam_ctl_ipp_release_hdl(hdl_idx);

CTRL_TEST_REGION_END:
	//exam_ctl_ipp_buf_release();
	g_ctl_ipp_api_dbg_log = 0;
	g_ctl_ipp_clr_buf = 0;

	CTL_IPP_DBG_DUMP("---- REGION TEST DONE ----\r\n");

	return 0;
}

static INT32 exam_ctl_ipp_test_pattern_paste_stamp_alloc(UINT32 width, UINT32 height, UINT32 lofs, UINT32 *stamp_buf_addr, UINT32 *stamp_img_addr)
{
	UINT32 stamp_size, stamp_buf_lofs;
	UINT32 i, j;

	/* make stamp image */
	// prevent buffer underflow when pat_lofs < pat_width
	stamp_buf_lofs = (lofs >= width) ? lofs : width;
	stamp_size = ALIGN_CEIL(((sizeof(UINT16) * stamp_buf_lofs * height) + VOS_ALIGN_BYTES), VOS_ALIGN_BYTES);
	(*stamp_buf_addr) = (UINT32)ctl_ipp_util_os_malloc_wrap(stamp_size);
	if ((*stamp_buf_addr) == 0) {
		CTL_IPP_DBG_ERR("data stamp buffer alloc failed\r\n");
		return -1;
	}

	(*stamp_img_addr) = ALIGN_CEIL((*stamp_buf_addr), VOS_ALIGN_BYTES);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			((UINT16*)(*stamp_img_addr))[i*stamp_buf_lofs + j] =
				((((1<<5)-1) * (i+j) / (height+width+1) / 1) << 11) +
				((((1<<6)-1) * (i+j) / (height+width+1) / 2) << 5) +
				((((1<<5)-1) * (i+j) / (height+width+1) / 3) << 0);
		}
		for (j = width; j < stamp_buf_lofs; j++) {
			((UINT16*)(*stamp_img_addr))[i*stamp_buf_lofs + j] = 0xFFF0;
		}
	}
	vos_cpu_dcache_sync(*stamp_buf_addr, stamp_size, VOS_DMA_TO_DEVICE);

	return 0;
}

static void exam_ctl_ipp_test_pattern_paste_stamp_release(UINT32 stamp_buf_addr)
{
	ctl_ipp_util_os_mfree_wrap((void *)stamp_buf_addr);
}

static void exam_ctl_ipp_test_pattern_paste_set_pat_param(UINT32 hdl, UINT32 *bgn_color, URECT pat_win, USIZE pat_size, UINT32 pat_lofs, UINT32 stamp_img_addr)
{
	CTL_IPP_PATTERN_PASTE_INFO pat_paste;

	/* set pattern paste parameter */
	memset((void *)&pat_paste, 0, sizeof(CTL_IPP_PATTERN_PASTE_INFO));
	pat_paste.bgn_color[0] = bgn_color[0];
	pat_paste.bgn_color[1] = bgn_color[1];
	pat_paste.bgn_color[2] = bgn_color[2];
	pat_paste.pat_win = pat_win;
	pat_paste.pat_info.func_en = 1;
	pat_paste.pat_info.img_info.size = pat_size;
	pat_paste.pat_info.img_info.fmt = VDO_PXLFMT_RGB565;
	pat_paste.pat_info.img_info.pos.x = 123; // don't care
	pat_paste.pat_info.img_info.pos.y = 123; // don't care
	pat_paste.pat_info.img_info.lofs = pat_lofs << 1; // might < width
	pat_paste.pat_info.img_info.addr = stamp_img_addr;
	pat_paste.pat_info.iq_info.color_key_en = DISABLE;
	pat_paste.pat_info.iq_info.color_key_val = 0;
	pat_paste.pat_info.iq_info.bld_wt_0 = 0xf;
	pat_paste.pat_info.iq_info.bld_wt_1 = 0xf;
	pat_paste.pat_cst_info.func_en = ENABLE;
	pat_paste.pat_cst_info.auto_mode_en = ENABLE;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_PATTERN_PASTE, (void *)&pat_paste);
}

static void exam_ctl_ipp_test_pattern_paste_set_out_path(UINT32 hdl, UINT32 pid, UINT32 path_en, UINT32 fmt, UINT32 w, UINT32 h, UINT32 lofs)
{
	CTL_IPP_OUT_PATH ime_out;

	ime_out.pid = pid;
	ime_out.enable = path_en;
	ime_out.fmt = fmt;
	ime_out.lofs = lofs;
	ime_out.size.w = w;
	ime_out.size.h = h;
	ime_out.crp_window.x = 0;
	ime_out.crp_window.y = 0;
	ime_out.crp_window.w = ime_out.size.w;
	ime_out.crp_window.h = ime_out.size.h;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);
}

static void exam_ctl_ipp_test_pattern_paste_set_out_path_region(UINT32 hdl, UINT32 pid, UINT32 en, UINT32 w, UINT32 h, UINT32 lofs, UINT32 x, UINT32 y)
{
	CTL_IPP_OUT_PATH_REGION region;

	region.pid = pid;
	region.enable = en;
	region.bgn_lofs = lofs;
	region.bgn_size.w = w;
	region.bgn_size.h = h;
	region.region_ofs.x = x;
	region.region_ofs.y = y;
	ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
}

static void exam_ctl_ipp_test_pattern_paste_trig(void)
{
	UINT32 timeout_cnt, evt_cnt = 1;

	proc_end_cnt = 0;
	timeout_cnt = 0;
	while (proc_end_cnt != evt_cnt) {
		vos_util_delay_ms(30);

		timeout_cnt += 1;
		if (timeout_cnt > evt_cnt * 5) {
			CTL_IPP_DBG_DUMP("timeout\r\n");
			break;
		}
	}
	vos_util_delay_ms(300); // wait buf task push buf
}

static INT32 exam_ctl_ipp_test_pattern_paste(UINT32 test_case)
{
	CTL_IPP_REG_CB_INFO cb_info = {0};
	CTL_IPP_ALGID algid;
	CTL_IPP_IN_CROP rhe_crop;
	CTL_IPP_SCL_METHOD_SEL scl_method_sel;
	CTL_IPP_FUNC func_en;
	CHAR hdl_name[16];
	UINT32 i, i_pattern_paste, i_region, hdl_idx = 0, evt_cnt = 1, apply_id;
	UINT32 stamp_buf_addr, stamp_img_addr;
	UINT32 vdo_frame_w, vdo_frame_h;
	void *in_addr;

#define EXAM_CTL_IPP_TEST_PATTERN_PASTE_IN_LOFS 3840
	typedef struct {
		UINT32 bgn_color[3];
		URECT pat_win;
		USIZE pat_size;
		UINT32 pat_lofs;
	} EXAM_CTL_IPP_PATTERN_PASTE;
	typedef struct {
		struct {
			URECT win;
		} path[CTL_IPP_OUT_PATH_ID_MAX];
	} EXAM_CTL_IPP_REGION;
	EXAM_CTL_IPP_PATTERN_PASTE pattern_paste_tbl[] = {
	//		r	g		b			x	y		w		h			w	h		lofs
		{{	127	,255	,127}	,{	0	,0		,50		,50}	,{	100	,100}	,100},
		{{	127	,255	,127}	,{	0	,50		,50		,50}	,{	100	,100}	,100},
		{{	127	,127	,127}	,{	50	,0		,50		,50}	,{	100	,100}	,100},
		{{	127	,127	,127}	,{	50	,50		,50		,50}	,{	100	,100}	,100},
		{{	127	,127	,127}	,{	25	,25		,50		,50}	,{	100	,100}	,100},
		{{	127	,255	,127}	,{	13	,27		,56		,37}	,{	124	,456}	,124},
		{{	127	,255	,127}	,{	0	,0		,100	,100}	,{	500	,300}	,500},
		{{	127	,255	,127}	,{	0	,0		,99		,99}	,{	500	,300}	,500},
		{{	127	,255	,127}	,{	1	,1		,99		,99}	,{	500	,300}	,500},
		{{	127	,255	,127}	,{	99	,99		,1		,1}		,{	30	,30}	,32},
		{{	127	,127	,127}	,{	25	,25		,50		,50}	,{	100	,100}	,112},
	};
	EXAM_CTL_IPP_REGION region_tbl[] = {
	//    p1	x			y			w			h			p2	x			y			w			h
		{{{{	1920/2		,1080/4		,1920/2		,1080/2}}	,{{	1280/2		,720/4		,1280/2		,720/2}}}},
	};
	EXAM_CTL_IPP_PATTERN_PASTE pattern_paste_tbl_err[] = {
	//		r	g		b			x	y		w		h			w	h		lofs
		{{	127	,255	,127}	,{	110	,0		,0		,0}		,{	100	,100}	,100}, // x% > 100%
		{{	127	,255	,127}	,{	0	,110	,0		,0}		,{	100	,100}	,100}, // y% > 100%
		{{	127	,255	,127}	,{	50	,0		,70		,70}	,{	100	,100}	,100}, // x% + w% > 100%
		{{	127	,255	,127}	,{	0	,50		,70		,70}	,{	100	,100}	,100}, // y% + h% > 100%
		{{	127	,255	,127}	,{	50	,50		,0		,50}	,{	100	,100}	,100}, // w% = 0%
		{{	127	,255	,127}	,{	50	,50		,50		,0}		,{	100	,100}	,100}, // h% = 0%
		{{	127	,255	,127}	,{	50	,50		,50		,50}	,{	0	,100}	,100}, // w = 0
		{{	127	,255	,127}	,{	50	,50		,50		,50}	,{	100	,0}		,100}, // h = 0
		{{	127	,127	,127}	,{	25	,25		,50		,50}	,{	100	,100}	,0},   // lofs = 0
	};
	EXAM_CTL_IPP_PATTERN_PASTE pattern_paste_tbl_err_lofs[] = {
	//		r	g		b			x	y		w		h			w	h		lofs
		{{	127	,255	,127}	,{	99	,99		,1		,1}		,{	50	,30}	,100}, // vdo_frame_lofs < w
	};
	enum {
		EXAM_CTL_IPP_TEST_PATTERN_PASTE_BASIC = 0,
		EXAM_CTL_IPP_TEST_PATTERN_PASTE_WITH_REGION,
		EXAM_CTL_IPP_TEST_PATTERN_PASTE_NORMAL_FLOW_SWITCH,
		EXAM_CTL_IPP_TEST_PATTERN_PASTE_WITH_REGION_AND_NORMAL_FLOW_SWITCH,
		EXAM_CTL_IPP_TEST_PATTERN_PASTE_ERROR_HANDLE,
	};

	CTL_IPP_DBG_DUMP("---- PATTERN PASTE TEST ----\r\n");

	g_ctl_ipp_api_dbg_log = 1;
	g_ctl_ipp_clr_buf = 1;

	if (exam_ctl_ipp_buf_alloc() != E_OK) {
		goto CTRL_TEST_PATTERN_PASTE_END;
	}

	// allocate one line of input
	{
		NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('i', 'p', 'p', 't', 'e', 's', 't', 0);
		NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
		NVTMPP_DDR     ddr = NVTMPP_DDR_1;
		NVTMPP_VB_BLK  blk;

		// get one line buffer
		blk = nvtmpp_vb_get_block(module, pool, EXAM_CTL_IPP_TEST_PATTERN_PASTE_IN_LOFS, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			CTL_IPP_DBG_DUMP("get input buffer fail\n");
			nvtmpp_vb_exit();
			goto CTRL_TEST_PATTERN_PASTE_END;
		}
		in_addr = (void *)nvtmpp_vb_blk2va(blk);
		CTL_IPP_DBG_TRC("input raw buffer addr 0x%.8x\r\n", (unsigned int)in_addr);
	}

	// create handle
	if (g_ctl_ipp_hdl_list[hdl_idx] != 0) {
		CTL_IPP_DBG_DUMP("hdl_list[%d] 0x%.8x != 0\r\n", (int)hdl_idx, (unsigned int)g_ctl_ipp_hdl_list[hdl_idx]);
		return -1;
	}

	snprintf(hdl_name, 16, "exam_hdl_%d", (int)hdl_idx);
	g_ctl_ipp_hdl_list[hdl_idx] = ctl_ipp_open(hdl_name, g_ctl_ipp_flow_sel[hdl_idx]);
	if (g_ctl_ipp_hdl_list[hdl_idx] == 0) {
		CTL_IPP_DBG_DUMP("Open ctrl hdl fail\r\n");
		return -1;
	}

	cb_info.cbevt = CTL_IPP_CBEVT_IN_BUF;
	cb_info.fp = exam_ctl_ipp_in_buf_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_ENG_IME_ISR;
	cb_info.fp = exam_ctl_ipp_ime_isr_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_OUT_BUF;
	cb_info.fp = exam_ctl_ipp_bufio_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_DATASTAMP;
	cb_info.fp = exam_ctl_ipp_datastamp_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_IPP_CBEVT_PRIMASK;
	cb_info.fp = exam_ctl_ipp_primask_cb;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

	/* config setting */
	//exam_ctl_ipp_config(g_ctl_ipp_hdl_list[hdl_idx], hdl_idx);
	algid.type = CTL_IPP_ALGID_IQ;
	algid.id = hdl_idx;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_ALGID_IMM, (void *)&algid);

	/* IME Output scale method select */
	scl_method_sel.scl_th = 0x00010001;
	scl_method_sel.method_l = CTL_IPP_SCL_AUTO;
	scl_method_sel.method_h = CTL_IPP_SCL_AUTO;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_SCL_METHOD, (void *)&scl_method_sel);

	/* private buf calculate */
	{
		CTL_IPP_PRIVATE_BUF buf_info;
		CTL_IPP_BUFCFG buf_cfg;

		memset((void *)&buf_info, 0, sizeof(CTL_IPP_PRIVATE_BUF));
		buf_info.func_en = 0;
		buf_info.max_size.w = g_ctl_ipp_raw_input.width;
		buf_info.max_size.h = g_ctl_ipp_raw_input.height;
		buf_info.pxlfmt = VDO_PXLFMT_MAKE_RAW(g_ctl_ipp_raw_input.fmt, 1, VDO_PIX_RGGB_R);
		ctl_ipp_get(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_BUFQUY, (void *)&buf_info);

		if (buf_info.buf_size > 0) {
			buf_cfg.start_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(buf_info.buf_size);
			buf_cfg.size = buf_info.buf_size;
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_BUFCFG, (void *)&buf_cfg);
		}
	}

	/* Crop size reference input header */
	rhe_crop.mode = CTL_IPP_IN_CROP_USER;//CTL_IPP_IN_CROP_NONE;
	rhe_crop.crp_window.x = 0;
	rhe_crop.crp_window.y = 0;
	rhe_crop.crp_window.w = 1920;
	rhe_crop.crp_window.h = 1080;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_IN_CROP, (void *)&rhe_crop);

	/* Function enable */
	func_en = CTL_IPP_FUNC_NONE;
	ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_FUNCEN, (void *)&func_en);

	/* Pattern paste test case */
	if (test_case & (1<<EXAM_CTL_IPP_TEST_PATTERN_PASTE_BASIC)) {
		CTL_IPP_DBG_DUMP("---- BASIC TEST ----\r\n");
		for (i_pattern_paste = 0; i_pattern_paste < sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0])); i_pattern_paste++) {
			CTL_IPP_DBG_DUMP("\r\nPattern paste test %d / %d (buf_id %d)\r\n",
				(int)i_pattern_paste+1, (int)(sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0]))), (int)g_ctl_ipp_last_push_buf[0].buf_id+1);

			if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl[i_pattern_paste].pat_size.w, pattern_paste_tbl[i_pattern_paste].pat_size.h,
				pattern_paste_tbl[i_pattern_paste].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
				return -1;
			}

			exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl[i_pattern_paste].bgn_color[0],
				pattern_paste_tbl[i_pattern_paste].pat_win, pattern_paste_tbl[i_pattern_paste].pat_size, pattern_paste_tbl[i_pattern_paste].pat_lofs, stamp_img_addr);

			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
					g_ctl_ipp_ime_output[i].width, g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs);
				exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, DISABLE, 0, 0, 0, 0, 0);
			}

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			vdo_frame_w = (!pattern_paste_tbl[i_pattern_paste].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.w, 4);
			vdo_frame_h = (!pattern_paste_tbl[i_pattern_paste].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.h, 4);

			exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, vdo_frame_w);
			exam_ctl_ipp_test_pattern_paste_trig();
			exam_ctl_ipp_saverst(hdl_idx);

			exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
		}
	}

	/* Pattern paste with region test case */
	if (test_case & (1<<EXAM_CTL_IPP_TEST_PATTERN_PASTE_WITH_REGION)) {
		CTL_IPP_DBG_DUMP("---- PATTERN PASTE + REGION TEST ----\r\n");
		for (i_pattern_paste = 0; i_pattern_paste < sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0])); i_pattern_paste++) {
			for (i_region = 0; i_region < sizeof(region_tbl) / sizeof(typeof(region_tbl[0])); i_region++) {
				CTL_IPP_DBG_DUMP("\r\nPattern paste (%d / %d) + region (%d / %d) test (buf_id %d)\r\n",
					(int)i_pattern_paste+1, (int)(sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0]))),
					(int)i_region+1, (int)(sizeof(region_tbl) / sizeof(typeof(region_tbl[0]))),
					(int)g_ctl_ipp_last_push_buf[0].buf_id+1);

				if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl[i_pattern_paste].pat_size.w, pattern_paste_tbl[i_pattern_paste].pat_size.h,
					pattern_paste_tbl[i_pattern_paste].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
					return -1;
				}

				exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl[i_pattern_paste].bgn_color[0],
				pattern_paste_tbl[i_pattern_paste].pat_win, pattern_paste_tbl[i_pattern_paste].pat_size, pattern_paste_tbl[i_pattern_paste].pat_lofs, stamp_img_addr);

				for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
					exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
						region_tbl[i_region].path[i].win.w, region_tbl[i_region].path[i].win.h, 0x100); // don't care, test ignore path lofs
					exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].width,
						g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs, region_tbl[i_region].path[i].win.x, region_tbl[i_region].path[i].win.y);
				}

				/* set apply */
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

				vdo_frame_w = (!pattern_paste_tbl[i_pattern_paste].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.w, 4);
				vdo_frame_h = (!pattern_paste_tbl[i_pattern_paste].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.h, 4);

				exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, vdo_frame_w);
				exam_ctl_ipp_test_pattern_paste_trig();
				exam_ctl_ipp_saverst(hdl_idx);

				exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
			}
		}
	}

	/* Pattern paste & normal flow switch test case */
	if (test_case & (1<<EXAM_CTL_IPP_TEST_PATTERN_PASTE_NORMAL_FLOW_SWITCH)) {
		CTL_IPP_DBG_DUMP("---- PATTERN PASTE & NORMAL FLOW SWITCH TEST ----\r\n");
		for (i_pattern_paste = 0; i_pattern_paste < sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0])); i_pattern_paste++) {
			CTL_IPP_DBG_DUMP("\r\nPattern paste test & normal flow switch %d / %d (buf_id %d)\r\n",
				(int)i_pattern_paste+1, (int)(sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0]))), (int)g_ctl_ipp_last_push_buf[0].buf_id+1);

			if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl[i_pattern_paste].pat_size.w, pattern_paste_tbl[i_pattern_paste].pat_size.h,
				pattern_paste_tbl[i_pattern_paste].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
				return -1;
			}

			exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl[i_pattern_paste].bgn_color[0],
				pattern_paste_tbl[i_pattern_paste].pat_win, pattern_paste_tbl[i_pattern_paste].pat_size, pattern_paste_tbl[i_pattern_paste].pat_lofs, stamp_img_addr);

			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
					g_ctl_ipp_ime_output[i].width, g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs);
				exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, DISABLE, 0, 0, 0, 0, 0);
			}

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			vdo_frame_w = (!pattern_paste_tbl[i_pattern_paste].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.w, 4);
			vdo_frame_h = (!pattern_paste_tbl[i_pattern_paste].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.h, 4);

			{
				exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);
				exam_ctl_ipp_test_pattern_paste_trig();
			}

			{
				exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, vdo_frame_w);
				exam_ctl_ipp_test_pattern_paste_trig();
				exam_ctl_ipp_saverst(hdl_idx);
			}

			{
				exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);
				exam_ctl_ipp_test_pattern_paste_trig();
				exam_ctl_ipp_saverst(hdl_idx);
			}

			exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
		}
	}

	/* Pattern paste with region and normal flow switch test case */
	if (test_case & (1<<EXAM_CTL_IPP_TEST_PATTERN_PASTE_WITH_REGION_AND_NORMAL_FLOW_SWITCH)) {
		CTL_IPP_DBG_DUMP("---- PATTERN PASTE + REGION TEST + NORMAL FLOW SWITCH ----\r\n");
		for (i_pattern_paste = 0; i_pattern_paste < sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0])); i_pattern_paste++) {
			for (i_region = 0; i_region < sizeof(region_tbl) / sizeof(typeof(region_tbl[0])); i_region++) {
				CTL_IPP_DBG_DUMP("\r\nPattern paste (%d / %d) + region (%d / %d) and normal flow switch test (buf_id %d)\r\n",
					(int)i_pattern_paste+1, (int)(sizeof(pattern_paste_tbl) / sizeof(typeof(pattern_paste_tbl[0]))),
					(int)i_region+1, (int)(sizeof(region_tbl) / sizeof(typeof(region_tbl[0]))),
					(int)g_ctl_ipp_last_push_buf[0].buf_id+1);

				if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl[i_pattern_paste].pat_size.w, pattern_paste_tbl[i_pattern_paste].pat_size.h,
					pattern_paste_tbl[i_pattern_paste].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
					return -1;
				}

				exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl[i_pattern_paste].bgn_color[0],
				pattern_paste_tbl[i_pattern_paste].pat_win, pattern_paste_tbl[i_pattern_paste].pat_size, pattern_paste_tbl[i_pattern_paste].pat_lofs, stamp_img_addr);

				for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
					exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
						region_tbl[i_region].path[i].win.w, region_tbl[i_region].path[i].win.h, 0x100); // don't care, test ignore path lofs
					exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].width,
						g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs, region_tbl[i_region].path[i].win.x, region_tbl[i_region].path[i].win.y);
				}

				/* set apply */
				ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

				vdo_frame_w = (!pattern_paste_tbl[i_pattern_paste].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.w * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.w, 4);
				vdo_frame_h = (!pattern_paste_tbl[i_pattern_paste].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl[i_pattern_paste].pat_size.h * 100 / pattern_paste_tbl[i_pattern_paste].pat_win.h, 4);

				{
					exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);
					exam_ctl_ipp_test_pattern_paste_trig();
				}

				{
					exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, vdo_frame_w);
					exam_ctl_ipp_test_pattern_paste_trig();
					exam_ctl_ipp_saverst(hdl_idx);
				}

				{
					exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);
					exam_ctl_ipp_test_pattern_paste_trig();
					exam_ctl_ipp_saverst(hdl_idx);
				}

				exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
			}
		}
	}

	/* Pattern paste error handling test case */
	if (test_case & (1<<EXAM_CTL_IPP_TEST_PATTERN_PASTE_ERROR_HANDLE)) {
		CTL_IPP_DBG_DUMP("---- ERROR TEST ----\r\n");
		for (i_pattern_paste = 0; i_pattern_paste < sizeof(pattern_paste_tbl_err) / sizeof(typeof(pattern_paste_tbl_err[0])); i_pattern_paste++) {
			CTL_IPP_DBG_DUMP("\r\nPattern paste error test %d / %d (buf_id %d)\r\n",
				(int)i_pattern_paste+1, (int)(sizeof(pattern_paste_tbl_err) / sizeof(typeof(pattern_paste_tbl_err[0]))), (int)g_ctl_ipp_last_push_buf[0].buf_id+1);

			if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl_err[i_pattern_paste].pat_size.w, pattern_paste_tbl_err[i_pattern_paste].pat_size.h,
				pattern_paste_tbl_err[i_pattern_paste].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
				return -1;
			}

			exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl_err[i_pattern_paste].bgn_color[0],
				pattern_paste_tbl_err[i_pattern_paste].pat_win, pattern_paste_tbl_err[i_pattern_paste].pat_size, pattern_paste_tbl_err[i_pattern_paste].pat_lofs, stamp_img_addr);

			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
					g_ctl_ipp_ime_output[i].width, g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs);
				exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, DISABLE, 0, 0, 0, 0, 0);
			}

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			vdo_frame_w = (!pattern_paste_tbl_err[i_pattern_paste].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl_err[i_pattern_paste].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl_err[i_pattern_paste].pat_size.w * 100 / pattern_paste_tbl_err[i_pattern_paste].pat_win.w, 4);
			vdo_frame_h = (!pattern_paste_tbl_err[i_pattern_paste].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl_err[i_pattern_paste].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl_err[i_pattern_paste].pat_size.h * 100 / pattern_paste_tbl_err[i_pattern_paste].pat_win.h, 4);

			exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, vdo_frame_w);
			exam_ctl_ipp_test_pattern_paste_trig();
			exam_ctl_ipp_saverst(hdl_idx);

			exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
		}

		CTL_IPP_DBG_DUMP("---- ERROR TEST VDO_FRAME LOFS ----\r\n");
		{
			if (exam_ctl_ipp_test_pattern_paste_stamp_alloc(pattern_paste_tbl_err_lofs[0].pat_size.w, pattern_paste_tbl_err_lofs[0].pat_size.h,
				pattern_paste_tbl_err_lofs[0].pat_lofs, &stamp_buf_addr, &stamp_img_addr) != 0) {
				return -1;
			}

			exam_ctl_ipp_test_pattern_paste_set_pat_param(g_ctl_ipp_hdl_list[hdl_idx], &pattern_paste_tbl_err_lofs[0].bgn_color[0],
				pattern_paste_tbl_err_lofs[0].pat_win, pattern_paste_tbl_err_lofs[0].pat_size, pattern_paste_tbl_err_lofs[0].pat_lofs, stamp_img_addr);

			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				exam_ctl_ipp_test_pattern_paste_set_out_path(g_ctl_ipp_hdl_list[hdl_idx], i, g_ctl_ipp_ime_output_enable[i], g_ctl_ipp_ime_output[i].fmt,
					g_ctl_ipp_ime_output[i].width, g_ctl_ipp_ime_output[i].height, g_ctl_ipp_ime_output[i].lofs);
				exam_ctl_ipp_test_pattern_paste_set_out_path_region(g_ctl_ipp_hdl_list[hdl_idx], i, DISABLE, 0, 0, 0, 0, 0);
			}

			/* set apply */
			ctl_ipp_set(g_ctl_ipp_hdl_list[hdl_idx], CTL_IPP_ITEM_APPLY, (void *)&apply_id);

			vdo_frame_w = (!pattern_paste_tbl_err_lofs[0].pat_win.w) ? ALIGN_CEIL(pattern_paste_tbl_err_lofs[0].pat_size.w, 4) : ALIGN_CEIL(pattern_paste_tbl_err_lofs[0].pat_size.w * 100 / pattern_paste_tbl_err_lofs[0].pat_win.w, 4);
			vdo_frame_h = (!pattern_paste_tbl_err_lofs[0].pat_win.h) ? ALIGN_CEIL(pattern_paste_tbl_err_lofs[0].pat_size.h, 4) : ALIGN_CEIL(pattern_paste_tbl_err_lofs[0].pat_size.h * 100 / pattern_paste_tbl_err_lofs[0].pat_win.h, 4);

			exam_ctl_ipp_sndevt_pattern_paste(hdl_idx, evt_cnt, (UINT32)in_addr, vdo_frame_w, vdo_frame_h, EXAM_CTL_IPP_TEST_PATTERN_PASTE_IN_LOFS);
			exam_ctl_ipp_test_pattern_paste_trig();
			exam_ctl_ipp_saverst(hdl_idx);

			exam_ctl_ipp_test_pattern_paste_stamp_release(stamp_buf_addr);
		}
	}

	//exam_ctl_ipp_release_hdl(hdl_idx);

CTRL_TEST_PATTERN_PASTE_END:
	//exam_ctl_ipp_buf_release();
	g_ctl_ipp_api_dbg_log = 0;
	g_ctl_ipp_clr_buf = 0;

	CTL_IPP_DBG_DUMP("---- PATTERN PASTE TEST DONE ----\r\n");

	return 0;
}

THREAD_DECLARE(ctl_ipp_test_tsk, p1)
{
#if 0
	// do something
	while (1) {
		exam_ctl_ipp_test_script_rtc_flush(500);
	}
#endif

	THREAD_RETURN(0);
}

BOOL nvt_ctl_ipp_api_ctrl_testcmd(unsigned char argc, char **pargv)
{
	INT32 rt;

	rt = 0;
	if (strcmp(pargv[0], "buf_alloc") == 0) {
		/* allocat input/output buffer from frammap */
		rt = exam_ctl_ipp_buf_alloc();
	} else if (strcmp(pargv[0], "buf_release") == 0) {
		/* release input/output buffer to frammap */
		rt = exam_ctl_ipp_buf_release();
	} else if (strcmp(pargv[0], "crt_hdl") == 0) {
		/* create handle */
		UINT32 hdl_idx;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		rt = exam_ctl_ipp_create_hdl(hdl_idx);
	} else if (strcmp(pargv[0], "rls_hdl") == 0) {
		/* release handle */
		UINT32 hdl_idx;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		rt = exam_ctl_ipp_release_hdl(hdl_idx);
	} else if (strcmp(pargv[0], "pause") == 0) {
		/* pause kdf task */
		BOOL wait_end, flush;

		sscanf(pargv[1], "%d", (int *)&wait_end);
		sscanf(pargv[2], "%d", (int *)&flush);
		rt = exam_ctl_ipp_tsk_pause(wait_end, flush);
	} else if (strcmp(pargv[0], "resume") == 0) {
		/* resume kdf task */
		BOOL flush;

		sscanf(pargv[1], "%d", (int *)&flush);
		rt = exam_ctl_ipp_tsk_resume(flush);
	} else if (strcmp(pargv[0], "sndevt") == 0) {
		/* load image and send kdf event */
		UINT32 hdl_idx, evt_cnt;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		sscanf(pargv[2], "%d", (int *)&evt_cnt);
		rt = exam_ctl_ipp_sndevt(hdl_idx, evt_cnt);
	} else if (strcmp(pargv[0], "sndevt_hdr") == 0) {
		/* load image and send kdf event */
		UINT32 hdl_idx, evt_cnt;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		sscanf(pargv[2], "%d", (int *)&evt_cnt);
		rt = exam_ctl_ipp_sndevt_hdr(hdl_idx, evt_cnt);
	} else if (strcmp(pargv[0], "saverst") == 0) {
		/* save result */
		UINT32 hdl_idx;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		rt = exam_ctl_ipp_saverst(hdl_idx);
	} else if (strcmp(pargv[0], "script") == 0) {
		/* test script */
		exam_ctl_ipp_test_script();
	} else if (strcmp(pargv[0], "r2y") == 0) {
		/* raw to yuv test */
		UINT32 hdl_idx, evt_cnt;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		sscanf(pargv[2], "%d", (int *)&evt_cnt);
		exam_ctl_ipp_test_r2y(hdl_idx, evt_cnt);
	} else if (strcmp(pargv[0], "region") == 0) {
		exam_ctl_ipp_test_region();
	} else if (strcmp(pargv[0], "pat_paste") == 0) {
		UINT32 test_case = 0xffffffff;

		if (argc > 1) {
			sscanf(pargv[1], "%x", (int *)&test_case);
		}
		exam_ctl_ipp_test_pattern_paste(test_case);
	} else if (strcmp(pargv[0], "sndevt_auto") == 0) {
		UINT32 op, hdl_idx, data;

		sscanf(pargv[1], "%d", (int *)&op);
		sscanf(pargv[2], "%d", (int *)&hdl_idx);
		sscanf(pargv[3], "%d", (int *)&data);
		exam_ctl_ipp_sndevt_timer_op(op, hdl_idx, data);
	} else if (strcmp(pargv[0], "reg_isp") == 0) {
		UINT32 evt;

		sscanf(pargv[2], "%x", (unsigned int *)&evt);
		exam_ctl_ipp_isp_reg_cb(pargv[1], evt);
	} else if (strcmp(pargv[0], "isp_saveyuv") == 0) {
		UINT32 isp_id, pid;

		sscanf(pargv[1], "%d", (int *)&isp_id);
		sscanf(pargv[2], "%d", (int *)&pid);
		exam_ctl_ipp_isp_saveyuv(isp_id, pid);
	} else if (strcmp(pargv[0], "flush_test") == 0) {
		UINT32 interval;

		sscanf(pargv[1], "%d", (int *)&interval);
		exam_ctl_ipp_test_script_rtc_flush(interval);
	} else if (strcmp(pargv[0], "dir_start") == 0) {
		UINT32 hdl_idx;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		rt = exam_ctl_ipp_direct_start(hdl_idx);
	} else if (strcmp(pargv[0], "dir_set_cb") == 0) {
		UINT32 sie_hdl;

		sscanf(pargv[1], "%x", (int *)&sie_hdl);
		rt = exam_ctl_ipp_direct_set_cb(sie_hdl);
	} else if (strcmp(pargv[0], "dir_stop") == 0) {
		UINT32 hdl_idx;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		rt = exam_ctl_ipp_direct_stop(hdl_idx);
	} else if (strcmp(pargv[0], "dir_wake") == 0) {
		UINT32 hdl_addr;

		sscanf(pargv[1], "%x", (int *)&hdl_addr);
		rt = exam_ctl_ipp_direct_wakeup(hdl_addr);
	} else if (strcmp(pargv[0], "dir_sleep") == 0) {
		UINT32 hdl_addr;

		sscanf(pargv[1], "%x", (int *)&hdl_addr);
		rt = exam_ctl_ipp_direct_sleep(hdl_addr);
	} else if (strcmp(pargv[0], "cb_dir_start") == 0) {
		UINT32 hdl_idx;
		UINT32 cnt;
		UINT32 event;

		sscanf(pargv[1], "%d", (int *)&hdl_idx);
		sscanf(pargv[2], "%d", (int *)&cnt);
		sscanf(pargv[3], "%d", (int *)&event);
		rt = exam_ctl_ipp_direct_cb(hdl_idx, cnt, event);

	} else if (strcmp(pargv[0], "bufio_test") == 0) {
		exam_ctl_ipp_test_script_bufio_sequence();
	} else if (strcmp(pargv[0], "task_open") == 0) {
		THREAD_HANDLE g_ctl_ipp_test_tsk_id;

		THREAD_CREATE(g_ctl_ipp_test_tsk_id, ctl_ipp_test_tsk, NULL, "ctl_ipp_test_tsk");
		THREAD_SET_PRIORITY(g_ctl_ipp_test_tsk_id, CTL_IPP_TSK_PRI);
		THREAD_RESUME(g_ctl_ipp_test_tsk_id);
	} else if (strcmp(pargv[0], "ctx_init") == 0) {
		CTL_IPP_CTX_BUF_CFG ctx_buf;
		UINT32 buf_size, buf_addr;

		sscanf(pargv[1], "%d", (int *)&ctx_buf.n);
		buf_size = ctl_ipp_query(ctx_buf);
		buf_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(buf_size);
		rt = ctl_ipp_init(ctx_buf, buf_addr, buf_size);
	} else if (strcmp(pargv[0], "ctx_uninit") == 0) {
		rt = ctl_ipp_uninit();
	} else {
		DBG_DUMP("Unknown cmd %s\r\n", pargv[0]);
		rt = -1;
	}

	return rt;
}

BOOL nvt_ctl_ipp_api_dump_isp_item(unsigned char argc, char **pargv)
{
	UINT32 isp_item;
	UINT32 isp_id;

	sscanf(pargv[0], "%d", (int *)&isp_id);
	sscanf(pargv[1], "%d", (int *)&isp_item);

	CTL_IPP_DBG_DUMP("dump isp_id %d, isp_item %d\r\n", (int)isp_id, (int)isp_item);

	if (isp_item == CTL_IPP_ISP_ITEM_FUNC_EN) {
		ISP_FUNC_EN func_en = 0;

		ctl_ipp_isp_get(isp_id, CTL_IPP_ISP_ITEM_FUNC_EN, (void *)&func_en);
		DBG_DUMP("isp func_en 0x%.8x\r\n", (unsigned int)func_en);
	} else if (isp_item == CTL_IPP_ISP_ITEM_IOSIZE) {
		UINT32 i;
		CTL_IPP_ISP_IOSIZE iosize = {0};

		ctl_ipp_isp_get(isp_id, CTL_IPP_ISP_ITEM_IOSIZE, (void *)&iosize);
		DBG_DUMP("isp in_sz = (%d %d)\r\n", (int)(iosize.in_sz.w), (int)(iosize.in_sz.h));
		for (i = 0; i < CTL_IPP_ISP_OUT_CH_MAX_NUM; i++) {
			DBG_DUMP("isp out_ch[%d] = (%d %d)\r\n", (int)(i), (int)(iosize.out_ch[i].w), (int)(iosize.out_ch[i].h));
		}
	} else if (isp_item == CTL_IPP_ISP_ITEM_VA_RST) {
		CTL_IPP_ISP_VA_RST *va_rst;
		UINT32 x, y;

		va_rst = ctl_ipp_util_os_malloc_wrap(sizeof(CTL_IPP_ISP_VA_RST));
		if (va_rst == NULL) {
			DBG_DUMP("malloc failed\r\n");
			return 0;
		}
		ctl_ipp_isp_get(isp_id, isp_item, (void *)va_rst);

		DBG_DUMP("G1:\r\n");
		for (y = 0; y < 8; y++) {
			DBG_DUMP("%d: ", (int)y);
			for (x = 0; x < 8; x++) {
				DBG_DUMP("(%5d, %5d), ", (int)va_rst->buf_g1_h[x + y*8], (int)va_rst->buf_g1_v[x + y*8]);
			}
			DBG_DUMP("\r\n");
		}

		DBG_DUMP("G1 CNT:\r\n");
		for (y = 0; y < 8; y++) {
			DBG_DUMP("%d: ", (int)y);
			for (x = 0; x < 8; x++) {
				DBG_DUMP("(%5d, %5d), ", (int)va_rst->buf_g1_h_cnt[x + y*8], (int)va_rst->buf_g1_v_cnt[x + y*8]);
			}
			DBG_DUMP("\r\n");
		}

		DBG_DUMP("G2:\r\n");
		for (y = 0; y < 8; y++) {
			DBG_DUMP("%d: ", (int)y);
			for (x = 0; x < 8; x++) {
				DBG_DUMP("(%5d, %5d), ", (int)va_rst->buf_g2_h[x + y*8], (int)va_rst->buf_g2_v[x + y*8]);
			}
			DBG_DUMP("\r\n");
		}

		DBG_DUMP("G2 CNT:\r\n");
		for (y = 0; y < 8; y++) {
			DBG_DUMP("%d: ", (int)y);
			for (x = 0; x < 8; x++) {
				DBG_DUMP("(%5d, %5d), ", (int)va_rst->buf_g2_h_cnt[x + y*8], (int)va_rst->buf_g2_v_cnt[x + y*8]);
			}
			DBG_DUMP("\r\n");
		}

		ctl_ipp_util_os_mfree_wrap(va_rst);
	} else if (isp_item == CTL_IPP_ISP_ITEM_VA_INDEP_RST) {
		CTL_IPP_ISP_VA_INDEP_RST *va_indep_rst;
		UINT32 i;

		va_indep_rst = ctl_ipp_util_os_malloc_wrap(sizeof(CTL_IPP_ISP_VA_INDEP_RST));
		if (va_indep_rst == NULL) {
			DBG_DUMP("malloc failed\r\n");
			return 0;
		}
		ctl_ipp_isp_get(isp_id, isp_item, (void *)va_indep_rst);

		DBG_DUMP("G1:\r\n");
		for (i = 0; i < 5; i++) {
			DBG_DUMP("(%5d, %5d)\r\n", (int)va_indep_rst->g1_h[i], (int)va_indep_rst->g1_v[i]);
		}

		DBG_DUMP("G2:\r\n");
		for (i = 0; i < 5; i++) {
			DBG_DUMP("(%5d, %5d)\r\n", (int)va_indep_rst->g2_h[i], (int)va_indep_rst->g2_v[i]);
		}

		ctl_ipp_util_os_mfree_wrap(va_indep_rst);
	} else if (isp_item == CTL_IPP_ISP_ITEM_DCE_HIST_RST) {
		CTL_IPP_ISP_DCE_HIST_RST *hist_rst;
		UINT32 i;

		hist_rst = ctl_ipp_util_os_malloc_wrap(sizeof(CTL_IPP_ISP_DCE_HIST_RST));
		if (hist_rst == NULL) {
			DBG_DUMP("malloc failed\r\n");
			return 0;
		}
		ctl_ipp_isp_get(isp_id, isp_item, (void *)hist_rst);

		DBG_DUMP("PRE_WDR HIST:\r\n");
		for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
			DBG_DUMP("%d, ", (int)hist_rst->hist_stcs_pre_wdr[i]);
		}

		DBG_DUMP("\r\nPOST_WDR HIST:\r\n");
		for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
			DBG_DUMP("%d, ", (int)hist_rst->hist_stcs_post_wdr[i]);
		}
		DBG_DUMP("\r\n");

		ctl_ipp_util_os_mfree_wrap(hist_rst);
	} else if (isp_item == CTL_IPP_ISP_ITEM_EDGE_STCS) {
		CTL_IPP_ISP_EDGE_STCS edge_stcs = {0};

		ctl_ipp_isp_get(isp_id, isp_item, (void *)&edge_stcs);
		DBG_DUMP("edge_stcs(localmax, coneng_max, coneng_avg): %d, %d, %d\r\n", (int)edge_stcs.localmax_max, (int)edge_stcs.coneng_max, (int)edge_stcs.coneng_avg);
	} else if (isp_item == CTL_IPP_ISP_ITEM_DEFOG_STCS) {
		CTL_IPP_ISP_DEFOG_STCS defog_stcs = {0};

		ctl_ipp_isp_get(isp_id, isp_item, (void *)&defog_stcs);
		DBG_DUMP("defog airlight: %d %d %d\r\n", (int)defog_stcs.airlight[0], (int)defog_stcs.airlight[1], (int)defog_stcs.airlight[2]);
	} else if (isp_item == CTL_IPP_ISP_ITEM_3DNR_STA) {
		CTL_IPP_ISP_3DNR_STA sta_param = {0};

		ctl_ipp_isp_get(isp_id, isp_item, (void *)&sta_param);
		DBG_DUMP("3dnr sta addr 0x%.8x, lofs %d, spl_step(%d, %d), spl_num(%d, %d), spl_st(%d, %d)\r\n",
			(unsigned int)sta_param.buf_addr, (int)sta_param.lofs,
			(int)sta_param.sample_step.w, (int)sta_param.sample_step.h,
			(int)sta_param.sample_num.w, (int)sta_param.sample_num.h,
			(int)sta_param.sample_st.x, (int)sta_param.sample_st.y);
	} else if (isp_item == CTL_IPP_ISP_ITEM_STRIPE_INFO) {
		CTL_IPP_ISP_STRP_INFO strp_info = {0};
		INT32 rt;
		UINT32 i;

		rt = ctl_ipp_isp_get(isp_id, isp_item, (void *)&strp_info);

		if (rt == E_OK) {
			DBG_DUMP("stripe number %d\r\n", (int)strp_info.num);
			for (i = 0; i < strp_info.num; i++) {
				DBG_DUMP("stripe size[%d] = %d\r\n", (int)i, (int)strp_info.size[i]);
			}
		} else {
			DBG_DUMP("isp get stripe failed\r\n");
		}
	} else if (isp_item == CTL_IPP_ISP_ITEM_IME_LCA_SIZE) {
		CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_ratio = {0};

		ctl_ipp_isp_get(isp_id, isp_item, (void *)&lca_ratio);
		DBG_DUMP("lca ratio = %d, base = %d\r\n", lca_ratio.ratio, lca_ratio.ratio_base);
	} else if (isp_item == CTL_IPP_ISP_ITEM_MD_SUBOUT) {
		CTL_IPP_ISP_MD_SUBOUT md = {0};
		INT32 rt;

		rt = ctl_ipp_isp_get(isp_id, isp_item, (void *)&md);
		if (rt == E_OK) {
			DBG_DUMP("md src_size(%d %d), md_size(%d %d), lof %d, addr 0x%.8x\r\n",
				(int)md.src_img_size.w, (int)md.src_img_size.h,
				(int)md.md_size.w, (int)md.md_size.h, (int)md.md_lofs, (unsigned int)(int)md.addr);
			debug_dumpmem(md.addr, (md.md_lofs * md.md_size.h));
		} else {
			DBG_DUMP("isp get md failed\r\n");
		}
	}

	return 0;
}

BOOL nvt_ctl_ipp_api_set_flip(unsigned char argc, char **pargv)
{
	UINT32 rt, hdl, apply_id, i;
	CTL_IPP_FLIP_TYPE ipp_global_flip;
	CTL_IPP_OUT_PATH_FLIP ipp_out_path_flip;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	CTL_IPP_DBG_DUMP("hdl 0x%.8x\r\n", (unsigned int)hdl);

	sscanf(pargv[1], "%d", (int *)&ipp_global_flip);
	CTL_IPP_DBG_DUMP("global flip %d\r\n", (int)(ipp_global_flip));
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_FLIP, (void *)&ipp_global_flip);

	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		sscanf(pargv[2 + i], "%d", (int *)&ipp_out_path_flip.enable);
		ipp_out_path_flip.pid = i;
		CTL_IPP_DBG_DUMP("path %d, flip %d\r\n", (int)(i), (int)(ipp_out_path_flip.enable));
		rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH_FLIP, (void *)&ipp_out_path_flip);
	}

	apply_id = 0;
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("this setting will apply at frame %d\r\n", (int)(apply_id));

	return 0;
}

BOOL nvt_ctl_ipp_api_set_out_path(unsigned char argc, char **pargv)
{
	UINT32 rt, hdl, apply_id;
	CTL_IPP_OUT_PATH ime_out;
	VDO_PXLFMT fmt;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	CTL_IPP_DBG_DUMP("hdl 0x%.8x\r\n", (unsigned int)hdl);

	sscanf(pargv[1], "%d", (int *)&ime_out.pid);
	rt = ctl_ipp_get(hdl, CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("get path_%d info failed\r\n", (int)(ime_out.pid));
		return -1;
	}

	sscanf(pargv[2], "%d", (int *)&ime_out.enable);
	sscanf(pargv[3], "%d", (int *)&ime_out.size.w);
	sscanf(pargv[4], "%d", (int *)&ime_out.size.h);
	sscanf(pargv[5], "%x", (unsigned int *)&fmt);
	ime_out.lofs = ime_out.size.w;
	ime_out.crp_window.x = 0;
	ime_out.crp_window.y = 0;
	ime_out.crp_window.w = ime_out.size.w;
	ime_out.crp_window.h = ime_out.size.h;
	if (fmt != 0) {
		ime_out.fmt = fmt;
	}

	apply_id = 0;
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH, (void *)&ime_out);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("this setting will apply at frame %d\r\n", (int)(apply_id));

	return 0;
}

BOOL nvt_ctl_ipp_api_set_in_crop(unsigned char argc, char **pargv)
{
	UINT32 rt, hdl, apply_id;
	CTL_IPP_IN_CROP crop;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%d", (int *)&crop.mode);
	sscanf(pargv[2], "%d", (int *)&crop.crp_window.x);
	sscanf(pargv[3], "%d", (int *)&crop.crp_window.y);
	sscanf(pargv[4], "%d", (int *)&crop.crp_window.w);
	sscanf(pargv[5], "%d", (int *)&crop.crp_window.h);

	apply_id = 0;
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_IN_CROP, (void *)&crop);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl 0x%.8x, crop mode %d, window (%d %d %d %d), apply at frame %d\r\n", (unsigned int)hdl,
		(int)crop.mode, (int)crop.crp_window.x, (int)crop.crp_window.y, (int)crop.crp_window.w, (int)crop.crp_window.h, (int)apply_id);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_func_en(unsigned char argc, char **pargv)
{
	UINT32 rt, hdl, func_en, apply_id;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%x", (unsigned int *)&func_en);

	apply_id = 0;
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_FUNCEN, (void *)&func_en);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl 0x%.8x, func_en 0x%.8x, apply at frame %d\r\n", (unsigned int)hdl, (unsigned int)func_en, (int)apply_id);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_ispid(unsigned char argc, char **pargv)
{
	CTL_IPP_ALGID isp_id;
	UINT32 rt, hdl;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%x", (unsigned int *)&isp_id.id);

	isp_id.type = CTL_IPP_ALGID_IQ;
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_ALGID_IMM, (void *)&isp_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl 0x%.8x, isp_id 0x%x\r\n", (unsigned int)hdl, (unsigned int)isp_id.id);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_isp_cbtime_check(unsigned char argc, char **pargv)
{
	UINT32 th, num;

	sscanf(pargv[0], "%d", (int *)&th);
	sscanf(pargv[1], "%d", (int *)&num);

	ctl_ipp_isp_dbg_cbtime_set_threshold(th, num);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_bp(unsigned char argc, char **pargv)
{
	INT32 rt;
	UINT32 hdl;
	UINT32 bp_line;
	UINT32 apply_id;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%d", (int *)&bp_line);

	apply_id = 0;
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_LOW_DELAY_BP, (void *)&bp_line);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl(0x%.8x), bp %d, apply at frame %d\r\n", (unsigned int)hdl, (int)bp_line, (int)apply_id);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_eng_hang_handle(unsigned char argc, char **pargv)
{
	UINT32 mode;

	sscanf(pargv[0], "%x", (unsigned int*)&mode);
	CTL_IPP_DBG_DUMP("eng hang handle mode 0x%.8x, f_path %s\r\n", (unsigned int)mode, pargv[1]);
	kdf_ipp_eng_hang_config(mode, pargv[1]);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_strp_rule(unsigned char argc, char **pargv)
{
	INT32 rt;
	UINT32 hdl;
	UINT32 rule;
	UINT32 apply_id;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%d", (int *)&rule);

	apply_id = 0;
	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_STRP_RULE, (void *)&rule);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl(0x%.8x), strp rule %d, apply at frame %d\r\n", (unsigned int)hdl, (int)rule, (int)apply_id);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_path_order(unsigned char argc, char **pargv)
{
	INT32 rt;
	UINT32 hdl;
	CTL_IPP_OUT_PATH_ORDER cfg;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%d", (int *)&cfg.pid);
	sscanf(pargv[2], "%d", (int *)&cfg.order);

	rt = ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH_ORDER_IMM, (void *)&cfg);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl(0x%.8x), path %d, order %d\r\n", (unsigned int)hdl, (int)cfg.pid, (int)cfg.order);

	return 0;
}

BOOL nvt_ctl_ipp_api_set_path_region(unsigned char argc, char **pargv)
{
	INT32 rt = CTL_IPP_E_OK;
	UINT32 hdl, apply_id;
	CTL_IPP_OUT_PATH_REGION region;

	sscanf(pargv[0], "%x", (unsigned int *)&hdl);
	sscanf(pargv[1], "%d", (int *)&region.pid);
	sscanf(pargv[2], "%d", (int *)&region.enable);
	sscanf(pargv[3], "%d", (int *)&region.bgn_size.w);
	sscanf(pargv[4], "%d", (int *)&region.bgn_size.h);
	sscanf(pargv[5], "%d", (int *)&region.region_ofs.x);
	sscanf(pargv[6], "%d", (int *)&region.region_ofs.y);

	region.bgn_lofs = region.bgn_size.w;

	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
	rt |= ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_DUMP("set failed\r\n");
		return -1;
	}
	CTL_IPP_DBG_DUMP("hdl(0x%.8x), path %d, en %d, bg_lofs %d, bg_w %d, bg_h %d, ofs_x %d, ofs_y %d\r\n", (unsigned int)hdl, (int)region.pid, (int)region.enable, (int)region.bgn_lofs, (int)region.bgn_size.w, (int)region.bgn_size.h, (int)region.region_ofs.x, (int)region.region_ofs.y);

	return 0;
}
#endif

#if 0
#endif

BOOL nvt_ctl_ipp_api_dump_ts(unsigned char argc, char **pargv)
{
	ctl_ipp_dbg_ts_dump(ctl_ipp_int_printf);
	DBG_DUMP("\r\n");
	ctl_ipp_dbg_inbuf_cbtime_dump(ctl_ipp_int_printf);
	DBG_DUMP("\r\n");
	ctl_ipp_dbg_outbuf_cbtime_dump(ctl_ipp_int_printf);
	DBG_DUMP("\r\n");
	ctl_ipp_isp_dbg_cbtime_dump(ctl_ipp_int_printf);
	DBG_DUMP("\r\n");
	kdf_ipp_kdrv_wrapper_debug_dump(ctl_ipp_int_printf);
	DBG_DUMP("\r\n");
	ipp_event_dump_proc_time(ctl_ipp_int_printf);
	return 0;
}

BOOL nvt_ctl_ipp_api_dump_hdl_all(unsigned char argc, char **pargv)
{
	ctl_ipp_dbg_hdl_dump_all(ctl_ipp_int_printf);

	/* other information */
	ctl_ipp_buf_msg_dump(ctl_ipp_int_printf);
	ctl_ipp_ise_dumpinfo(ctl_ipp_int_printf);
	ctl_ipp_isp_dump(ctl_ipp_int_printf);
	ctl_ipp_info_dump(ctl_ipp_int_printf);
	ctl_ipp_dbg_ctxbuf_log_dump(ctl_ipp_int_printf);
	ctl_ipp_dbg_dma_wp_dump(ctl_ipp_int_printf);

	return 0;
}

BOOL nvt_ctl_ipp_api_dbg_level(unsigned char argc, char **pargv)
{
	INT32 level;
	UINT32 err = 0;

	sscanf(pargv[0], "%d", (int *)&level);
	if (level <= NVT_DBG_USER) {
		ctl_ipp_debug_level = level;
		CTL_IPP_DBG_DUMP("set debug level to %d\r\n", (int)(level));
	}

	if (argc >= 2) {
		if (strcmp(pargv[1], "none") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_NONE;
		} else if (strcmp(pargv[1], "flow") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_FLOW;
		} else if (strcmp(pargv[1], "buf") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_BUF;
		} else if (strcmp(pargv[1], "isp") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_ISP;
		} else if (strcmp(pargv[1], "ise") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_ISE;
		} else if (strcmp(pargv[1], "exam") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_EXAM;
		} else if (strcmp(pargv[1], "dbg") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_DBG;
		} else if (strcmp(pargv[1], "all") == 0) {
			g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_ALL;
		} else {
			CTL_IPP_DBG_WRN("Unknown type %s\r\n", pargv[1]);
			err = 1;
		}

		if (err == 0) {
			CTL_IPP_DBG_DUMP("set trc type to %s\r\n", pargv[1]);
		}
	}

	return 0;
}

static INT32 nvt_ctl_ipp_isp_dbg_cb(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	if ((evt != ISP_EVENT_IPP_CFGSTART) && (evt != ISP_EVENT_PARAM_RST))
		return E_OK;

	/* ife */
	{
		KDRV_IFE_FUSION_PARAM par_fusion;
		KDRV_IFE_PARAM_IQ_ALL_PARAM ife_iq;

		par_fusion.fu_ctrl.y_mean_sel = 0;
		par_fusion.fu_ctrl.mode    = 0;
		par_fusion.fu_ctrl.ev_ratio = 16;
		par_fusion.bld_cur.nor_sel = 1;
		par_fusion.bld_cur.dif_sel = 2;
		par_fusion.bld_cur.l_nor_w_edge = 0;
		par_fusion.bld_cur.l_nor_range = 9;
		par_fusion.bld_cur.l_nor_knee[0] = 3072;

		par_fusion.bld_cur.s_nor_w_edge = 1;
		par_fusion.bld_cur.s_nor_range = 6;
		par_fusion.bld_cur.s_nor_knee[0] = 192;

		par_fusion.bld_cur.l_dif_w_edge = 0;
		par_fusion.bld_cur.l_dif_range = 9;
		par_fusion.bld_cur.l_dif_knee[0] = 3072;

		par_fusion.bld_cur.s_dif_w_edge = 1;
		par_fusion.bld_cur.s_dif_range = 6;
		par_fusion.bld_cur.s_dif_knee[0] = 192;

		par_fusion.mc_para.lum_th = 256;
		par_fusion.mc_para.diff_ratio = 0;
		par_fusion.mc_para.pos_diff_w[0] = 0;
		par_fusion.mc_para.pos_diff_w[1] = 2;
		par_fusion.mc_para.pos_diff_w[2] = 4;
		par_fusion.mc_para.pos_diff_w[3] = 8;
		par_fusion.mc_para.pos_diff_w[4] = 10;
		par_fusion.mc_para.pos_diff_w[5] = 12;
		par_fusion.mc_para.pos_diff_w[6] = 14;
		par_fusion.mc_para.pos_diff_w[7] = 16;
		par_fusion.mc_para.pos_diff_w[8] = 14;
		par_fusion.mc_para.pos_diff_w[9] = 12;
		par_fusion.mc_para.pos_diff_w[10] = 10;
		par_fusion.mc_para.pos_diff_w[11] = 8;
		par_fusion.mc_para.pos_diff_w[12] = 6;
		par_fusion.mc_para.pos_diff_w[13] = 4;
		par_fusion.mc_para.pos_diff_w[14] = 2;
		par_fusion.mc_para.pos_diff_w[15] = 0;
		par_fusion.mc_para.dwd = 0;

		par_fusion.mc_para.neg_diff_w[0] = 0;
		par_fusion.mc_para.neg_diff_w[1] = 2;
		par_fusion.mc_para.neg_diff_w[2] = 4;
		par_fusion.mc_para.neg_diff_w[3] = 8;
		par_fusion.mc_para.neg_diff_w[4] = 10;
		par_fusion.mc_para.neg_diff_w[5] = 12;
		par_fusion.mc_para.neg_diff_w[6] = 14;
		par_fusion.mc_para.neg_diff_w[7] = 16;
		par_fusion.mc_para.neg_diff_w[8] = 14;
		par_fusion.mc_para.neg_diff_w[9] = 12;
		par_fusion.mc_para.neg_diff_w[10] = 10;
		par_fusion.mc_para.neg_diff_w[11] = 8;
		par_fusion.mc_para.neg_diff_w[12] = 6;
		par_fusion.mc_para.neg_diff_w[13] = 4;
		par_fusion.mc_para.neg_diff_w[14] = 2;
		par_fusion.mc_para.neg_diff_w[15] = 0;

		par_fusion.dk_sat.th[0] = 32;
		par_fusion.dk_sat.step[0] = 16;
		par_fusion.dk_sat.low_bound[0] = 0;

		par_fusion.dk_sat.th[1] = 32;
		par_fusion.dk_sat.step[1] = 16;
		par_fusion.dk_sat.low_bound[1] = 0;

		par_fusion.s_comp.enable = FALSE;
		par_fusion.s_comp.knee[0] = 16;
		par_fusion.s_comp.knee[1] = 64;
		par_fusion.s_comp.knee[2] = 256;
		par_fusion.s_comp.sub_point[0] = 0;
		par_fusion.s_comp.sub_point[1] = 64;
		par_fusion.s_comp.sub_point[2] = 256;
		par_fusion.s_comp.sub_point[3] = 1024;
		par_fusion.s_comp.shift[0] = 2;
		par_fusion.s_comp.shift[1] = 2;
		par_fusion.s_comp.shift[2] = 2;
		par_fusion.s_comp.shift[3] = 2;

		par_fusion.fu_cgain.enable = FALSE;
		par_fusion.fu_cgain.bit_field = 0;

		par_fusion.fu_cgain.fcgain_path0_r   = 256;
		par_fusion.fu_cgain.fcgain_path0_gr  = 256;
		par_fusion.fu_cgain.fcgain_path0_gb  = 256;
		par_fusion.fu_cgain.fcgain_path0_b   = 256;
		par_fusion.fu_cgain.fcgain_path0_ir  = 256;

		par_fusion.fu_cgain.fcgain_path1_r   = 256;
		par_fusion.fu_cgain.fcgain_path1_gr  = 256;
		par_fusion.fu_cgain.fcgain_path1_gb  = 256;
		par_fusion.fu_cgain.fcgain_path1_b   = 256;
		par_fusion.fu_cgain.fcgain_path1_ir  = 256;

		par_fusion.fu_cgain.fcofs_path0_r   = 0;
		par_fusion.fu_cgain.fcofs_path0_gr  = 0;
		par_fusion.fu_cgain.fcofs_path0_gb  = 0;
		par_fusion.fu_cgain.fcofs_path0_b   = 0;
		par_fusion.fu_cgain.fcofs_path0_ir  = 0;

		par_fusion.fu_cgain.fcofs_path1_r   = 0;
		par_fusion.fu_cgain.fcofs_path1_gr  = 0;
		par_fusion.fu_cgain.fcofs_path1_gb  = 0;
		par_fusion.fu_cgain.fcofs_path1_b   = 0;
		par_fusion.fu_cgain.fcofs_path1_ir  = 0;

		memset((void *)&ife_iq, 0, sizeof(KDRV_IFE_PARAM_IQ_ALL_PARAM));
		ife_iq.p_fusion = &par_fusion;
		ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IFE_IQ_PARAM, (void *)&ife_iq);
	}

	/* dce */
	{
		UINT32 cfa_freq_lut[KDRV_CFA_FREQ_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 15};
		UINT32 cfa_luma_lut[KDRV_CFA_LUMA_NUM] = {128, 120, 95, 72, 58, 48, 40, 36, 32, 32, 32, 32, 32, 32, 32, 32, 32};
		UINT32 cfa_fcs_lut[KDRV_CFA_FCS_NUM] = {15, 15, 7, 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0};
		KDRV_DCE_CFA_PARAM cfa_param;
		KDRV_DCE_PARAM_IQ_ALL_PARAM dce_iq;
		UINT32 i;

		cfa_param.cfa_enable = TRUE;
		cfa_param.cfa_interp.edge_dth = 12;
		cfa_param.cfa_interp.edge_dth2 = 40;
		cfa_param.cfa_interp.freq_th = 160;
		for (i = 0; i < KDRV_CFA_FREQ_NUM; i++) {
			cfa_param.cfa_interp.freq_lut[i] = cfa_freq_lut[i];
		}
		for (i = 0; i < KDRV_CFA_LUMA_NUM; i++) {
			cfa_param.cfa_interp.luma_wt[i] = cfa_luma_lut[i];
		}

		cfa_param.cfa_correction.rb_corr_enable = TRUE;
		cfa_param.cfa_correction.rb_corr_th1 = 0;
		cfa_param.cfa_correction.rb_corr_th2 = 0;

		cfa_param.cfa_fcs.fcs_dirsel = 1;
		cfa_param.cfa_fcs.fcs_coring = 8;
		cfa_param.cfa_fcs.fcs_weight = 64;
		for (i = 0; i < KDRV_CFA_FCS_NUM; i++) {
			cfa_param.cfa_fcs.fcs_strength[i] = cfa_fcs_lut[i];
		}

		cfa_param.cfa_ir_hfc.cl_check_enable = TRUE;
		cfa_param.cfa_ir_hfc.hf_check_enable = TRUE;
		cfa_param.cfa_ir_hfc.average_mode = 1;
		cfa_param.cfa_ir_hfc.cl_sel = 0;
		cfa_param.cfa_ir_hfc.cl_th = 0x20;
		cfa_param.cfa_ir_hfc.hfg_th = 0x4;
		cfa_param.cfa_ir_hfc.hf_diff = 0x20;
		cfa_param.cfa_ir_hfc.hfe_th = 0x40;
		cfa_param.cfa_ir_hfc.ir_g_edge_th = 8;
		cfa_param.cfa_ir_hfc.ir_rb_cstrength = 4;

		cfa_param.cfa_ir_sub.ir_sub_r = 256;
		cfa_param.cfa_ir_sub.ir_sub_g = 256;
		cfa_param.cfa_ir_sub.ir_sub_b = 256;
		cfa_param.cfa_ir_sub.ir_sub_wt_lb = 0;
		cfa_param.cfa_ir_sub.ir_sub_th = 128;
		cfa_param.cfa_ir_sub.ir_sub_range = 0;
		cfa_param.cfa_ir_sub.ir_sat_gain = 256;

		cfa_param.cfa_pink_reduc.pink_rd_en = 0;
		cfa_param.cfa_pink_reduc.pink_rd_mode = 0;
		cfa_param.cfa_pink_reduc.pink_rd_th1 = 192;
		cfa_param.cfa_pink_reduc.pink_rd_th2 = 224;
		cfa_param.cfa_pink_reduc.pink_rd_th3 = 255;
		cfa_param.cfa_pink_reduc.pink_rd_th4 = 255;

		cfa_param.cfa_cgain.gain_range = 0;
		cfa_param.cfa_cgain.r_gain = 256;
		cfa_param.cfa_cgain.g_gain = 256;
		cfa_param.cfa_cgain.b_gain = 256;

		memset((void *)&dce_iq, 0, sizeof(KDRV_DCE_PARAM_IQ_ALL_PARAM));
		dce_iq.p_cfa_param = &cfa_param;
		ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_DCE_IQ_PARAM, (void *)&dce_iq);
	}

	/* ipe */
	{
		INT16 cst_coef_init[9] = {77, 150, 29, -43, -85, 128, 128, -107, -21 };
		KDRV_IPE_CST_PARAM cst_param;
		KDRV_IPE_PARAM_IQ_ALL_PARAM ipe_iq;
		UINT32 i;

		cst_param.enable = 1;
		for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
			cst_param.cst_coef[i] = cst_coef_init[i];
		}
		cst_param.cst_off_sel = KDRV_IPE_CST_MINUS128;

		memset((void *)&ipe_iq, 0, sizeof(KDRV_IPE_PARAM_IQ_ALL_PARAM));
		ipe_iq.p_cst = &cst_param;
		ctl_ipp_isp_set(id, CTL_IPP_ISP_ITEM_IPE_IQ_PARAM, (void *)&ipe_iq);
	}

	return E_OK;
}

BOOL nvt_ctl_ipp_api_set_isp_dbg_mode(unsigned char argc, char **pargv)
{
	UINT32 dbg_mode;
	ER rt;

	sscanf(pargv[0], "%d", (int *)&dbg_mode);
	ctl_ipp_dbg_set_isp_mode(dbg_mode);
	rt = ctl_ipp_isp_evt_fp_dbg_mode(dbg_mode);

	if (dbg_mode == 1 && rt == E_OK) {
		ctl_ipp_isp_evt_fp_reg("ipp_dbg_isp", nvt_ctl_ipp_isp_dbg_cb, (ISP_EVENT_PARAM_RST | ISP_EVENT_IPP_CFGSTART), CTL_IPP_ISP_CB_MSG_NONE);
	}

	return rt;
}

BOOL nvt_ctl_ipp_api_saveyuv(unsigned char argc, char **pargv)
{
	UINT32 count;

	sscanf(pargv[2], "%d", (int *)&count);
	CTL_IPP_DBG_DUMP("handle name %s, file_path_base %s, count %d\r\n", pargv[0], pargv[1], (int)(count));
	ctl_ipp_dbg_saveyuv_cfg(pargv[0], pargv[1], count);

	return 0;
}

BOOL nvt_ctl_ipp_api_saveraw(unsigned char argc, char **pargv)
{
	UINT32 id = 0;
	CHAR name[CTL_IPP_HANDLE_NAME_MAX];

	// if there is user input, and the input is not a number, treat input as string of handle name
	if (argc >= 1) {
		if (sscanf_s(pargv[0], "%d", (int *)&id) != 1) { // convert to number fail
			id = -1; // mark as string
		}
	}

	if (id == (UINT32)-1) { // user input is handle name
		ctl_ipp_dbg_saveyuv_cfg(pargv[0], CTL_IPP_INT_ROOT_PATH, CTL_IPP_OUT_PATH_ID_MAX);
	} else { // user input is handle id#
		snprintf(name, CTL_IPP_HANDLE_NAME_MAX, "vdoprc%u", (unsigned int)id);
		ctl_ipp_dbg_saveyuv_cfg(name, CTL_IPP_INT_ROOT_PATH, CTL_IPP_OUT_PATH_ID_MAX);
	}

	return 0;
}

BOOL nvt_ctl_ipp_api_savemem(unsigned char argc, char **pargv)
{
	typedef struct {
		char *name;
		VDO_PXLFMT fmt;
	} CTL_IPP_SAVEMEM_FMT_CMD;

	// command table
	CTL_IPP_SAVEMEM_FMT_CMD fmt_cmd[] = {
		{"444", VDO_PXLFMT_YUV444_PLANAR},
		{"422", VDO_PXLFMT_YUV422},
		{"420", VDO_PXLFMT_YUV420},
		{"y", VDO_PXLFMT_Y8},
		{"nvx2", VDO_PXLFMT_YUV420_NVX2},
	};

	CHAR f_name[80];
	ULONG addr = 0;
	UINT32 width = 0, height = 0, fmt = 0, size = 0, i;

	if (argc < 2) {
		CTL_IPP_DBG_ERR("input error (addr, size) or (addr, width, height, [format])\r\n");
		return 0;
	}

	// read address
	if (sscanf_s(pargv[0], "%lx", &addr) != 1) {
		CTL_IPP_DBG_ERR("read addr(%%x) fail\r\n");
		return 0;
	}

	if (argc >= 4) { // read width, height, format
		if (sscanf_s(pargv[1], "%d", (int *)&width) != 1) {
			CTL_IPP_DBG_ERR("read width(%%d) fail\r\n");
			return 0;
		}
		if (sscanf_s(pargv[2], "%d", (int *)&height) != 1) {
			CTL_IPP_DBG_ERR("read height(%%d) fail\r\n");
			return 0;
		}

		// read format string
		for (i = 0; i < sizeof(fmt_cmd)/sizeof(typeof(fmt_cmd[0])); i++) {
			if (strcmp(pargv[3], fmt_cmd[i].name) == 0) {
				fmt = fmt_cmd[i].fmt;
				break;
			}
		}
		if (fmt == 0) { // if format is not found, read format hex
			if (sscanf_s(pargv[0], "%x", &fmt) != 1) {
				CTL_IPP_DBG_ERR("read fmt(%%x) fail\r\n");
				DBG_DUMP("fmt list:\r\n");
				for (i = 0; i < sizeof(fmt_cmd)/sizeof(typeof(fmt_cmd[0])); i++) {
					DBG_DUMP("%s (0x%x)\r\n", fmt_cmd[i].name, fmt_cmd[i].fmt);
				}
				return 0;
			}
		}

		size = ctl_ipp_util_yuvsize(fmt, width, height);

	} else if (argc >= 3) { // read width, height
		if (sscanf_s(pargv[1], "%d", (int *)&width) != 1) {
			CTL_IPP_DBG_ERR("read width(%%d) fail\r\n");
			return 0;
		}
		if (sscanf_s(pargv[2], "%d", (int *)&height) != 1) {
			CTL_IPP_DBG_ERR("read height(%%d) fail\r\n");
			return 0;
		}
		size = width * height;

	} else { // read size
		if (sscanf_s(pargv[1], "%x", (int *)&size) != 1) {
			CTL_IPP_DBG_ERR("read size(%%x) fail\r\n");
			return 0;
		}
	}

	if (size == 0) {
		CTL_IPP_DBG_ERR("size is 0\r\n");
		return 0;
	}

	if (snprintf(f_name, sizeof(f_name)-1, CTL_IPP_INT_ROOT_PATH"mem_0x%016lx_%ux%u_%x_%u.RAW",
		addr,
		(unsigned int)width,
		(unsigned int)height,
		(unsigned int)size,
		(unsigned int)ctl_ipp_util_get_syst_counter()) >= ((int)sizeof(f_name)-1)) {
		CTL_IPP_DBG_ERR("snprintf fail\r\n");
	}
	ctl_ipp_dbg_savefile(f_name, addr, size);

	return 0;
}

BOOL nvt_ctl_ipp_api_dump_dtsi(unsigned char argc, char **pargv)
{
	ctl_ipp_dbg_dump_dtsi(pargv[0]);
	return 0;
}

BOOL nvt_ctl_ipp_api_fastboot_debug(unsigned char argc, char **pargv)
{
	UINT32 item = 0;

	if (argc > 0) {
		sscanf(pargv[0], "%d", (int *)&item);
	}
	kdrv_ipp_builtin_debug(item, NULL);
	ctl_ipp_dbg_ts_dump_fastboot();

	return 0;
}

BOOL nvt_ctl_ipp_api_dbg_primask(unsigned char argc, char **pargv)
{
	UINT32 mode = 0;

	if (argc > 0) {
		sscanf(pargv[0], "%d", (int *)&mode);
	}

	if (mode <= 1) {
		ctl_ipp_dbg_primask_en(mode);
	} else {
		/* refer to CTL_IPP_DBG_PRIMASK_CFG*/
		if (mode == CTL_IPP_DBG_PRIMASK_CFG_SIZE) {
			USIZE size;

			if (argc > 2) {
				sscanf(pargv[1], "%d", (int *)&size.w);
				sscanf(pargv[2], "%d", (int *)&size.h);
				ctl_ipp_dbg_primask_cfg(mode, (void *)&size);
			}
		}
	}

	return 0;
}

BOOL nvt_ctl_ipp_api_dbg_dma_wp(unsigned char argc, char **pargv)
{
#define CTL_IPP_DBG_DMA_WP_BUF_IN (CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 | CTL_IPP_DBG_DMA_WP_BUF_IFE_IN1 | CTL_IPP_DBG_DMA_WP_BUF_DCE_IN | CTL_IPP_DBG_DMA_WP_BUF_IPE_IN | CTL_IPP_DBG_DMA_WP_BUF_IME_IN | CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN)
#define CTL_IPP_DBG_DMA_WP_BUF_OUT (CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT | CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT | CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT | CTL_IPP_DBG_DMA_WP_BUF_IME_P2_OUT | CTL_IPP_DBG_DMA_WP_BUF_IME_P3_OUT | CTL_IPP_DBG_DMA_WP_BUF_IME_P4_OUT | CTL_IPP_DBG_DMA_WP_BUF_IME_P5_OUT)
#define CTL_IPP_DBG_DMA_WP_BUF_SUBOUT (CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR | CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA | CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR | CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG | CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA)

	typedef struct {
		char *name;
		CTL_IPP_DBG_DMA_WP_BUF_TYPE type;
	} CTL_IPP_DBG_DMA_WP_TYPE_CMD;

	// command table
	CTL_IPP_DBG_DMA_WP_TYPE_CMD type_cmd[] = {
		{"all", 		CTL_IPP_DBG_DMA_WP_BUF_ALL},
		{"in", 			CTL_IPP_DBG_DMA_WP_BUF_IN},
		{"out", 		CTL_IPP_DBG_DMA_WP_BUF_OUT},
		{"inout", 		CTL_IPP_DBG_DMA_WP_BUF_IN | CTL_IPP_DBG_DMA_WP_BUF_OUT},
		{"subout", 		CTL_IPP_DBG_DMA_WP_BUF_SUBOUT},

		{"ife_in0", 	CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0},
		{"ife_in1", 	CTL_IPP_DBG_DMA_WP_BUF_IFE_IN1},
		{"dce_in", 		CTL_IPP_DBG_DMA_WP_BUF_DCE_IN},
		{"ipe_in", 		CTL_IPP_DBG_DMA_WP_BUF_IPE_IN},
		{"ime_in", 		CTL_IPP_DBG_DMA_WP_BUF_IME_IN},
		{"ime_refin", 	CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN},

		{"dce_out", 	CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT},
		{"ipe_out", 	CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT},
		{"ime_out0", 	CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT},
		{"ime_out1", 	CTL_IPP_DBG_DMA_WP_BUF_IME_P2_OUT},
		{"ime_out2", 	CTL_IPP_DBG_DMA_WP_BUF_IME_P3_OUT},
		{"ime_out3", 	CTL_IPP_DBG_DMA_WP_BUF_IME_P4_OUT},
		{"ime_out4", 	CTL_IPP_DBG_DMA_WP_BUF_IME_P5_OUT},

		{"wdr", 		CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR | CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR},
		{"defog", 		CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG | CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG},
		{"ipe_va", 		CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA},
		{"lca", 		CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA | CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA},
		{"mv", 			CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV},
		{"ms", 			CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS},
		{"roi", 		CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI},
		{"pm", 			CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM},
		{"3dnr", 		CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA},
	};

	UINT32 en = 1, narg = 0, i, level = CTL_IPP_DBG_DMA_WP_LEVEL_AUTO;
	CTL_IPP_DBG_DMA_WP_BUF_TYPE type = CTL_IPP_DBG_DMA_WP_BUF_DISABLE;

	if (argc > 0) {
		if (strcmp(pargv[0], "?") == 0) { // show help
			DBG_DUMP("type list:\r\n");
			for (i = 0; i < sizeof(type_cmd)/sizeof(typeof(type_cmd[0])); i++) {
				DBG_DUMP("%s (0x%llx)\r\n", type_cmd[i].name, type_cmd[i].type);
			}
			return 0;

		} else {
			if (sscanf(pargv[0], "%d", (int *)&en) != 1) {
				CTL_IPP_DBG_ERR("scanf en error\r\n");
				return 0;
			}
		}
	}

	if (en == 0) { // disable all protection
		type = CTL_IPP_DBG_DMA_WP_BUF_ALL;

	} else {
		if (argc > 1) {
			for (i = 0; i < sizeof(type_cmd)/sizeof(typeof(type_cmd[0])); i++) {
				if (strcmp(pargv[1], type_cmd[i].name) == 0) { // read string
					type = type_cmd[i].type;
					break;
				}
			}

			if (type == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
				if (sscanf_s(pargv[1], "%x", &narg) == 1) { // if no cmd found in table, read number
					type = narg;
				} else {
					CTL_IPP_DBG_ERR("Unknown type %s\r\n", pargv[1]);

					// show help
					DBG_DUMP("type list:\r\n");
					for (i = 0; i < sizeof(type_cmd)/sizeof(typeof(type_cmd[0])); i++) {
						DBG_DUMP("%s (0x%llx)\r\n", type_cmd[i].name, type_cmd[i].type);
					}

					return 0;
				}
			}

		} else {
			type = CTL_IPP_DBG_DMA_WP_BUF_IN | CTL_IPP_DBG_DMA_WP_BUF_OUT;
		}

		// set protect mode
		if (argc > 2) {
			if (strcmp(pargv[2], "unwrite") == 0) {
				level = CTL_IPP_DBG_DMA_WP_LEVEL_UNWRITE;
			} else if (strcmp(pargv[2], "det") == 0) {
				level = CTL_IPP_DBG_DMA_WP_LEVEL_DETECT;
			} else if (strcmp(pargv[2], "unread") == 0) {
				level = CTL_IPP_DBG_DMA_WP_LEVEL_UNREAD;
			} else if (strcmp(pargv[2], "unrw") == 0) {
				level = CTL_IPP_DBG_DMA_WP_LEVEL_UNRW;
			} else {
				CTL_IPP_DBG_ERR("Unknown level (%s) [unwrite, det, unread, unrw]\r\n", pargv[2]);
			}
		}
	}

	CTL_IPP_DBG_IND("en %u  type 0x%llx\r\n", en, type);
	ctl_ipp_dbg_dma_wp_set_enable(en, type);
	ctl_ipp_dbg_dma_wp_set_prot_level(level);

	return 0;
}

BOOL nvt_ctl_ipp_api_mem_rw(unsigned char argc, char **pargv)
{
	char mode = 'r';
	ULONG addr = 0x0, len = 0x100;
	UINT32 val = 0x0;

	if (argc >= 1) sscanf(pargv[0], "%c", &mode);
	if (argc >= 2) sscanf(pargv[1], "%lx", &addr);

	addr = ALIGN_FLOOR_4(addr); //align to 4 bytes (UINT32)

	if (mode == 'w') {
#if defined(__LINUX)
		void*   map_addr = NULL;
		UINT32  *p_u32;

		if (argc < 3) {
			CTL_IPP_DBG_ERR("no value\r\n");
			return 0;
		}
		sscanf(pargv[2], "%x", &val);

		if ((addr & 0xF0000000) == 0xF0000000) {
			map_addr = ctl_ipp_util_os_ioremap_nocache_wrap(addr, 20);
			if (NULL == map_addr) {
				CTL_IPP_DBG_ERR("ioremap() failed, addr 0x%lx\r\n", addr);
				return 0;
			}
			p_u32 = (UINT32 *)map_addr;
		} else {
			p_u32 = (UINT32 *)addr;
		}

		*p_u32 = val;

		if (NULL != map_addr) {
			ctl_ipp_util_os_iounmap_wrap(map_addr);
		}

#else // RTOS
		UINT32  *p_u32;

		if (argc < 3) {
			CTL_IPP_DBG_ERR("no value\r\n");
			return 0;
		}
		sscanf(pargv[2], "%x", (unsigned int *)&val);

		p_u32 = (UINT32 *)addr;
		*p_u32 = val;
#endif

	} else { // mode == 'r'
		if (argc >= 3) sscanf(pargv[2], "%lx", &len);
		debug_dumpmem(addr, len);
	}

	return 0;
}

BOOL nvt_ctl_ipp_api_dir_tsk_mode(unsigned char argc, char **pargv)
{
	UINT32 en = 1;

	if (argc >= 1) sscanf(pargv[0], "%d", (int *)&en);

	CTL_IPP_DBG_DUMP("set direct task enable %u\r\n", en);

	ctl_ipp_direct_set_tsk_en(en);

	return 0;
}

int kflow_ctl_ipp_init(void)
{
	ctl_ipp_isp_drv_init();
	return 0;
}

