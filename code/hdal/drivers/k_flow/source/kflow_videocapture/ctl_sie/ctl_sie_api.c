#include "kwrap/mem.h"
#include "kwrap/file.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/util.h"
#include "ctl_sie_api.h"
#include "ctl_sie_dbg.h"
#include "ctl_sie_debug_int.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_videocapture/ctl_sie_isp.h"
#include "kflow_videocapture/ctl_sie.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#if defined (__FREERTOS)
#include <stdio.h>
#endif

static SXCMD_BEGIN(ctl_sie, "ctl_sie")
SXCMD_ITEM("dbgtype",		ctl_sie_cmd_set_dbg_type, 			"set ctl sie debug msg type (id, type, par1(opt.),par2(opt.),par3(opt.))")
SXCMD_ITEM("dbglevel",  	ctl_sie_cmd_set_dbg_level, 			"set ctl sie debug level (lvl), plz ref to CTL_SIE_DBG_LVL(1:ERR,2:WRN)")
SXCMD_ITEM("isp_dbgtype",	ctl_sie_cmd_set_isp_dbg_type,		"set isp debug msg type (id, type, par1, par2)")
SXCMD_ITEM("dump_fastboot", ctl_sie_cmd_set_fb_dump, 			"set fast boot info dump info(1: rec_en, 0: dump)")
SXCMD_ITEM("patgen", 		ctl_sie_cmd_force_pat_gen,			"force en sie_pat_gen, only valid when parallel sensor (id, pag_gen_mode, value), set pat_gen_mode = 6 to disable sie pat_gen")
SXCMD_ITEM("flip", 			ctl_sie_cmd_flip,					"h/v flip setting (id, flip)")
SXCMD_ITEM("rst_fc", 		ctl_sie_cmd_rst_frm_cnt,			"reset sie frame count (id, idx)")
#if CTL_SIE_TEST_CMD	//test cmd begin
SXCMD_ITEM("saveraw",   	ctl_sie_cmd_save_raw, 				"save raw out (id)")
SXCMD_ITEM("savemem",   	ctl_sie_cmd_save_mem, 				"save mem (addr, w, h)")
SXCMD_ITEM("getstatis",   	ctl_sie_cmd_get_statis, 			"dump statistic info (id, ca, la, va)")
SXCMD_ITEM("ctlsieon",   	ctl_sie_cmd_on, 					"test cmd, open -> config -> start sie, SIE1(linear) or SIE1/2(shdr)")
SXCMD_ITEM("ctlsieoff",   	ctl_sie_cmd_off, 					"test cmd, stop -> close sie, SIE1(linear) or SIE1/2(shdr)")
SXCMD_ITEM("ctlsieopen",    ctl_sie_cmd_open,  					"test cmd, open sie (id, type)")
SXCMD_ITEM("ctlsieclose",	ctl_sie_cmd_close,					"test cmd, close sie (id)")
SXCMD_ITEM("ctlsiestart", 	ctl_sie_cmd_start,					"test cmd, config and trig start sie (id, bitdepth(opt), pat_gen_mode(opt))")
SXCMD_ITEM("ctlsiestop", 	ctl_sie_cmd_stop,					"test cmd, trig stop sie (id)")
SXCMD_ITEM("ctlsietrig", 	ctl_sie_cmd_trig,					"test cmd, trigger sie start or stop (id, type(1:start,0:stop), frm_num(0xffffffff), wait_end(1))")
SXCMD_ITEM("iosize", 		ctl_sie_cmd_set_iosize,				"test cmd, set i/o size (id, sie_crp_en, auto(0:auto), factor(base:1000), ratio(16:9->0x00100009), sft_x, sft_y, crp_min_w(0), crp_min_h(0))")
SXCMD_ITEM("cfgeth", 		ctl_sie_cmd_isp_cfg_eth,			"test cmd, set eth out (id, en)")	//N.S. eth out in 520
SXCMD_ITEM("testcmd", 		ctl_sie_cmd_test_cmd,				"test cmd")
#endif
SXCMD_END()

BOOL ctl_sie_cmd_set_fb_dump(unsigned char argc, char **pargv)
{
	UINT32 en = 0;
	int fd;
	char tmp_buf[128]="/mnt/sd/nvt-na51055-fastboot-sie.dtsi";

	if (argc >= 1) {
		sscanf_s(pargv[0], "%s", tmp_buf, sizeof(tmp_buf));
	}

	fd = vos_file_open(tmp_buf, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		ctl_sie_dbg_err("open %s failure\r\n", tmp_buf);
		return FALSE;
	}

	ctl_sie_dbg_dump_fb_buf_info(fd, en);
	vos_file_close(fd);
	return 0;
}

BOOL ctl_sie_cmd_set_dbg_type(unsigned char argc, char **pargv)
{
	CTL_SIE_ID id = CTL_SIE_ID_1;
	UINT32 type = CTL_SIE_DBG_MSG_ALL;
	UINT32 par1 = 0, par2 = 0, par3 = 0;
	UINT32 i;

	switch (argc) {
	case 1:
		if (strncmp(pargv[0], "?", strlen(pargv[0])) == 0) {
			DBG_DUMP("---------------------------------------------------------------------\r\n");
			DBG_DUMP("  %s\n", "CTL_SIE_DBG_MSG_TYPE support list");
			DBG_DUMP("  Input Parameters: dbgtype id, type, par1(opt.), par2(opt.) , par3(opt.) \r\n");
			DBG_DUMP("---------------------------------------------------------------------\r\n");
			for (i = 0; i < CTL_SIE_DBG_MSG_MAX; i++) {
				DBG_DUMP("[idx:%2d]%s\r\n", i, dbg_type_str_tab[i].str);
			}
			return 0;
		} else {
			ctl_sie_dbg_wrn("wrong argument:%d, plz input id and msg_type, input ? for dump dbgtype description", (int)(argc));
			sscanf_s(pargv[0], "%d", &id);
		}
		break;

	case 2:
		sscanf_s(pargv[0], "%d", &id);
		sscanf_s(pargv[1], "%d", &type);
		break;

	case 3:
		sscanf_s(pargv[0], "%d", &id);
		sscanf_s(pargv[1], "%d", &type);
		sscanf_s(pargv[2], "%d", &par1);
		break;

	case 4:
		sscanf_s(pargv[0], "%d", &id);
		sscanf_s(pargv[1], "%d", &type);
		sscanf_s(pargv[2], "%d", &par1);
		sscanf_s(pargv[3], "%d", &par2);
		break;

	case 5:
		sscanf_s(pargv[0], "%d", &id);
		sscanf_s(pargv[1], "%d", &type);
		sscanf_s(pargv[2], "%d", &par1);
		sscanf_s(pargv[3], "%d", &par2);
		sscanf_s(pargv[4], "%d", &par3);
		break;

	default:
		nvt_dbg(ERR, "wrong argument:%d, plz input id and msg_type, force dump SIE1 All Msg", argc);
		break;
	}

	ctl_sie_dbg_set_msg_type(id, type, par1, par2, par3);

	return 0;
}

BOOL ctl_sie_cmd_set_dbg_level( unsigned char argc, char **pargv)
{
	CTL_SIE_DBG_LVL dbg_level = CTL_SIE_DBG_LVL_NONE;

	/*[id]*/
	if (argc == 1) {
		sscanf_s(pargv[0], "%d", (int *)&dbg_level);
	} else {
		nvt_dbg(ERR, "wrong argument:%d, plz input debug_level, force set to CTL_SIE_DBG_LVL_ERR", argc);
		dbg_level = CTL_SIE_DBG_LVL_ERR;
	}

	ctl_sie_set_dbg_lvl(dbg_level);

	return 0;
}

BOOL ctl_sie_cmd_set_isp_dbg_type( unsigned char argc, char **pargv)
{
	UINT32 id, type = 0, par1 = 0, par2 = 0;

	if (argc == 3) {
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&type);
		sscanf_s(pargv[2], "%d", (int *)&par1);
	} else if (argc == 4) {
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&type);
		sscanf_s(pargv[2], "%d", (int *)&par1);
		sscanf_s(pargv[3], "%d", (int *)&par2);
	} else {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	DBG_DUMP("Set id:%d , type %d, par: %d, %d\r\n", (int)(id), (int)(type), (int)(par1), (int)(par2));
	ctl_sie_dbg_isp_set_msg_type(id, type, par1, par2);
	return 0;
}

BOOL ctl_sie_cmd_force_pat_gen( unsigned char argc, char **pargv)
{
	CTL_SIE_ID id;
	CTL_SIE_PAG_GEN_INFO pat_gen_info;
	CTL_SIE_HDL *hdl;

	/*[id]*/
	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&pat_gen_info.pat_gen_mode);
	sscanf_s(pargv[2], "%d", (int *)&pat_gen_info.pat_gen_val);

	hdl = ctl_sie_get_hdl(id);
	if (pat_gen_info.pat_gen_mode == CTL_SIE_PAT_MAX) {
		DBG_ERR("id:%d, pat_gen disable \r\n", (int)(id));
	} else {
		DBG_ERR("id: %d, pat_gen _en, mode: %d, value: %d \r\n", (int)(id), (int)(pat_gen_info.pat_gen_mode), (int)(pat_gen_info.pat_gen_val));
	}
	ctl_sie_dbg_set_msg_type(id, CTL_SIE_DBG_MSG_FORCE_PATGEN, 0, 0, 0);

	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);

	return 0;
}

BOOL ctl_sie_cmd_rst_frm_cnt( unsigned char argc, char **pargv)
{
	CTL_SIE_ID id;
	CTL_SIE_ID_IDX id_idx;
	CTL_SIE_HDL *hdl;

	/*[id]*/
	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&id_idx);

	hdl = ctl_sie_get_hdl(id);
	if (id >= CTL_SIE_ID_MAX_NUM || id_idx >= CTL_SIE_ID_IDX_MAX) {
		DBG_ERR("id:%d, idx: %x overflow \r\n", (int)(id), (int)(id_idx));
		return 0;
	}

	DBG_DUMP("reset frame count for idx : %x\r\n", id_idx);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_RESET_FC_IMM, (void *)&id_idx);

	return 0;
}

BOOL ctl_sie_cmd_flip( unsigned char argc, char **pargv)
{
	CTL_SIE_ID id;
	CTL_SIE_FLIP_TYPE flip_type;
	CTL_SIE_HDL *hdl;

	/*[id]*/
	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&flip_type);

	hdl = ctl_sie_get_hdl(id);
	if (id >= CTL_SIE_ID_MAX_NUM || flip_type >= CTL_SIE_FLIP_MAX) {
		DBG_ERR("id:%d, flip_type: %d overflow \r\n", (int)(id), (int)(flip_type));
		return 0;
	}

	DBG_DUMP("set id: %d, flip type: %d\r\n", id, flip_type);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_FLIP, (void *)&flip_type);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);

	return 0;
}

#if 0
#endif

#if  CTL_SIE_TEST_CMD
#define EXAM_CTRL_SIE_USE_NVTMPP (1)
/*============================================================================================
SIE Buffer Size Cal.
============================================================================================*/
#define RAW_ENC_BUF_RATIO 	70	//raw encode minimum buffer ratio is 70
//sie debug info size
#define UNIT_SIE_CAL_DBG_INFO_SIZE()(0x200)
//ch0 raw out
//lineoffset = width * sie_out_bitdepth / 8
//sie_out_bitdepth only support 12bit when raw encode enable
//if raw encode enable: lineoffset = scale_out_h * RAW_ENC_BUF_RATIO / 100
//buf_size = lineoffset * scale_out_h
#define UNIT_SIE_CAL_RAW_BUF_SIZE(lofs, scale_out_h) ALIGN_CEIL_4(lofs * scale_out_h)

//ch0/1 yuv out
//ch0 buf_size = lineoffset_ch0 * crop_h
//ch1 buf_size = lineoffset_ch1 * crop_h
#define UNIT_SIE_CAL_CCIR_BUF_SIZE(lofs, crp_h) ALIGN_CEIL_4(lofs * crp_h)

//ch1 ca, R/G/B/Cnt/IRth, total 5 channel, each channel @16bit
//buf_size = win_w * win_h * 8 * 2
#define UNIT_SIE_CAL_CA_BUF_SIZE() ALIGN_CEIL_4((CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM * 5) << 1)

//ch2 la, PreGamma Lum/PostGamma Lum @16bit
//buf_size = (win_w * win_h * 2 * 2)
#define UNIT_SIE_CAL_LA_BUF_SIZE() ALIGN_CEIL_4((CTL_SIE_ISP_LA_MAX_WINNUM * CTL_SIE_ISP_LA_MAX_WINNUM << 1) << 1)

/*============================================================================================
SIE Function Control
============================================================================================*/
//default statistic setting for prepare public buffer
#define SIE_RING_BUFFER_NUM (3)

CTL_SIE_CA_RSLT	ca_rst;
CTL_SIE_ISP_LA_RSLT	la_rslt;
/*============================================================================================
global parameters
============================================================================================*/
static UINT32 exam_sie_drv_init_cnt;
static BOOL g_sie_buf_io_msg = DISABLE;	//sie buffer control debug msg on/off
static UINT32 g_ipc_sie_buffer[CTL_SIE_ID_MAX_NUM][SIE_RING_BUFFER_NUM];
static UINT32 g_ipc_sie_fc[CTL_SIE_ID_MAX_NUM];
static UINT32 g_ipc_sie_buffer_init_flag;
#if EXAM_CTRL_SIE_USE_NVTMPP
#else
static frammap_buf_t sie_eth_out_buf;
static frammap_buf_t sie_out_buf_info[SIE_RING_BUFFER_NUM];
#endif
#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)
#define CMD_LEN (64)
char cmd_buf[CMD_LEN * 4];
char *cmd[4];

UINT32 *p_buf;
CTL_SIE_HEADER_INFO *sie_head_info;
CTL_SIE_HEADER_INFO head_info;
CTL_SIE_FLOW_TYPE cur_flow_type[CTL_SIE_ID_MAX_NUM] = {
	CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN,
	CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN, CTL_SIE_FLOW_UNKNOWN
};

BOOL ctl_sie_cmd_save_mem( unsigned char argc, char **pargv)
{
	CHAR out_filename[128];
	CHAR root_dir[11];
	UINT32 addr, w, h;

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	sscanf_s(pargv[0], "%d", (int *)&addr);
	sscanf_s(pargv[1], "%d", (int *)&w);
	sscanf_s(pargv[2], "%d", (int *)&h);

#if defined (__LINUX)
		snprintf(root_dir, sizeof(root_dir), "//mnt//sd//");
#elif defined (__FREERTOS)
		snprintf(root_dir, sizeof(root_dir), "A:\\");
#endif

	if (addr != 0 && w != 0 && h != 0) {
		snprintf(out_filename, 128, "%sts%lld_addr_0x%x_w_%d_h_%d.RAW", root_dir, (long long)ctl_sie_util_get_timestamp(), (unsigned int)addr, (int)w, (int)h);
		ctl_sie_dbg_savefile(out_filename, addr, w*h);
		DBG_ERR("SAVE Addr: (0x%.8x), Size(%d %d)\r\n", (unsigned int)addr, (int)w, (int)h);
	}

	return 0;
}

BOOL ctl_sie_cmd_save_raw( unsigned char argc, char **pargv)
{
	CTL_SIE_ISP_HEADER_INFO head = {0};
	UINT32 id;
    CHAR out_filename[128];
	CHAR root_dir[11];
	VDO_PXLFMT pxfmt;

	/*[id]*/
	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}
	sscanf_s(pargv[0], "%d", (int *)&id);

#if defined (__LINUX)
		snprintf(root_dir, sizeof(root_dir), "//mnt//sd//");
#elif defined (__FREERTOS)
		snprintf(root_dir, sizeof(root_dir), "A:\\");
#endif

	if (ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_IMG_OUT, (void *)&head) == E_OK) {
		pxfmt = head.vdo_frm.pxlfmt & (~VDO_PIX_YCC_MASK);
		if (pxfmt == VDO_PXLFMT_YUV422 || pxfmt == VDO_PXLFMT_YUV420) {
			snprintf(out_filename, 128, "%sts%lld_y_id%d_0x%x_%d_%d.RAW", root_dir, (long long)ctl_sie_util_get_timestamp(), (int)id, (unsigned int)head.vdo_frm.addr[0], (int)head.vdo_frm.loff[0], (int)head.vdo_frm.size.h);
			ctl_sie_dbg_savefile(out_filename, head.vdo_frm.addr[0], (head.vdo_frm.loff[0]*head.vdo_frm.ph[0]));
			nvt_dbg(WRN, "save y image id: %d, lofs: %d, h: %d\r\n", (int)id, (int)head.vdo_frm.loff[0], (int)head.vdo_frm.ph[0]);
		snprintf(out_filename, 128, "%sts%lld_uv_id%d_0x%x_%d_%d.RAW", root_dir, (long long)ctl_sie_util_get_timestamp(), (int)id, (unsigned int)head.vdo_frm.addr[1], (int)head.vdo_frm.loff[0], (int)head.vdo_frm.size.h);
			ctl_sie_dbg_savefile(out_filename, head.vdo_frm.addr[1], (head.vdo_frm.loff[1]*head.vdo_frm.ph[1]));
			nvt_dbg(WRN, "save uv image id: %d, lofs: %d, h: %d\r\n", (int)id, (int)head.vdo_frm.loff[1], (int)head.vdo_frm.ph[1]);
		} else {
			snprintf(out_filename, 128, "%sts%lld_raw_id%d_0x%x_%d_%d.RAW", root_dir, (long long)ctl_sie_util_get_timestamp(), (int)id, (unsigned int)head.vdo_frm.addr[0], (int)head.vdo_frm.loff[0], (int)head.vdo_frm.size.h);
			ctl_sie_dbg_savefile(out_filename, head.vdo_frm.addr[0], (head.vdo_frm.loff[0]*head.vdo_frm.ph[0]));
			nvt_dbg(WRN, "save raw image addr %x, ts: %lld id: %d, lofs: %d, h: %d\r\n", (unsigned int)head.vdo_frm.addr[0], (long long)ctl_sie_util_get_timestamp(), (int)id, (int)head.vdo_frm.loff[0], (int)head.vdo_frm.ph[0]);
		}
		ctl_sie_isp_set(id, CTL_SIE_ISP_ITEM_IMG_OUT, (void *)&head);
	} else {
		nvt_dbg(ERR, "get img timeout %d\r\n", (int)id);
	}

	return 0;
}

/*============================================================================================
Exam API
============================================================================================*/
/**
	query buffer and init ctl sie
*/
struct vos_mem_cma_info_t ctl_sie_test_cma_info;
VOS_MEM_CMA_HDL ctl_sie_test_cma_hdl;
static void exam_ctrl_sie_drv_init(void)
{
	UINT32 buf_size;

	if (exam_sie_drv_init_cnt == 0) {
		buf_size = ctl_sie_buf_query(CTL_SIE_MAX_SUPPORT_ID);
		if (vos_mem_init_cma_info(&ctl_sie_test_cma_info, VOS_MEM_CMA_TYPE_CACHE, buf_size) != 0) {
			DBG_ERR("init cma failed\r\n");
			return;
		}
		ctl_sie_test_cma_hdl = vos_mem_alloc_from_cma(&ctl_sie_test_cma_info);
		if (ctl_sie_test_cma_hdl == NULL) {
			DBG_ERR("alloc sie test cmd cma failed\r\n");
			return;
		}
		ctl_sie_init(ctl_sie_test_cma_info.vaddr, buf_size);
	}
	exam_sie_drv_init_cnt++;
}

/**
	uninit ctl sie and free buffer
*/
static void exam_ctrl_sie_drv_uninit(void)
{
#if CTL_SIE_TEST_DIRECT_FLOW
#else
	if (exam_sie_drv_init_cnt > 0) {
		exam_sie_drv_init_cnt--;
		if (exam_sie_drv_init_cnt == 0) {
			ctl_sie_uninit();
			if (vos_mem_release_from_cma(ctl_sie_test_cma_hdl) != 0) {
				DBG_ERR("release sie test cmd cma failed\r\n");
			}
		}
	}
#endif
}

#define BUF_IO_DUMP_PERIOD 60
static INT32 exam_ctrl_sie_common_bufio_cb(UINT32 id, UINT32 sts, void *in_data, void *out_data)
{
	UINT32 idx;
	//new buffer for sie
	UINT32 *size = (UINT32 *)in_data;

	switch (sts) {
		case CTL_SIE_BUF_IO_NEW:
			idx = (g_ipc_sie_fc[id] % SIE_RING_BUFFER_NUM);
			sie_head_info = (CTL_SIE_HEADER_INFO *)out_data;
			sie_head_info->buf_addr = g_ipc_sie_buffer[id][idx];
			sie_head_info->buf_id = 0xff;
			if (g_sie_buf_io_msg && (g_ipc_sie_fc[id] % BUF_IO_DUMP_PERIOD == 0)) {
				DBG_DUMP("NEW: size: %#x,addr: %#x\r\n", (unsigned int)*size, (unsigned int)sie_head_info->buf_addr);
			}
			g_ipc_sie_fc[id]++;
			break;
		case CTL_SIE_BUF_IO_PUSH:
			head_info = *(CTL_SIE_HEADER_INFO *)in_data;
			if (g_sie_buf_io_msg && (g_ipc_sie_fc[id] % BUF_IO_DUMP_PERIOD == 0)) {
				DBG_DUMP("PUSH %#x, %d, %d, %d, fc: %llu, ts: %llu\r\n", (unsigned int)head_info.buf_addr, (int)head_info.vdo_frm.pw[0], (int)head_info.vdo_frm.ph[0], (int)head_info.vdo_frm.loff[0], (long long)head_info.vdo_frm.count, (long long)head_info.vdo_frm.timestamp);
			}
			break;
		case CTL_SIE_BUF_IO_UNLOCK:
			if (g_sie_buf_io_msg && (g_ipc_sie_fc[id] % BUF_IO_DUMP_PERIOD == 0)) {
				sie_head_info = (CTL_SIE_HEADER_INFO *)in_data;
				DBG_DUMP("\r\nUNLOCK %#x, %lld\r\n\r\n", (unsigned int)sie_head_info->buf_addr, (long long)sie_head_info->vdo_frm.count);
			}
			break;
		case CTL_SIE_BUF_IO_LOCK:
			if (g_sie_buf_io_msg && (g_ipc_sie_fc[id] % BUF_IO_DUMP_PERIOD == 0)) {
				sie_head_info = (CTL_SIE_HEADER_INFO *)in_data;
				DBG_DUMP("\r\nLOCK %x, %lld\r\n\r\n", (unsigned int)(sie_head_info->buf_addr), (long long)(sie_head_info->vdo_frm.count));
			}
			break;
	}
	return 0;
}

static INT32 ctrl_sie1_bufio_cb(UINT32 sts, void *in_data, void *out_data)
{
	return exam_ctrl_sie_common_bufio_cb(CTL_SIE_ID_1, sts, in_data, out_data);
}

static INT32 ctrl_sie2_bufio_cb(UINT32 sts, void *in_data, void *out_data)
{
	return exam_ctrl_sie_common_bufio_cb(CTL_SIE_ID_2, sts, in_data, out_data);
}

static INT32 ctrl_sie3_bufio_cb(UINT32 sts, void *in_data, void *out_data)
{
	return exam_ctrl_sie_common_bufio_cb(CTL_SIE_ID_3, sts, in_data, out_data);
}

static INT32 ctrl_sie4_bufio_cb(UINT32 sts, void *in_data, void *out_data)
{
	return exam_ctrl_sie_common_bufio_cb(CTL_SIE_ID_4, sts, in_data, out_data);
}

static INT32 ctrl_sie5_bufio_cb(UINT32 sts, void *in_data, void *out_data)
{
	return exam_ctrl_sie_common_bufio_cb(CTL_SIE_ID_5, sts, in_data, out_data);
}

static INT32 ctrl_sie_isr_cb(UINT32 msg, void *in, void *out)
{
//	DBG_ERR("---- EXAM CTRL SIE ISR msg = %x ----\r\n", (unsigned int)(*(UINT32 *)in));
	return 0;
}

static BOOL exam_sie_ctrl_open( unsigned char argc, char **pargv)
{
	UINT32 id = 0;
	CTL_SIE_OPEN_CFG open_cfg;
	CTL_SIE_MCLK sie_mclk;
	UINT32 flow_type;
	UINT32 sie_hdl;

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&flow_type);


	memset((void *)&open_cfg, 0, sizeof(CTL_SIE_OPEN_CFG));

	exam_ctrl_sie_drv_init();
	sie_mclk.mclk_en = ENABLE;
	sie_mclk.mclk_id_sel = CTL_SIE_ID_MCLK;
	// Must set the same clock rate as ssenif reference rate
	open_cfg.id = id;
	open_cfg.flow_type = (CTL_SIE_FLOW_TYPE)flow_type;
	open_cfg.dupl_src_id = id;
	sie_hdl = ctl_sie_open((void *)&open_cfg);
	if (sie_hdl == 0) {
		DBG_ERR("ctl_sie_open open fail\r\n");
		return FALSE;
	}
	cur_flow_type[id] = (CTL_SIE_FLOW_TYPE)flow_type;

	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_mclk);
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

	return TRUE;
}

static BOOL exam_sie_ctrl_close( unsigned char argc, char **pargv)
{
	UINT32 id = 0;
	CTL_SIE_HDL *hdl;
	ER rt;

	sscanf_s(pargv[0], "%d", (int *)&id);
	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}

	rt = ctl_sie_close((UINT32)hdl);

	if (rt == E_OK) {
		g_ipc_sie_buffer_init_flag &= ~(1 << id);
		exam_ctrl_sie_drv_uninit();
		#if EXAM_CTRL_SIE_USE_NVTMPP
			#if CTL_SIE_TEST_DIRECT_FLOW
			#else
			if (g_ipc_sie_buffer_init_flag == 0) {
				nvtmpp_vb_exit();
			}
			#endif
		#else
		{
			UINT32 i;
			UINT32 total_buf_num;

			exam_ctrl_sie_drv_uninit();
			total_buf_num = SIE_RING_BUFFER_NUM;
			for (i = 0; i < total_buf_num; i++) {
				frm_free_buf_ddr(g_ipc_sie_buffer[id][i]);
			}
		}
		#endif
		cur_flow_type[id] = CTL_SIE_FLOW_UNKNOWN;

		return TRUE;
	} else {
		return FALSE;
	}
}

static void ctrl_sie_config_buffer(CTL_SIE_ID id)
{
	UINT32 i;
	CTL_SIE_HDL *hdl;
	UINT32 buf_size[CTL_SIE_CH_MAX] = {0};
	UINT32 dbg_info_size = 0;
	UINT32 total_buf_size = 0;
	CTL_SIE_DATAFORMAT data_fmt;
	UINT32 lofs_ch0 = 0, lofs_ch1 = 0;
	CTL_SIE_IO_SIZE_INFO io_size;
	BOOL encode = 0;
	CTL_SIE_ALG_FUNC func_en;
#if EXAM_CTRL_SIE_USE_NVTMPP
	NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('s', 'i', 'e', 't', 'e', 's', 't', 0);
	NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
	NVTMPP_DDR     ddr = NVTMPP_DDR_1;
	NVTMPP_VB_BLK  blk;
	UINT32 nvtmpp_size, nvtmpp_addr;
#if CTL_SIE_TEST_DIRECT_FLOW
#else
	NVTMPP_VB_CONF_S st_conf;
#endif
#endif

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_ENCODE, (void *)&encode);
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);

	dbg_info_size = UNIT_SIE_CAL_DBG_INFO_SIZE();
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);

	if (data_fmt == CTL_SIE_YUV_422_SPT || data_fmt == CTL_SIE_YUV_420_SPT) {
		//CCIR
		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_CH0_LOF, (void *)(&lofs_ch0));
		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_CH1_LOF, (void *)(&lofs_ch1));

		buf_size[0] = UNIT_SIE_CAL_CCIR_BUF_SIZE(lofs_ch0, io_size.size_info.sie_crp.h);
		buf_size[1] = UNIT_SIE_CAL_CCIR_BUF_SIZE(lofs_ch1, io_size.size_info.sie_crp.h);
	} else if (data_fmt == CTL_SIE_YUV_422_NOSPT) {
		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_CH0_LOF, (void *)(&lofs_ch0));
		buf_size[0] = UNIT_SIE_CAL_CCIR_BUF_SIZE(lofs_ch0, io_size.size_info.sie_crp.h);
	} else {		//RAW
		//ch0
		//if raw encode enable: buf_size = lineoffset * sie_scl_out_h * 70/100
		//if raw encode disable: buf_size = lineoffset * sie_scl_out_h
#if CTL_SIE_TEST_DIRECT_FLOW
#else
		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_CH0_LOF, (void *)(&lofs_ch0));

		buf_size[0] = sizeof(VDO_FRAME);
		buf_size[0] += UNIT_SIE_CAL_RAW_BUF_SIZE(lofs_ch0, io_size.size_info.sie_scl_out.h);
#endif
		if (CTL_SIE_TEST_FUNC_EN & CTL_SIE_TEST_FUNC_CA) {
			func_en.type = CTL_SIE_ALG_TYPE_AWB;
			func_en.func_en = ENABLE;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_ALG_FUNC_IMM, (void *)&func_en);
			buf_size[1] = UNIT_SIE_CAL_CA_BUF_SIZE();
		}
	}

	if (CTL_SIE_TEST_FUNC_EN & CTL_SIE_TEST_FUNC_LA) {
		func_en.type = CTL_SIE_ALG_TYPE_AE;
		func_en.func_en = ENABLE;
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_ALG_FUNC_IMM, (void *)&func_en);
		buf_size[2] = UNIT_SIE_CAL_LA_BUF_SIZE();
	}

	total_buf_size = dbg_info_size + buf_size[0] + buf_size[1] + buf_size[2];
	DBG_DUMP("buffer_size[%d] = 0x%.8x, 0x%.8x, 0x%.8x, total %x\r\n", (int)id,
			 (unsigned int)buf_size[0], (unsigned int)buf_size[1], (unsigned int)buf_size[2], (unsigned int)total_buf_size);

#if EXAM_CTRL_SIE_USE_NVTMPP
	#if CTL_SIE_TEST_DIRECT_FLOW
	#else
		if (g_ipc_sie_buffer_init_flag == 0) {
			memset(&st_conf, 0, sizeof(NVTMPP_VB_CONF_S));
			st_conf.max_pool_cnt = 64;
		    st_conf.common_pool[0].blk_size = total_buf_size;
			st_conf.common_pool[0].blk_cnt = SIE_RING_BUFFER_NUM * CTL_SIE_MAX_SUPPORT_ID;
			st_conf.common_pool[0].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[0].type = POOL_TYPE_COMMON;

		    st_conf.common_pool[1].blk_size = io_size.size_info.sie_crp.w*io_size.size_info.sie_crp.h;
			st_conf.common_pool[1].blk_cnt = 1;
			st_conf.common_pool[1].ddr = NVTMPP_DDR_1;
			st_conf.common_pool[1].type = POOL_TYPE_COMMON;

			nvtmpp_vb_set_conf(&st_conf);
			nvtmpp_vb_init();
		}
		g_ipc_sie_buffer_init_flag |= (1 << id);
	#endif
		nvtmpp_vb_add_module(module);

		nvtmpp_size = total_buf_size;
		for (i = 0; i < SIE_RING_BUFFER_NUM; i++) {
			blk = nvtmpp_vb_get_block(module, pool, nvtmpp_size, ddr);
			if (blk == NVTMPP_VB_INVALID_BLK) {
				DBG_DUMP("get input buffer fail\n");
			}
			nvtmpp_addr = nvtmpp_vb_blk2va(blk);
			DBG_DUMP("input buffer addr 0x%.8x size 0x%.8x\r\n", (unsigned int)nvtmpp_addr, (unsigned int)nvtmpp_size);
			g_ipc_sie_buffer[id][i] = nvtmpp_addr;
		}
#else
	if ((g_ipc_sie_buffer_init_flag & (1 << id)) == 0) {
		g_ipc_sie_buffer_init_flag |= (1 << id);
		for (i = 0; i < SIE_RING_BUFFER_NUM; i++) {
			/*get in buffer*/
			sie_out_buf_info[i].size = total_buf_size;
			sie_out_buf_info[i].align = 64;      ///< address alignment
			sie_out_buf_info[i].name = "ctrl_sie_out";
			sie_out_buf_info[i].alloc_type = ALLOC_CACHEABLE;
			if (frm_get_buf_ddr(DDR_ID0, &sie_out_buf_info[i])) {
				DBG_ERR("get input buffer fail\n");
				return;
			}
			DBG_IND("sie out buffer addr 0x%.8x(0x%.8x) size 0x%.8x\r\n", (unsigned int)sie_out_buf_info[i].va_addr, (unsigned int)sie_out_buf_info[i].phy_addr, (unsigned int)sie_out_buf_info[i].size);

			g_ipc_sie_buffer[id][i] = (UINT32)sie_out_buf_info[i].va_addr;
		}
	}
#endif
}

static void sie_config_encode(CTL_SIE_ID id)
{
	BOOL raw_encode;
	CTL_SIE_HDL *hdl;

	raw_encode = TRUE;
	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_ENCODE, (void *)&raw_encode);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
}

#define CTL_SIE_IO_SIZE_TEST DISABLE
#define CTL_SIE_IO_SIZE_TEST_ADV DISABLE
#if CTL_SIE_IO_SIZE_TEST
static void exam_ctl_sie_dump_io_size_set(CTL_SIE_IO_SIZE_INFO io_size)
{
	DBG_IND("(set)crp_sel(%d)\t,iosz_sel(%d)\t,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
		, io_size.auto_info.crp_sel, io_size.iosize_sel
		, io_size.auto_info.ratio_h_v, io_size.auto_info.factor
		, io_size.auto_info.sft.x, io_size.auto_info.sft.y
		, io_size.auto_info.sie_crp_min.w, io_size.auto_info.sie_crp_min.h);
	DBG_IND("(set)size_info(siecrp(x%d,y%d,w%d,h%d)\t,dest_crp(x%d,y%d,w%d,h%d)\t,align(w%d,h%d)\r\n"
		, io_size.size_info.sie_crp.x, io_size.size_info.sie_crp.y, io_size.size_info.sie_crp.w, io_size.size_info.sie_crp.h
		, io_size.size_info.dest_crp.x, io_size.size_info.dest_crp.y, io_size.size_info.dest_crp.w, io_size.size_info.dest_crp.h
		, io_size.align.w, io_size.align.h);
}
static void exam_ctl_sie_dump_io_size_get(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	DBG_IND("(get)crp_sel(%d)\t,iosz_sel(%d)\t,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
		, io_size.auto_info.crp_sel, io_size.iosize_sel
		, io_size.auto_info.ratio_h_v, io_size.auto_info.factor
		, io_size.auto_info.sft.x, io_size.auto_info.sft.y
		, io_size.auto_info.sie_crp_min.w, io_size.auto_info.sie_crp_min.h);
	DBG_IND("(get)size_info(siecrp(x%d,y%d,w%d,h%d)\t,dest_crp(x%d,y%d,w%d,h%d)\t,align(w%d,h%d)\r\n"
		, io_size.size_info.sie_crp.x, io_size.size_info.sie_crp.y, io_size.size_info.sie_crp.w, io_size.size_info.sie_crp.h
		, io_size.size_info.dest_crp.x, io_size.size_info.dest_crp.y, io_size.size_info.dest_crp.w, io_size.size_info.dest_crp.h
		, io_size.align.w, io_size.align.h);
}

/*
	sensor:
	mode_basic->size_info.valid.w = 1920;
	mode_basic->size_info.valid.h = 1080;
	mode_basic->size_info.act.x = 0;
	mode_basic->size_info.act.y = 6;
	mode_basic->size_info.act.w = 1920;
	mode_basic->size_info.act.h = 1080;
	mode_basic->size_info.crp.w = 1900;
	mode_basic->size_info.crp.h = 1040;
*/
static void exam_ctl_sie_io_size_test_1(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 248, y 20, w 1424, h 1040,
		sie scl: w 1424, h 1040,
		dest crp: x 0, y 0, w 1424, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(4, 3);	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 3 ============================\r\n");
	/*
		sie crp: x 580, y 332, w 760, h 416,
		sie scl: w 760, h 416,
		dest crp: x 0, y 0, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 4 ============================\r\n");
	/*
		sie crp: x 16, y 34, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 6;	//
	io_size.auto_info.sft.y = 14;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 5 ============================\r\n");
	/*
		sie crp: x 320, y 180, w 1280, h 720,
		sie scl: w 1280, h 720,
		dest crp: x 260, y 152, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 1280;	//
	io_size.auto_info.sie_crp_min.h = 720;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 6 ============================\r\n");
	/*
		sie crp: x 12, y 20, w 1896, h 1040,
		sie scl: w 1896, h 1040,
		dest crp: x 0, y 0, w 1896, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 8;	//
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 7 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 96, y 52, w 1708, h 936,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 8 ============================\r\n");
	/*
		sie crp: x 12, y 20, w 1896, h 1036,
		sie scl: w 1896, h 1036,
		dest crp: x 0, y 0, w 1896, h 1036,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 7;	//
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 9 ============================\r\n");
	/*
		sie crp: x 600, y 70, w 800(640), h 1000(624),
		sie scl: w 800, h 1000,
		dest crp: x 80, y 188, w 640, h 624,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 600;	//
	io_size.auto_info.sft.x = 40;	//
	io_size.auto_info.sft.y = 30;	//
	io_size.auto_info.sie_crp_min.w = 800;	//
	io_size.auto_info.sie_crp_min.h = 1000;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 10 ============================\r\n");
	/*
		sie crp: x 774, y 358, w 372, h 364,
		sie scl: w 372, h 364,
		dest crp: x 0, y 0, w 372, h 364,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 350;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 11 ============================\r\n");
	/*
		sie crp: x 2, y 18, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = -8;	//
	io_size.auto_info.sft.y = -2;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);


	DBG_ERR("============================ pattern 12 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_DFT;	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

}

static void exam_ctl_sie_io_size_test_2(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 248, y 20, w 1424, h 1040,
		sie scl: w 1424, h 1040,
		dest crp: x 0, y 0, w 1424, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(4, 3);	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 3 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 570, y 312, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 4 ============================\r\n");
	/*
		sie crp: x 16, y 34, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 6;	//
	io_size.auto_info.sft.y = 14;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 5 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1080,
		dest crp: x 570, y 312, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 1280;	//
	io_size.auto_info.sie_crp_min.h = 720;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 6 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1896, h 1040,
		sie scl: w 1896, h 1040,
		dest crp: x 0, y 0, w 1896, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 8;	//
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 7 ============================\r\n");
	/*
		sie crp: x 12, y 22, w 1896, h 1036,
		sie scl: w 1896, h 1036,
		dest crp: x 12, y 8, w 1896, h 1036,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 7;	//
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 8 ============================\r\n");
	/*
		sie crp: x 456, y 50, w 1068, h 1040,
		sie scl: w 1068, h 1040,
		dest crp: x 214, y 208, w 640, h 624,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 600;	//
	io_size.auto_info.sft.x = 40;	//
	io_size.auto_info.sft.y = 30;	//
	io_size.auto_info.sie_crp_min.w = 800;	//
	io_size.auto_info.sie_crp_min.h = 1000;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 9 ============================\r\n");
	/*
		sie crp: x 426, y 20, w 1068, h 1040,
		sie scl: w 1068, h 1040,
		dest crp: x 348, y 338, w 372, h 364,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 350;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 10 ============================\r\n");
	/*
		sie crp: x 2, y 18, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = -8;	//
	io_size.auto_info.sft.y = -2;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);


	DBG_ERR("============================ pattern 11 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_DFT;	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

}

static void exam_ctl_sie_io_size_test_3(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 5, y 8, w 1908, h 1048,
		sie scl: w 1908, h 1048,
		dest crp: x 20, y 40, w 1280, h 720,
	*/
	io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.size_info.sie_crp.x = 5;
	io_size.size_info.sie_crp.y = 8;
	io_size.size_info.sie_crp.w = 1910;
	io_size.size_info.sie_crp.h = 1050;
	io_size.size_info.dest_crp.x = 20;
	io_size.size_info.dest_crp.y = 40;
	io_size.size_info.dest_crp.w = 1280;
	io_size.size_info.dest_crp.h = 720;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 5, y 8, w 1900, h 1008,
		sie scl: w 1900, h 1008,
		dest crp: x 30, y 40, w 640, h 720,
	*/
	io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
	io_size.align.w = 5;
	io_size.align.h = 9;
	io_size.size_info.sie_crp.x = 5;
	io_size.size_info.sie_crp.y = 8;
	io_size.size_info.sie_crp.w = 1913;
	io_size.size_info.sie_crp.h = 1037;
	io_size.size_info.dest_crp.x = 30;
	io_size.size_info.dest_crp.y = 40;
	io_size.size_info.dest_crp.w = 646;
	io_size.size_info.dest_crp.h = 726;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	exam_ctl_sie_dump_io_size_get(id);
}
#endif

#if CTL_SIE_IO_SIZE_TEST_ADV
USIZE test_dest_ext_crp_prop = {100, 100};
USIZE test_sie_scl_max_sz = {0};
USIZE test_dest_align = {0};
USIZE test_scl_sz = {0};
static void exam_ctl_sie_dump_io_size_set(CTL_SIE_IO_SIZE_INFO io_size)
{
	DBG_IND("(set)crp_sel(%d)\t,iosz_sel(%d)\t,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
		, io_size.auto_info.crp_sel, io_size.iosize_sel
		, io_size.auto_info.ratio_h_v, io_size.auto_info.factor
		, io_size.auto_info.sft.x, io_size.auto_info.sft.y
		, io_size.auto_info.sie_crp_min.w, io_size.auto_info.sie_crp_min.h);
	DBG_IND("(set)size_info(siecrp(x%d,y%d,w%d,h%d)\t,dest_crp(x%d,y%d,w%d,h%d)\t,align(w%d,h%d)\r\n"
		, io_size.size_info.sie_crp.x, io_size.size_info.sie_crp.y, io_size.size_info.sie_crp.w, io_size.size_info.sie_crp.h
		, io_size.size_info.dest_crp.x, io_size.size_info.dest_crp.y, io_size.size_info.dest_crp.w, io_size.size_info.dest_crp.h
		, io_size.align.w, io_size.align.h);
}
static void exam_ctl_sie_dump_io_size_get(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	DBG_IND("(get)crp_sel(%d)\t,iosz_sel(%d)\t,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
		, io_size.auto_info.crp_sel, io_size.iosize_sel
		, io_size.auto_info.ratio_h_v, io_size.auto_info.factor
		, io_size.auto_info.sft.x, io_size.auto_info.sft.y
		, io_size.auto_info.sie_crp_min.w, io_size.auto_info.sie_crp_min.h);
	DBG_IND("(get)size_info(siecrp(x%d,y%d,w%d,h%d)\t,scl_size(w%d,h%d)\t,dest_crp(x%d,y%d,w%d,h%d)\t,align(w%d,h%d)\r\n"
		, io_size.size_info.sie_crp.x, io_size.size_info.sie_crp.y, io_size.size_info.sie_crp.w, io_size.size_info.sie_crp.h
		, io_size.size_info.sie_scl_out.w, io_size.size_info.sie_scl_out.h
		, io_size.size_info.dest_crp.x, io_size.size_info.dest_crp.y, io_size.size_info.dest_crp.w, io_size.size_info.dest_crp.h
		, io_size.align.w, io_size.align.h);
}

/*
	sensor:
	mode_basic->size_info.valid.w = 1920;
	mode_basic->size_info.valid.h = 1080;
	mode_basic->size_info.act.x = 0;
	mode_basic->size_info.act.y = 6;
	mode_basic->size_info.act.w = 1920;
	mode_basic->size_info.act.h = 1080;
	mode_basic->size_info.crp.w = 1900;
	mode_basic->size_info.crp.h = 1040;
*/
static void exam_ctl_sie_io_size_test_adv_1(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 248, y 20, w 1424, h 1040,
		sie scl: w 1424, h 1040,
		dest crp: x 0, y 0, w 1424, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(4, 3);	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 3 ============================\r\n");
	/*
		sie crp: x 580, y 332, w 760, h 416,
		sie scl: w 760, h 416,
		dest crp: x 0, y 0, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 4 ============================\r\n");
	/*
		sie crp: x 16, y 34, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 6;	//
	io_size.auto_info.sft.y = 14;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 5 ============================\r\n");
	/*
		sie crp: x 320, y 180, w 1280, h 720,
		sie scl: w 1280, h 720,
		dest crp: x 260, y 152, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 1280;	//
	io_size.auto_info.sie_crp_min.h = 720;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 6 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1896, h 1040,
		dest crp: x 0, y 0, w 1896, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 8;	//
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 7 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 96, y 52, w 1708, h 936,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 10;
	io_size.auto_info.dest_ext_crp_prop.h = 10;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 8 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1280, h 720,
		dest crp: x 0, y 0, w 1280, h 720,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = 1280;
	io_size.auto_info.sie_scl_max_sz.h = 720;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 9 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 14, y 0, w 1872, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = 9;
	io_size.dest_align.h = 10;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 10 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1896, h 1036,
		dest crp: x 12, y 8, w 1872, h 1020,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 7;	//
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = 9;
	io_size.dest_align.h = 10;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 11 ============================\r\n");
	/*
		sie crp: x 600((1920-800)/2+40), y 70((1080-1000)/2+30), w 800(640), h 1000(624),
		sie scl: w 500, h 400,
		dest crp: x 50, y 76, w 400(500 - (160 * 500 / 800)), h 248(400 - (376 * 400 / 100)),
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 600;	//
	io_size.auto_info.sft.x = 40;	//
	io_size.auto_info.sft.y = 30;	//
	io_size.auto_info.sie_crp_min.w = 800;	//
	io_size.auto_info.sie_crp_min.h = 1000;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = 500;
	io_size.auto_info.sie_scl_max_sz.h = 400;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 12 ============================\r\n");
	/*
		sie crp: x 774, y 358, w 372, h 364,
		sie scl: w 372, h 364,
		dest crp: x 0, y 0, w 372, h 364,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 350;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 13 ============================\r\n");
	/*
		sie crp: x 2, y 18, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = -8;	//
	io_size.auto_info.sft.y = -2;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);


	DBG_ERR("============================ pattern 14 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_DFT;	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

}

static void exam_ctl_sie_io_size_test_adv_2(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 248, y 20, w 1424, h 1040,
		sie scl: w 1424, h 1040,
		dest crp: x 0, y 0, w 1424, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(4, 3);	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 3 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 570, y 312, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 4 ============================\r\n");
	/*
		sie crp: x 16, y 34, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 6;	//
	io_size.auto_info.sft.y = 14;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 5 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 570, y 312, w 760, h 416,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 400;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 1280;	//
	io_size.auto_info.sie_crp_min.h = 720;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 6 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1896, h 1040,
		dest crp: x 0, y 0, w 1896, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 8;	//
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 7 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 96, y 52, w 1708, h 936,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 10;
	io_size.auto_info.dest_ext_crp_prop.w = 10;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 8 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1280, h 720,
		dest crp: x 0, y 0, w 1280, h 720,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = 1280;
	io_size.auto_info.sie_scl_max_sz.h = 720;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 9 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 14, y 0, w 1872, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = 9;
	io_size.dest_align.h = 10;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 10 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1896, h 1036,
		dest crp: x 12, y 8, w 1872, h 1020,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = 6;	//
	io_size.align.h = 7;	//
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = 9;
	io_size.dest_align.h = 10;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 11 ============================\r\n");
	/*
		sie crp: x 466, y 40(20+20+1040 = 1080(max clamp for active size)), w 1068, h 1040,
		sie scl: w 500, h 400,
		dest crp: x 100, y 80+10*(500/1040), w 300, h 240,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 600;	//
	io_size.auto_info.sft.x = 40;	//
	io_size.auto_info.sft.y = 30;	//
	io_size.auto_info.sie_crp_min.w = 800;	//
	io_size.auto_info.sie_crp_min.h = 1000;	//
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = 500;
	io_size.auto_info.sie_scl_max_sz.h = 400;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 12 ============================\r\n");
	/*
		sie crp: x 426, y 20, w 1068, h 1040,
		sie scl: w 1068, h 1040,
		dest crp: x 348, y 338, w 372, h 364,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(9, 9);	//
	io_size.auto_info.factor = 350;	//
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 13 ============================\r\n");
	/*
		sie crp: x 2, y 18, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = -8;	//
	io_size.auto_info.sft.y = -2;	//
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);


	DBG_ERR("============================ pattern 14 ============================\r\n");
	/*
		sie crp: x 10, y 20, w 1900, h 1040,
		sie scl: w 1900, h 1040,
		dest crp: x 0, y 0, w 1900, h 1040,
	*/
	io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
	io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	io_size.auto_info.ratio_h_v = CTL_SIE_DFT;	//
	io_size.auto_info.factor = 1000;
	io_size.auto_info.sft.x = 0;
	io_size.auto_info.sft.y = 0;
	io_size.auto_info.sie_crp_min.w = 0;
	io_size.auto_info.sie_crp_min.h = 0;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
	io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

}

static void exam_ctl_sie_io_size_test_adv_3(CTL_SIE_ID id)
{
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
//	memset((void *)&io_size, 0, sizeof(CTL_SIE_IO_SIZE_INFO));
	DBG_ERR("============================ pattern 1 ============================\r\n");
	/*
		sie crp: x 5, y 8, w 1908, h 1048,
		sie scl: w 1800, h 900,
		dest crp: x 20, y 40, w 1280, h 720,
	*/
	io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
	io_size.align.w = CTL_SIE_DFT;
	io_size.align.h = CTL_SIE_DFT;
	io_size.size_info.sie_crp.x = 5;
	io_size.size_info.sie_crp.y = 8;
	io_size.size_info.sie_crp.w = 1910;
	io_size.size_info.sie_crp.h = 1050;
	io_size.size_info.sie_scl_out.w = 1800;
	io_size.size_info.sie_scl_out.h = 900;
	io_size.size_info.dest_crp.x = 20;
	io_size.size_info.dest_crp.y = 40;
	io_size.size_info.dest_crp.w = 1280;
	io_size.size_info.dest_crp.h = 720;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = 1800;
	io_size.auto_info.sie_scl_max_sz.h = 900;
	io_size.dest_align.w = CTL_SIE_DFT;
	io_size.dest_align.h = CTL_SIE_DFT;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);

	DBG_ERR("============================ pattern 2 ============================\r\n");
	/*
		sie crp: x 5, y 8, w 1912, h 1036,
		sie scl: w 1800, h 900,
		dest crp: x 30, y 40, w 644, h 720,
	*/
	io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
	io_size.align.w = 5;
	io_size.align.h = 9;
	io_size.size_info.sie_crp.x = 5;
	io_size.size_info.sie_crp.y = 8;
	io_size.size_info.sie_crp.w = 1913;
	io_size.size_info.sie_crp.h = 1037;
	io_size.size_info.sie_scl_out.w = 1800;
	io_size.size_info.sie_scl_out.h = 900;
	io_size.size_info.dest_crp.x = 30;
	io_size.size_info.dest_crp.y = 40;
	io_size.size_info.dest_crp.w = 646;
	io_size.size_info.dest_crp.h = 726;
	io_size.auto_info.dest_ext_crp_prop.h = 0;
	io_size.auto_info.dest_ext_crp_prop.w = 0;
	io_size.auto_info.sie_scl_max_sz.w = 1821;
	io_size.auto_info.sie_scl_max_sz.h = 907;
	io_size.dest_align.w = 7;
	io_size.dest_align.h = 3;
	exam_ctl_sie_dump_io_size_set(io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	exam_ctl_sie_dump_io_size_get(id);
}

#endif

static BOOL exam_sie_ctrl_config( unsigned char argc, char **pargv)
{
	UINT32 id = 0, bitdepth = 0, pat_gen_mode = 0;
	CTL_SIE_HDL *hdl;
	CTL_SIE_CCIR_INFO ccir_info;
#if CTL_SIE_TEST_CCIR_SENSOR_FLOW
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_YUV_422_SPT;
#else
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_BAYER_8;
#endif
	CTL_SIE_FLIP_TYPE flip = CTL_SIE_FLIP_NONE;
	CTL_SIE_OUT_DEST out_dest = CTL_SIE_OUT_DEST_DRAM;
	CTL_SIE_PAG_GEN_INFO *pat_gen_info;
	CTL_SIE_PAG_GEN_INFO pat_gen_info_h_i = { {2, 2, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, { 0, 0, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, CTL_SIE_PAT_HINCREASE, 0x800, 3000};
	CTL_SIE_PAG_GEN_INFO pat_gen_info_h_v_i = { {2, 2, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, { 0, 0, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, CTL_SIE_PAT_HVINCREASE, 0x800, 3000};
	CTL_SIE_PAG_GEN_INFO pat_gen_info_c_b = { {2, 2, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, { 0, 0, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, CTL_SIE_PAT_COLORBAR, 0x64, 3000};
	CTL_SIE_PAG_GEN_INFO pat_gen_info_rand = { {2, 2, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, { 0, 0, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, CTL_SIE_PAT_RANDOM, 0x64, 3000};
	CTL_SIE_PAG_GEN_INFO pat_gen_info_fixed = { {2, 2, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, { 0, 0, CTL_SIE_TEST_PAT_GEN_SZ_W, CTL_SIE_TEST_PAT_GEN_SZ_H}, CTL_SIE_PAT_FIXED, 0x800, 3000};
	CTL_SIE_CHGSENMODE_INFO chgsenmode_info;
	CTL_SIE_REG_CB_INFO cb_info;
	CTL_SIE_IO_SIZE_INFO io_size;

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&bitdepth);
	sscanf_s(pargv[2], "%d", (int *)&pat_gen_mode);

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}

#if CTL_SIE_TEST_CCIR_SENSOR_FLOW
#else
	switch (bitdepth) {
	case 0:
		data_fmt = CTL_SIE_BAYER_8;
		break;

	case 1:
		data_fmt = CTL_SIE_BAYER_10;
		break;

	case 2:
		data_fmt = CTL_SIE_BAYER_12;
		break;

	case 3:
		data_fmt = CTL_SIE_BAYER_16;
		break;

	default:
		DBG_ERR("input data format error\r\n");
		break;
	}
#endif
	switch (pat_gen_mode) {
	case 0:
		pat_gen_info = &pat_gen_info_h_i;
		break;

	case 1:
		pat_gen_info = &pat_gen_info_h_v_i;
		break;

	case 2:
		pat_gen_info = &pat_gen_info_c_b;
		break;

	case 3:
		pat_gen_info = &pat_gen_info_rand;
		break;

	case 4:
		pat_gen_info = &pat_gen_info_fixed;
		break;

	default:
		pat_gen_info = &pat_gen_info_h_i;
		DBG_ERR("input data format error\r\n");
		break;
	}

	memset((void *)&chgsenmode_info, 0, sizeof(CTL_SIE_CHGSENMODE_INFO));

#if CTL_SIE_TEST_DIRECT_FLOW
//	out_dest = CTL_SIE_OUT_DEST_BOTH;	//output both direct and dram
	out_dest = CTL_SIE_OUT_DEST_DIRECT;
#else
	out_dest = CTL_SIE_OUT_DEST_DRAM;
#endif

	if (cur_flow_type[id] == CTL_SIE_FLOW_PATGEN) {
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)pat_gen_info);
	} else if ((cur_flow_type[id] == CTL_SIE_FLOW_SEN_IN) || (cur_flow_type[id] == CTL_SIE_FLOW_SIG_DUPL)) {
		chgsenmode_info.sel = CTL_SEN_MODESEL_AUTO;
		chgsenmode_info.auto_info.frame_rate = 3000; //DAL_SEN_FPS(30, 1);
		chgsenmode_info.auto_info.size.w = 1920;
		chgsenmode_info.auto_info.size.h = 1080;
		chgsenmode_info.auto_info.frame_num = 1;	// linear mode
		chgsenmode_info.output_dest = CTL_SEN_OUTPUT_DEST_AUTO;
#if CTL_SIE_TEST_CCIR_SENSOR_FLOW
		chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_YUV;
#else
		chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_RGB;
#endif
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
		if ((chgsenmode_info.auto_info.data_fmt == CTL_SEN_DATA_FMT_YUV) || (chgsenmode_info.auto_info.data_fmt == CTL_SEN_DATA_FMT_YUV)) {
			/* set by project */
			chgsenmode_info.auto_info.ccir.interlace = FALSE;
			chgsenmode_info.auto_info.ccir.fmt = CTL_SEN_DVI_CCIR601;
		}
		chgsenmode_info.auto_info.mux_singnal_en = FALSE;
		if (chgsenmode_info.auto_info.mux_singnal_en == TRUE) {
			/* set by project */
			chgsenmode_info.auto_info.mux_signal_info.mux_data_num = 2;
		}

		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_CHGSENMODE, (void *)&chgsenmode_info);
#if CTL_SIE_IO_SIZE_TEST
		exam_ctl_sie_io_size_test_1(id);	// CTL_SIE_CRP_ON + CTL_SIE_IOSIZE_AUTO
		exam_ctl_sie_io_size_test_2(id);	// CTL_SIE_CRP_OFF + CTL_SIE_IOSIZE_AUTO
		exam_ctl_sie_io_size_test_3(id);	// CTL_SIE_IOSIZE_MANUAL
#endif

#if CTL_SIE_IO_SIZE_TEST_ADV	// for scale
		exam_ctl_sie_io_size_test_adv_1(id);	// CTL_SIE_CRP_ON + CTL_SIE_IOSIZE_AUTO
		exam_ctl_sie_io_size_test_adv_2(id);	// CTL_SIE_CRP_OFF + CTL_SIE_IOSIZE_AUTO
		exam_ctl_sie_io_size_test_adv_3(id);	// CTL_SIE_IOSIZE_MANUAL
#endif

		io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
		io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
		io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
		io_size.auto_info.factor = 1000;
		io_size.auto_info.sft.x = 0;
		io_size.auto_info.sft.y = 0;
		io_size.auto_info.sie_crp_min.w = 0;
		io_size.auto_info.sie_crp_min.h = 0;
		io_size.size_info.sie_crp.x = 0;
		io_size.size_info.sie_crp.y = 0;
		io_size.size_info.sie_crp.w = 0;
		io_size.size_info.sie_crp.h = 0;
		io_size.size_info.dest_crp.x = 0;
		io_size.size_info.dest_crp.y = 0;
		io_size.size_info.dest_crp.w = 0;
		io_size.size_info.dest_crp.h = 0;
		io_size.align.w = CTL_SIE_DFT;
		io_size.align.h = CTL_SIE_DFT;
		io_size.auto_info.dest_ext_crp_prop.w = 0;
		io_size.auto_info.dest_ext_crp_prop.h = 0;
		io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
		io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
		io_size.dest_align.w = CTL_SIE_DFT;
		io_size.dest_align.h = CTL_SIE_DFT;
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
	}

	//Raw Encode
	if (CTL_SIE_TEST_FUNC_EN & CTL_SIE_TEST_FUNC_ENCODE) {
		data_fmt = CTL_SIE_BAYER_12;
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);
		sie_config_encode(id);
	} else {
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);
	}

	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_OUT_DEST, (void *)&out_dest);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_FLIP, (void *)&flip);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);

	if (data_fmt == CTL_SIE_YUV_422_NOSPT || data_fmt == CTL_SIE_YUV_422_SPT || data_fmt == CTL_SIE_YUV_420_SPT) {
		ccir_info.field_sel = CTL_SIE_FIELD_DISABLE;
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_CCIR, (void *)&ccir_info);
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
	}

	cb_info.cbevt = CTL_SIE_CBEVT_ENG_SIE_ISR;
	cb_info.fp = ctrl_sie_isr_cb;
	cb_info.sts = CTL_SIE_INTE_DRAM_OUT0_END | CTL_SIE_INTE_VD;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

	cb_info.cbevt = CTL_SIE_CBEVT_BUFIO;

	if (id == CTL_SIE_ID_1) {
		cb_info.fp = ctrl_sie1_bufio_cb;
	} else if (id == CTL_SIE_ID_2) {
		cb_info.fp = ctrl_sie2_bufio_cb;
	} else if (id == CTL_SIE_ID_3) {
		cb_info.fp = ctrl_sie3_bufio_cb;
	} else if (id == CTL_SIE_ID_4) {
		cb_info.fp = ctrl_sie4_bufio_cb;
	} else if (id == CTL_SIE_ID_5) {
		cb_info.fp = ctrl_sie5_bufio_cb;
	} else {
		DBG_ERR("NULL cb fp\r\n");
	}

	cb_info.sts = CTL_SIE_BUF_IO_ALL;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);
	//Buffer Calculation
	ctrl_sie_config_buffer(id);

	return TRUE;
}

BOOL ctl_sie_cmd_on( unsigned char argc, char **pargv)
{
	UINT32 i;

	UINT32 ID_1 = CTL_SIE_ID_1;
	memset((void *)cmd_buf, 0, CMD_LEN * 4);
	for (i = 0; i < 4; i++) {
		cmd[i] = &cmd_buf[CMD_LEN * i];
	}

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&ID_1);
	}

	//open sie
		snprintf(cmd[0], CMD_LEN, "%d", (int)ID_1);
#if CTL_SIE_TEST_PAT_GEN
		snprintf(cmd[1], CMD_LEN, "%d", 2);	//pattern gen
#else
		snprintf(cmd[1], CMD_LEN, "%d", 1);	//sensor in
#endif
		exam_sie_ctrl_open(2, cmd);
		//config sie
		snprintf(cmd[0], CMD_LEN, "%d", (int)ID_1);
		snprintf(cmd[1], CMD_LEN, "%d", 0);
		snprintf(cmd[2], CMD_LEN, "%d", 0);
		exam_sie_ctrl_config(3, cmd);
		//trigger start
		snprintf(cmd[0], CMD_LEN, "%d", (int)ID_1);
		snprintf(cmd[1], CMD_LEN, "%d", 1);	//0: trig stop, 1: trig start
		snprintf(cmd[2], CMD_LEN, "%x", 0xffffffff);	//trig frame number for continus
		snprintf(cmd[3], CMD_LEN, "%d", 1);	//1: wait end
		ctl_sie_cmd_trig(4, cmd);

	return 0;
}

BOOL ctl_sie_cmd_off( unsigned char argc, char **pargv)
{
	UINT32 ID_1 = CTL_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&ID_1);
	}
	//trigger stop
	snprintf(cmd[0], CMD_LEN, "%d", (int)ID_1);
	snprintf(cmd[1], CMD_LEN, "%d", 0);
	snprintf(cmd[2], CMD_LEN, "%x", 0xffffffff);
	snprintf(cmd[3], CMD_LEN, "%d", 1);
	ctl_sie_cmd_trig(4, cmd);
	//close sie
	snprintf(cmd[0], CMD_LEN, "%d", (int)ID_1);
	exam_sie_ctrl_close(1, cmd);
	return 0;
}

BOOL ctl_sie_cmd_open( unsigned char argc, char **pargv)
{
	UINT32 i;
	UINT32 id, type;

	memset((void *)cmd_buf, 0, CMD_LEN * 4);
	for (i = 0; i < 4; i++) {
		cmd[i] = &cmd_buf[CMD_LEN * i];
	}

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&type);	//1: CTL_SIE_FLOW_SEN_IN, 2: CTL_SIE_FLOW_PATGEN

	//open sie
	snprintf(cmd[0], CMD_LEN, "%d", (int)id);
	snprintf(cmd[1], CMD_LEN, "%d", (int)type);
	exam_sie_ctrl_open(type, cmd);

//	ctl_sie_isp_evt_fp_reg("ISP_DEV", &ctl_sie_cmd_isp_cb, ISP_EVENT_SIE_CRPEND|ISP_EVENT_SIE_CRPST, CTL_SIE_ISP_CB_MSG_NONE);

	return 0;
}

BOOL ctl_sie_cmd_close( unsigned char argc, char **pargv)
{
	UINT32 id;

	sscanf_s(pargv[0], "%d", (int *)&id);

	//close sie
	snprintf(cmd[0], CMD_LEN, "%d", (int)id);
	exam_sie_ctrl_close(1, cmd);

	return 0;
}

BOOL ctl_sie_cmd_start( unsigned char argc, char **pargv)
{
	UINT32 i;
	UINT32 id, bitdepth, pat_gen_mode;

	memset((void *)cmd_buf, 0, CMD_LEN * 4);
	for (i = 0; i < 4; i++) {
		cmd[i] = &cmd_buf[CMD_LEN * i];
	}

	sscanf_s(pargv[0], "%d", (int *)&id);

	//config sie
	snprintf(cmd[0], CMD_LEN, "%d", (int)id);
	if (argc > 1) {
		sscanf_s(pargv[1], "%d", (int *)&bitdepth);
		sscanf_s(pargv[2], "%d", (int *)&pat_gen_mode);
		snprintf(cmd[1], CMD_LEN, "%d", (int)bitdepth);
		snprintf(cmd[2], CMD_LEN, "%d", (int)pat_gen_mode);
	} else {
		snprintf(cmd[1], CMD_LEN, "%d", 0);
		snprintf(cmd[2], CMD_LEN, "%d", 0);
	}
	exam_sie_ctrl_config(3, cmd);

	snprintf(cmd[1], CMD_LEN, "%d", 1);
	snprintf(cmd[2], CMD_LEN, "%x", 0xffffffff);
	snprintf(cmd[3], CMD_LEN, "%d", 1);
	ctl_sie_cmd_trig(4, cmd);

	return 0;
}

BOOL ctl_sie_cmd_stop( unsigned char argc, char **pargv)
{
	UINT32 id, i;

	memset((void *)cmd_buf, 0, CMD_LEN * 4);
	for (i = 0; i < 4; i++) {
		cmd[i] = &cmd_buf[CMD_LEN * i];
	}

	sscanf_s(pargv[0], "%d", (int *)&id);

	snprintf(cmd[0], CMD_LEN, "%d", (int)id);
	snprintf(cmd[1], CMD_LEN, "%d", 0);
	snprintf(cmd[2], CMD_LEN, "%x", 0xffffffff);
	snprintf(cmd[3], CMD_LEN, "%d", 1);
	ctl_sie_cmd_trig(4, cmd);

	return 0;
}

BOOL ctl_sie_cmd_trig( unsigned char argc, char **pargv)
{
	UINT32 id, trig_type;
	CTL_SIE_HDL *hdl;
	CTL_SIE_TRIG_INFO trig_info;

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&trig_type);
	sscanf_s(pargv[2], "%x", (unsigned int *)&trig_info.trig_frame_num);
	sscanf_s(pargv[3], "%d", (int *)&trig_info.b_wait_end);

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}
	trig_info.trig_type = trig_type;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);

	return 0;
}

BOOL ctl_sie_cmd_test_cmd( unsigned char argc, char **pargv)
{
	CTL_SIE_HDL *hdl;
	CTL_SIE_HDL *hdl2;
	UINT32 i;
	UINT32 id, id2, loop_time, min_ratio;
	UINT32 factor = 1000;

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&id2);
	sscanf_s(pargv[2], "%d", (int *)&loop_time);
	sscanf_s(pargv[3], "%d", (int *)&min_ratio);

	hdl = ctl_sie_get_hdl(id);
	if (id == id2) {
		hdl2 = 0;
	} else {
		hdl2 = ctl_sie_get_hdl(id2);
	}

	for (i = 0; i < loop_time; i++) {
		if (hdl->flow_type == CTL_SIE_FLOW_PATGEN && (hdl2 != 0 && hdl2->flow_type == CTL_SIE_FLOW_PATGEN)) {
			CTL_SIE_PAG_GEN_INFO pat_gen_info;
			USIZE org_size;

			factor = hwclock_get_counter();
			if (i >= loop_time -1) {
				factor = 1000;
			} else {
				factor = (factor % (1000 - min_ratio)) + min_ratio;
			}

			ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
			org_size.w = pat_gen_info.crp_win.w;
			org_size.h = pat_gen_info.crp_win.h;
			pat_gen_info.crp_win.w = org_size.w * factor / 1000;
			pat_gen_info.crp_win.h = org_size.h * factor / 1000;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
			if (hdl2 != 0) {
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_LOAD, NULL);
			}
			vos_util_delay_ms(3000);
			ctl_sie_dbg_set_msg_type(id, 4, 10, 1, 0);
			vos_util_delay_ms(3000);

			pat_gen_info.crp_win.w = org_size.w * factor / 1000;
			pat_gen_info.crp_win.h = org_size.h * factor / 1000;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
			if (hdl2 != 0) {
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_LOAD, NULL);
			}
			vos_util_delay_ms(3000);
			ctl_sie_dbg_set_msg_type(id, 4, 10, 1, 0);
			vos_util_delay_ms(3000);

			pat_gen_info.crp_win.w = org_size.w * factor / 1000;
			pat_gen_info.crp_win.h = org_size.h * factor / 1000;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
			if (hdl2 != 0) {
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_LOAD, NULL);
			}
			vos_util_delay_ms(3000);
			ctl_sie_dbg_set_msg_type(id, 4, 10, 1, 0);
			vos_util_delay_ms(3000);

			//set back to org size
			pat_gen_info.crp_win.w = org_size.w;
			pat_gen_info.crp_win.h = org_size.h;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
			if (hdl2 != 0) {
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_LOAD, NULL);
			}
			vos_util_delay_ms(3000);
			ctl_sie_dbg_set_msg_type(id, 4, 10, 1, 0);
			vos_util_delay_ms(3000);
		} else {
			CTL_SIE_IO_SIZE_INFO io_size;

			factor = hwclock_get_counter();
			if (i >= loop_time -1) {
				factor = 1000;
			} else {
				factor = (factor % (1000 - min_ratio)) + min_ratio;
			}
			DBG_ERR("loop cnt %d, factor %d, total cnt %d\r\n", (int)(i), (int)factor, (int)(loop_time));
			io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
			io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
			io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
			io_size.auto_info.factor = factor;
			io_size.auto_info.sft.x = 0;
			io_size.auto_info.sft.y = 0;
			io_size.auto_info.sie_crp_min.w = 0;
			io_size.auto_info.sie_crp_min.h = 0;
			io_size.align.w = CTL_SIE_DFT;
			io_size.align.h = CTL_SIE_DFT;
			io_size.auto_info.dest_ext_crp_prop.w = 0;
			io_size.auto_info.dest_ext_crp_prop.h = 0;
			io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
			io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
			io_size.dest_align.w = CTL_SIE_DFT;
			io_size.dest_align.h = CTL_SIE_DFT;

			DBG_ERR("chg io size, ratio %d\r\n", (int)(io_size.auto_info.factor));
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
			if (hdl2 != 0) {
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);
				ctl_sie_set((UINT32)hdl2, CTL_SIE_ITEM_LOAD, NULL);
			}
			vos_util_delay_ms((hwclock_get_counter()%400)+300);
		}
	}

	DBG_ERR("ctl sie test cmd end\r\n");
	return 0;
}

BOOL ctl_sie_cmd_isp_cfg_eth( unsigned char argc, char **pargv)
{
	CTL_SIE_ID id;
	UINT32 en;
	CTL_SIE_ISP_ETH_PARAM eth_param = {0,0,100,{0,0,100,100},{1, 1, 1, 1, 1, 12,24,36},0};
	CTL_SIE_ISP_ETH_PARAM eth_get_param;
	CTL_SIE_ISP_IO_SIZE sie_io_size;
#if EXAM_CTRL_SIE_USE_NVTMPP
	NVTMPP_MODULE  module = MAKE_NVTMPP_MODULE('e', 't', 'h', 't', 'e', 's', 't', 0);
		NVTMPP_VB_POOL pool = NVTMPP_VB_INVALID_POOL;
		NVTMPP_DDR     ddr = NVTMPP_DDR_1;
		NVTMPP_VB_BLK  blk;
		UINT32 nvtmpp_size, nvtmpp_addr;
#endif

	/*[id]*/
	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}

	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&en);

	if (en) {
		ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_IO_SIZE, (void *)&sie_io_size);
#if EXAM_CTRL_SIE_USE_NVTMPP
		nvtmpp_size = sie_io_size.in_crp_win.w*sie_io_size.in_crp_win.h;
		blk = nvtmpp_vb_get_block(module, pool, nvtmpp_size, ddr);
		if (blk == NVTMPP_VB_INVALID_BLK) {
			DBG_DUMP("get input buffer fail\n");
		}
		nvtmpp_addr = nvtmpp_vb_blk2va(blk);
		DBG_DUMP("input buffer addr 0x%.8x size 0x%.8x\r\n", (unsigned int)nvtmpp_addr, (unsigned int)nvtmpp_size);
		eth_param.buf_addr = nvtmpp_addr;
		eth_param.buf_size = nvtmpp_size;
#else
		/*get in buffer*/
		sie_eth_out_buf.size = sie_io_size.in_crp_win.w*sie_io_size.in_crp_win.h;
		sie_eth_out_buf.align = 64;      ///< address alignment
		sie_eth_out_buf.name = "ctrl_sie_out";
		sie_eth_out_buf.alloc_type = ALLOC_CACHEABLE;
		if (frm_get_buf_ddr(DDR_ID0, &sie_eth_out_buf)) {
			DBG_ERR("get input buffer fail\n");
			return 0;
		}
		eth_param.buf_addr = (UINT32)sie_eth_out_buf.va_addr;
		eth_param.buf_size = sie_eth_out_buf.size;
#endif
		eth_param.roi_base = 100;
		eth_param.crop_roi.x = 0;
		eth_param.crop_roi.y = 0;
		eth_param.crop_roi.w = 100;	//eth horizontal crop eth_param.crop_roi.w/eth_param.roi_base of sie crop width
		eth_param.crop_roi.h = 100; //eth horizontal crop eth_param.crop_roi.h/eth_param.roi_base of sie crop height
	} else {
		eth_param.eth_info.enable = DISABLE;
	}
	//set eth out
	ctl_sie_isp_set(id, CTL_SIE_ISP_ITEM_ETH, (void *)&eth_param);

	//get sie eth out info
	ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_ETH, (void *)&eth_get_param);
	DBG_ERR("eth_en:%d, eth_out_addr:%x,eth_crp_w:%d,eth_crp_h:%d,eth_out_lofs:%d\r\n", (int)(eth_get_param.eth_info.enable), (unsigned int)(eth_get_param.buf_addr), (int)(eth_get_param.crop_roi.w), (int)(eth_get_param.crop_roi.h), (int)(eth_get_param.out_lofs));

	return 0;
}

BOOL ctl_sie_cmd_set_iosize( unsigned char argc, char **pargv)
{
	CTL_SIE_ID id;
	CTL_SIE_HDL *hdl;

	sscanf_s(pargv[0], "%d", (int *)&id);
	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}
//Pattern Gen
	/*0~1:[id][crop w][crop h]*/
//Sensor In
	/*0~3:[id][CTL_SIE_CRP_SEL][CTL_SIE_DZOOM_SEL][factor]*/
	/*4~8:[ratio_h_v][sft.x][sft.y][sie_crp_min.w][sie_crp_min.h]*/
	/*9~18:[sie_crp.x][sie_crp.y][sie_crp.w][sie_crp.h][dest_crp.x][dest_crp.y][dest_crp.w][dest_crp.h][align.w][align.h]*/
	if (hdl->flow_type == CTL_SIE_FLOW_PATGEN) {
		CTL_SIE_PAG_GEN_INFO pat_gen_info;

		if ((argc != 3)) {
			nvt_dbg(ERR, "wrong argument:%d", argc);
			return 0;
		}

		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
		sscanf_s(pargv[1], "%d", (int *)&pat_gen_info.crp_win.w);
		sscanf_s(pargv[2], "%d", (int *)&pat_gen_info.crp_win.h);
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);
		DBG_ERR("set CTL_SIE_ID_%d, set crop size (%d, %d)\r\n", (int)(id+1), (int)(pat_gen_info.crp_win.w), (int)(pat_gen_info.crp_win.h));
	} else {
		CTL_SIE_IO_SIZE_INFO io_size;
		CTL_SIE_IO_SIZE_INFO io_size_get;

		if ((argc != 4) && (argc != 9) && (argc != 19)) {
			nvt_dbg(ERR, "wrong argument:%d", argc);
			return 0;
		}
		sscanf_s(pargv[1], "%d", (int *)&io_size.auto_info.crp_sel);
		sscanf_s(pargv[2], "%d", (int *)&io_size.iosize_sel);
		sscanf_s(pargv[3], "%d", (int *)&io_size.auto_info.factor);

		if (argc >= 9) {
			sscanf_s(pargv[4], "%x", (unsigned int *)&io_size.auto_info.ratio_h_v);
			sscanf_s(pargv[5], "%d", (int *)&io_size.auto_info.sft.x);
			sscanf_s(pargv[6], "%d", (int *)&io_size.auto_info.sft.y);
			sscanf_s(pargv[7], "%d", (int *)&io_size.auto_info.sie_crp_min.w);
			sscanf_s(pargv[8], "%d", (int *)&io_size.auto_info.sie_crp_min.h);
		} else {
			io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(16, 9);
			io_size.auto_info.sft.x = 0;
			io_size.auto_info.sft.y = 0;
			io_size.auto_info.sie_crp_min.w = 0;
			io_size.auto_info.sie_crp_min.h = 0;
		}

		if (argc >= 19) {
			sscanf_s(pargv[9], "%d", (int *)&io_size.size_info.sie_crp.x);
			sscanf_s(pargv[10], "%d", (int *)&io_size.size_info.sie_crp.y);
			sscanf_s(pargv[11], "%d", (int *)&io_size.size_info.sie_crp.w);
			sscanf_s(pargv[12], "%d", (int *)&io_size.size_info.sie_crp.h);
			sscanf_s(pargv[13], "%d", (int *)&io_size.size_info.dest_crp.x);
			sscanf_s(pargv[14], "%d", (int *)&io_size.size_info.dest_crp.y);
			sscanf_s(pargv[15], "%d", (int *)&io_size.size_info.dest_crp.w);
			sscanf_s(pargv[16], "%d", (int *)&io_size.size_info.dest_crp.h);
			sscanf_s(pargv[17], "%d", (int *)&io_size.align.w);
			sscanf_s(pargv[18], "%d", (int *)&io_size.align.h);
		} else {
			io_size.size_info.sie_crp.x = 0;
			io_size.size_info.sie_crp.y = 0;
			io_size.size_info.sie_crp.w = 1920;
			io_size.size_info.sie_crp.h = 1080;
			io_size.size_info.dest_crp.x = 12;
			io_size.size_info.dest_crp.y = 20;
			io_size.size_info.dest_crp.w = 1280;
			io_size.size_info.dest_crp.h = 720;
			io_size.align.w = 0;
			io_size.align.h = 0;
		}

		io_size.auto_info.dest_ext_crp_prop.w = 0;
		io_size.auto_info.dest_ext_crp_prop.h = 0;
		io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
		io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
		io_size.dest_align.w = CTL_SIE_DFT;
		io_size.dest_align.h = CTL_SIE_DFT;

		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size);

		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);

		ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&io_size_get);

		DBG_DUMP("(in)io_size:crp_sel%d,dzoom_sel%d,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
			, io_size.auto_info.crp_sel, io_size.iosize_sel
			, io_size.auto_info.ratio_h_v, io_size.auto_info.factor
			, io_size.auto_info.sft.x, io_size.auto_info.sft.y
			, io_size.auto_info.sie_crp_min.w, io_size.auto_info.sie_crp_min.h);
		DBG_DUMP("(in)io_size:size_info(siecrp(w%d,h%d),dest_crp(w%d,h%d)),align(w%d,h%d)\r\n"
			, io_size.size_info.sie_crp.w, io_size.size_info.sie_crp.h
			, io_size.size_info.dest_crp.w, io_size.size_info.dest_crp.h
			, io_size.align.w, io_size.align.h);

		DBG_DUMP("(out)io_size:crp_sel%d,iosz_sel%d,dzoom_auto_info(rt0x%.8x,fc%d,sft(x%d,y%d),siecrpmin(w%d,h%d))\r\n"
			, io_size_get.auto_info.crp_sel, io_size_get.iosize_sel
			, io_size_get.auto_info.ratio_h_v, io_size_get.auto_info.factor
			, io_size_get.auto_info.sft.x, io_size_get.auto_info.sft.y
			, io_size_get.auto_info.sie_crp_min.w, io_size_get.auto_info.sie_crp_min.h);
		DBG_DUMP("(out)io_size:size_info(siecrp(w%d,h%d),dest_crp(w%d,h%d)),align(w%d,h%d)\r\n"
			, io_size_get.size_info.sie_crp.w, io_size_get.size_info.sie_crp.h
			, io_size_get.size_info.dest_crp.w, io_size_get.size_info.dest_crp.h
			, io_size_get.align.w, io_size_get.align.h);
	}
	return 0;
}

/*

	// sensor driver setting
	switch (mode_dvi->mode) {
	case CTL_SEN_MODE_1:
		mode_dvi->fmt = CTL_SEN_DVI_CCIR601;
		mode_dvi->data_mode = CTL_SEN_DVI_DATA_MODE_HD;
		break;
	case CTL_SEN_MODE_2:
		mode_dvi->fmt = CTL_SEN_DVI_CCIR656_EAV;
		mode_dvi->data_mode = CTL_SEN_DVI_DATA_MODE_HD;
		break;
	case CTL_SEN_MODE_3:
		mode_dvi->fmt = CTL_SEN_DVI_CCIR656_ACT;
		mode_dvi->data_mode = CTL_SEN_DVI_DATA_MODE_HD;
		break;
	case CTL_SEN_MODE_4:
		mode_dvi->fmt = CTL_SEN_DVI_CCIR601;
		mode_dvi->data_mode = CTL_SEN_DVI_DATA_MODE_SD; (8)
		break;


	switch (mode_basic->mode) {
	case CTL_SEN_MODE_1:
		mode_basic->mode_type = CTL_SEN_MODE_CCIR;
		mode_basic->stpix = CTL_SEN_STPIX_YUV_YUYV;
		break;
	case CTL_SEN_MODE_2:
		mode_basic->mode_type = CTL_SEN_MODE_CCIR;
		mode_basic->stpix = CTL_SEN_STPIX_YUV_YVYU;
		break;
	case CTL_SEN_MODE_3:
		mode_basic->mode_type = CTL_SEN_MODE_CCIR_INTERLACE;
		mode_basic->stpix = CTL_SEN_STPIX_YUV_UYVY;
		break;
	case CTL_SEN_MODE_4:
		mode_basic->mode_type = CTL_SEN_MODE_CCIR_INTERLACE;
		mode_basic->stpix = CTL_SEN_STPIX_YUV_VYUY;
		break;

	// ctl sie api setting
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_YUV_422_NOSPT;

	// rst
	SIE REG 0xf0c000ac:
	CTL_SEN_MODE_1  C00001BA
	CTL_SEN_MODE_2  C00001FB
	CTL_SEN_MODE_3  4000013F
	CTL_SEN_MODE_4  400001D8
*/
#define CTL_SIE_CCIR_TEST DISABLE
#if CTL_SIE_CCIR_TEST
static void exam_ctl_sie_ccir_test(CTL_SIE_ID id, UINT32 *param)
{
	CTL_SIE_CCIR_INFO ccir_info;
	CTL_SIE_TRIG_INFO trig_info;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("param %d\r\n", (int)(param[0]));

	memset((void *)&ccir_info, 0, sizeof(CTL_SIE_CCIR_INFO));
	memset((void *)&trig_info, 0, sizeof(CTL_SIE_TRIG_INFO));


	trig_info.b_wait_end = FALSE;
	trig_info.trig_frame_num = 0xffffffff;
	trig_info.trig_type = CTL_SIE_TRIG_STOP;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);

	if (param[0] == 0) {
		ccir_info.field_sel = CTL_SIE_FIELD_DISABLE;
	} else if (param[0] == 1) {
		ccir_info.field_sel = CTL_SIE_FIELD_EN_0;
	} else if (param[0] == 2) {
		ccir_info.field_sel = CTL_SIE_FIELD_EN_1;
	}

	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_CCIR, (void *)&ccir_info);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);

	trig_info.b_wait_end = FALSE;
	trig_info.trig_frame_num = 0xffffffff;
	trig_info.trig_type = CTL_SIE_TRIG_START;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
}

static void exam_ctl_sie_chgsenmode_test(CTL_SIE_ID id, UINT32 *param)
{
	CTL_SIE_CHGSENMODE_INFO chgsenmode_info;
	CTL_SIE_TRIG_INFO trig_info;
	CTL_SIE_HDL *hdl;

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return;
	}
	DBG_ERR("param %d\r\n", (int)(param[0]));

	memset((void *)&chgsenmode_info, 0, sizeof(CTL_SIE_CHGSENMODE_INFO));
	memset((void *)&trig_info, 0, sizeof(CTL_SIE_TRIG_INFO));

	trig_info.b_wait_end = FALSE;
	trig_info.trig_frame_num = 0xffffffff;
	trig_info.trig_type = CTL_SIE_TRIG_STOP;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);

	chgsenmode_info.sel = CTL_SEN_MODESEL_AUTO;
	#if CTL_SIE_TEST_CCIR_SENSOR_FLOW
	chgsenmode_info.auto_info.frame_rate = 2500;// fit 6124b sensor driver mode
	#else
	chgsenmode_info.auto_info.frame_rate = 3000;
	#endif
	chgsenmode_info.auto_info.size.w = 1920;
	chgsenmode_info.auto_info.size.h = 1080;
	chgsenmode_info.auto_info.frame_num = 1;	// linear mode
	chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_YUV;
	chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	if ((chgsenmode_info.auto_info.data_fmt == CTL_SEN_DATA_FMT_YUV) || (chgsenmode_info.auto_info.data_fmt == CTL_SEN_DATA_FMT_YUV)) {
		/* set by project */
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
	}
	chgsenmode_info.auto_info.mux_singnal_en = FALSE;
	if (chgsenmode_info.auto_info.mux_singnal_en == TRUE) {
		/* set by project */
		chgsenmode_info.auto_info.mux_signal_info.mux_data_num = 2;
	}

	if (param[0] == 0) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 1) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 2) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 3) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 4) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 5) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_IGNORE;
	} else if (param[0] == 6) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 7) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 8) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 9) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 10) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 11) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	} else if (param[0] == 12) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	} else if (param[0] == 13) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	} else if (param[0] == 14) {
		chgsenmode_info.auto_info.ccir.interlace = FALSE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	} else if (param[0] == 15) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR601;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	} else if (param[0] == 16) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_EAV;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	} else if (param[0] == 17) {
		chgsenmode_info.auto_info.ccir.interlace = TRUE;
		chgsenmode_info.auto_info.ccir.dvi_fmt = CTL_SEN_DVI_CCIR656_ACT;
		chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_16BIT;
	}

	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_CHGSENMODE, (void *)&chgsenmode_info);
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_LOAD, NULL);


	trig_info.b_wait_end = FALSE;
	trig_info.trig_frame_num = 0xffffffff;
	trig_info.trig_type = CTL_SIE_TRIG_START;
	ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);

}
#endif	//ccir test

BOOL ctl_sie_cmd_dump_ca_rst(UINT32 id)
{
	UINT32 i, j;
	ER rt;
	CTL_SIE_HDL *hdl;
	CTL_SIE_ALG_FUNC alg_func = {0};

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}
	alg_func.type = CTL_SIE_ALG_TYPE_AWB;
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_ALG_FUNC_IMM, (void *)&alg_func);
	memset((void *)&ca_rst, 0, sizeof(CTL_SIE_CA_RSLT));

	DBG_DUMP("--dump ca rst-- %d x %d\r\n", CTL_SIE_ISP_CA_MAX_WINNUM, CTL_SIE_ISP_CA_MAX_WINNUM);
	if (alg_func.func_en) {
		rt = ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_CA_RSLT, (void *)&ca_rst);
		if (rt == E_OK) {
			for (i = 0; i < CTL_SIE_ISP_CA_MAX_WINNUM; i++) {
				for (j = 0; j < CTL_SIE_ISP_CA_MAX_WINNUM; j++) {
					DBG_DUMP("%-4d,", (int)ca_rst.r[i * CTL_SIE_ISP_CA_MAX_WINNUM + j]);
				}
				DBG_DUMP("\r\n");
			}
		}
	} else {
		DBG_DUMP("--ca disable--\r\n");
	}
	return 0;
}

BOOL ctl_sie_cmd_dump_la_rst(UINT32 id)
{
	#define DUMP_LEN_LA_RST 13
	UINT32 i, j;
	ER rt;
	CTL_SIE_HDL *hdl;
	CTL_SIE_ALG_FUNC alg_func = {0};

	hdl = ctl_sie_get_hdl(id);
	if (hdl == NULL) {
		DBG_ERR("id: %d get sie handle fail\r\n", id);
		return FALSE;
	}
	alg_func.type = CTL_SIE_ALG_TYPE_AE;
	ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_ALG_FUNC_IMM, (void *)&alg_func);

	DBG_DUMP("--dump la rst--\r\n");

	if (alg_func.func_en) {
		rt = ctl_sie_get((UINT32)hdl, CTL_SIE_ITEM_LA_RSLT, (void *)&la_rslt);
		if (rt == E_OK) {
			DBG_DUMP("--p_buf_lum_1--\r\n");
			for (i = 0; i < CTL_SIE_ISP_LA_MAX_WINNUM; i++) {
				for (j = 0; j < CTL_SIE_ISP_LA_MAX_WINNUM; j++) {
					debug_msg("%-4d,", (int)la_rslt.lum_1[i * CTL_SIE_ISP_LA_MAX_WINNUM + j]);
				}
				debug_msg("\r\n");
			}
			DBG_DUMP("--p_buf_lum_2--\r\n");
			for (i = 0; i < CTL_SIE_ISP_LA_MAX_WINNUM; i++) {
				for (j = 0; j < CTL_SIE_ISP_LA_MAX_WINNUM; j++) {
					debug_msg("%-4d,", (int)la_rslt.lum_2[i * CTL_SIE_ISP_LA_MAX_WINNUM + j]);
				}
				debug_msg("\r\n");
			}
			DBG_DUMP("--p_buf_histogram--\r\n");
			for (i = 0; i < (CTL_SIE_ISP_LA_HIST_NUM / DUMP_LEN_LA_RST); i++) {
				for (j = 0; j < DUMP_LEN_LA_RST; j++) {
					debug_msg("%-4d,", (int)la_rslt.histogram[i * DUMP_LEN_LA_RST + j]);
				}
				debug_msg("\r\n");
			}
		}
	} else {
		DBG_DUMP("--la disable--\r\n");
	}

	return 0;
}

BOOL ctl_sie_cmd_get_statis( unsigned char argc, char **pargv)
{
	UINT32 id;
	BOOL dump_ca, dump_la;

	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}
	sscanf_s(pargv[0], "%d", (int *)&id);
	sscanf_s(pargv[1], "%d", (int *)&dump_ca);
	sscanf_s(pargv[2], "%d", (int *)&dump_la);

	if (dump_ca) {
		ctl_sie_cmd_dump_ca_rst(id);
	}

	if (dump_la) {
		ctl_sie_cmd_dump_la_rst(id);
	}

	return 0;
}

#endif	//ctl sie test cmd end


#if 0
#endif

int ctl_sie_cmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_sie);
	UINT32 loop = 1;

	DBG_DUMP("---------------------------------------------------------------------\r\n");
	DBG_DUMP("  %s\n", "ctl_sie");
	DBG_DUMP("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", ctl_sie[loop].p_name, ctl_sie[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int ctl_sie_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(ctl_sie, argc, argv)
#endif

{
	UINT32 cmd_num = SXCMD_NUM(ctl_sie);
	UINT32 loop;
	int    ret;
	unsigned char ucargc = 0;

#if  0//CTL_SIE_TEST_CMD
	DBG_DUMP("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);
#endif

	if (strncmp(argv[1], "?", 2) == 0) {
		ctl_sie_cmd_showhelp();
		return 0;
	}

	if (argc < 1) {
		DBG_ERR("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ctl_sie[loop].p_name, strlen(argv[1])) == 0) {
			DBG_DUMP("\r\n");
			ret = ctl_sie[loop].p_func(ucargc, &argv[2]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ctl_sie_cmd_showhelp();
		return -1;
	}
	return 0;
}
