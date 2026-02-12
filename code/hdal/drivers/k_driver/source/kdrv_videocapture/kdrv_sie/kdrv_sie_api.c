#if defined (__FREERTOS)
#include <stdio.h>
#endif
#include "kwrap/stdio.h"
#include "kwrap/type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kwrap/mem.h"
#include "kwrap/file.h"
#include "sie_lib.h"
#include "sie_platform.h"
#include "sie_dbg.h"
#include "kdrv_sie_api.h"
#include "kdrv_sie.h"
#include "kdrv_sie_debug_int.h"
#include "comm/hwclock.h"

UINT32 kdrv_siet_cmd_kdrv_id[KDRV_SIE_MAX_ENG] = {0};

#if KDRV_SIE_TEST_CMD
#define KDRV_SIE_CMD_LEN (64)
char kdrv_sie_cmd_buf[KDRV_SIE_CMD_LEN * 4];
char *kdrv_sie_cmd[4];
#endif

static SXCMD_BEGIN(kdrv_sie, "kdrv_sie")
SXCMD_ITEM("dbgtype",		kdrv_sie_cmd_set_dbg_type, 			"set kdrv sie debug msg type (id, type), plz ref. to KDRV_SIE_DBG_MSG_TYPE")
SXCMD_ITEM("dbglevel",  	kdrv_sie_cmd_set_dbg_level, 		"set kdrv sie debug level (lvl), plz ref to KDRV_SIE_DBG_LVL")
SXCMD_ITEM("dbgcalcrawt",  	kdrv_sie_cmd_calc_raw_time, 		"calc sensor raw time (id, start_line, end_line), set star and end line to 0 after calc end")
#if KDRV_SIE_TEST_CMD
SXCMD_ITEM("saveraw",  		kdrv_sie_cmd_save_raw, 				"save sie output raw (id)")
SXCMD_ITEM("kdrvsieopen",   kdrv_sie_test_cmd_open,				"test cmd, open sie (id)")
SXCMD_ITEM("kdrvsieclose", 	kdrv_sie_test_cmd_close,			"test cmd, config and start sie (id)")
SXCMD_ITEM("kdrvsiestart", 	kdrv_sie_test_cmd_start,			"test cmd, close sie (id)")
SXCMD_ITEM("kdrvsiestop",  	kdrv_sie_test_cmd_stop,				"test cmd, stop sie (id)")
SXCMD_ITEM("kdrvsieon",   	kdrv_sie_test_cmd_on, 				"test cmd, open -> config -> start sie (id, pat_gen_mode, out_bitdepth)")
SXCMD_ITEM("kdrvsieoff",   	kdrv_sie_test_cmd_off, 				"test cmd, stop -> close sie (id)")
#endif
SXCMD_END()

#if 0
#endif

BOOL kdrv_sie_cmd_set_dbg_type(unsigned char argc, char **pargv)
{
	KDRV_SIE_PROC_ID id = KDRV_SIE_ID_1;
	UINT32 type = KDRV_SIE_DBG_MSG_ALL;

	if (argc == 1) {
		kdrv_sie_dbg_dump("wrong argument:%d, plz input id and msg_type", (int)(argc));
		sscanf_s(pargv[0], "%d", (int *)&id);
	} else if (argc == 2) {
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&type);
	} else {
		kdrv_sie_dbg_dump("wrong argument:%d, plz input id and msg_type, force dump All Msg", argc);
	}

	kdrv_sie_dbg_set_msg_type(id, type);
	return 0;
}

BOOL kdrv_sie_cmd_set_dbg_level( unsigned char argc, char **pargv)
{
	KDRV_SIE_DBG_LVL dbg_level = KDRV_SIE_DBG_LVL_NONE;

	/*[id]*/
	if (argc == 1) {
		sscanf_s(pargv[0], "%d", (int *)&dbg_level);
	} else {
		kdrv_sie_dbg_dump("wrong argument:%d, plz input debug_level, force set to CTL_SIE_DBG_LVL_ERR", argc);
		dbg_level = KDRV_SIE_DBG_LVL_ERR;
	}

	kdrv_sie_dbg_set_dbg_level(dbg_level);
	return 0;
}

BOOL kdrv_sie_cmd_calc_raw_time(unsigned char argc, char **pargv)
{
	KDRV_SIE_PROC_ID id = KDRV_SIE_ID_1;
	UINT32 start_line, end_line;

	if (argc == 3) {
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&start_line);
		sscanf_s(pargv[2], "%d", (int *)&end_line);
		sie_lib_calc_row_time(id, start_line, end_line);
	} else {
		kdrv_sie_dbg_dump("wrong argument:%d, plz input id, start_line and end_line", argc);
	}
	return 0;
}


void kdrv_sie_cmd_savefile(CHAR *f_name, UINT32 addr, UINT32 size)
{
	VOS_FILE fp;

	if (addr != 0) {
		fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
		    kdrv_sie_dbg_err("failed in file open:%s\n", f_name);
		} else {
			vos_file_write(fp, (void *)addr, size);
			vos_file_close(fp);
			kdrv_sie_dbg_dump("save file %s\r\n", f_name);
		}
	}
}

#if 0
#endif

#if KDRV_SIE_TEST_CMD
////////////////////////////test cmd begin/////////////////////////////////////////
#define KDRV_SIE_TEST_RING_BUF_NUM 3

typedef struct {
	struct vos_mem_cma_info_t cma_info;
	VOS_MEM_CMA_HDL cma_hdl;
} KDRV_SIE_VOS_MEM_INFO;

KDRV_SIE_VOS_MEM_INFO kdrv_vos_mem_info;
KDRV_SIE_VOS_MEM_INFO kdrv_vos_mem_sie_out_buf_info[KDRV_SIE_MAX_ENG][KDRV_SIE_CH_MAX][KDRV_SIE_TEST_RING_BUF_NUM] = {0};
////////////////////////////test cmd end/////////////////////////////////////////
static UINT32 kdrv_sie_test_cmd_fc[KDRV_SIE_MAX_ENG];
/*============================================================================================
Kdrv SIE Buffer Size Cal.
============================================================================================*/
static UINT32 kdrv_sie_test_ppb[KDRV_SIE_MAX_ENG][KDRV_SIE_CH_MAX][KDRV_SIE_TEST_RING_BUF_NUM];
static UINT32 kdrv_sie_test_init_flag = 0;

//ch0 raw out
//lineoffset = width * sie_out_bitdepth / 8
//sie_out_bitdepth only support 12bit when raw encode enable
//if raw encode enable: lineoffset = scale_out_h * RAW_ENC_BUF_RATIO / 100
//buf_size = lineoffset * scale_out_h
#define KDRV_SIE_CAL_RAW_BUF_SIZE(lofs, scale_out_h) ALIGN_CEIL_4(lofs * scale_out_h)

//ch0/1 yuv out
//ch0 buf_size = lineoffset_ch0 * crop_h
//ch1 buf_size = lineoffset_ch1 * crop_h
#define KDRV_SIE_CAL_CCIR_BUF_SIZE(lofs, crp_h) ALIGN_CEIL_4(lofs * crp_h)

//ch1 ca, R/G/B/Cnt/IRth/Rth/Gth/Bth, total 8 channel, each channel @16bit
//buf_size = win_w * win_h * 8 * 2
#define KDRV_SIE_CAL_CA_BUF_SIZE() ALIGN_CEIL_4((KDRV_SIE_CA_MAX_WINNUM * KDRV_SIE_CA_MAX_WINNUM << 3) << 1)

//ch2 la, PreGamma Lum/PostGamma Lum @16bit
//buf_size = (win_w * win_h * 2 * 2)
#define KDRV_SIE_CAL_LA_BUF_SIZE() ALIGN_CEIL_4((KDRV_SIE_LA_MAX_WINNUM * KDRV_SIE_LA_MAX_WINNUM << 1) << 1)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL kdrv_sie_cmd_save_raw( unsigned char argc, char **pargv)
{
	UINT32 id = 0;
    CHAR out_filename[64];
	CHAR root_dir[11];
	UINT32 kdrv_sie_out_addr[KDRV_SIE_CH_MAX] = {0};
	UINT32 kdrv_sie_out_lofs[KDRV_SIE_CH_MAX] = {0};
	KDRV_SIE_DATA_FMT data_fmt = KDRV_SIE_BAYER_8;
	USIZE scl_out = {0};
	KDRV_DEV_ENGINE kdrv_eng_map[] = {
		KDRV_VDOCAP_ENGINE0,
		KDRV_VDOCAP_ENGINE1,
		KDRV_VDOCAP_ENGINE2,
		KDRV_VDOCAP_ENGINE3,
		KDRV_VDOCAP_ENGINE4
	};

	/*[id]*/
	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return 0;
	}
	sscanf_s(pargv[0], "%d", (int *)&id);

	kdrv_siet_cmd_kdrv_id[id] = KDRV_DEV_ID(KDRV_CHIP0, kdrv_eng_map[id], 0);
#if defined (__LINUX)
		snprintf(root_dir, sizeof(root_dir), "//mnt//sd//");
#elif defined (__FREERTOS)
		snprintf(root_dir, sizeof(root_dir), "A:\\");
#endif
	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_DATA_FMT, (void *)&data_fmt);
	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_ADDR, (void *)&kdrv_sie_out_addr[KDRV_SIE_CH0]);
	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_LOF, (void *)&kdrv_sie_out_lofs[KDRV_SIE_CH0]);
	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_SCALEOUT, (void *)&scl_out);

	if (data_fmt == KDRV_SIE_YUV_422_SPT || data_fmt == KDRV_SIE_YUV_420_SPT) {
		snprintf(out_filename, 64, "%sts%lld_y_id%d_0x%x_%d_%d.RAW", root_dir, (long long)hwclock_get_counter(), (int)id, (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH0], (int)kdrv_sie_out_lofs[KDRV_SIE_CH0], (int)scl_out.h);
		kdrv_sie_cmd_savefile(out_filename, kdrv_sie_out_addr[KDRV_SIE_CH0], (kdrv_sie_out_lofs[KDRV_SIE_CH0]*scl_out.h));
		kdrv_sie_dbg_dump("save y image id: %d, lofs: %d, h: %d\r\n", (int)id, (int)kdrv_sie_out_lofs[KDRV_SIE_CH0], (int)scl_out.h);

		snprintf(out_filename, 64, "%sts%lld_uv_id%d_0x%x_%d_%d.RAW", root_dir, (long long)hwclock_get_counter(), (int)id, (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH0], (int)kdrv_sie_out_lofs[KDRV_SIE_CH0], (int)scl_out.h);
		kdrv_sie_cmd_savefile(out_filename, kdrv_sie_out_addr[KDRV_SIE_CH1], (kdrv_sie_out_lofs[KDRV_SIE_CH1]*scl_out.h));
		kdrv_sie_dbg_dump("save uv image id: %d, lofs: %d, h: %d\r\n", (int)id, (int)kdrv_sie_out_lofs[KDRV_SIE_CH1], (int)scl_out.h);
	} else if (data_fmt == KDRV_SIE_YUV_422_NOSPT) {

	} else {		//RAW
		snprintf(out_filename, 64, "%sts%lld_raw_id%d_0x%x_%d_%d.RAW", root_dir, (long long)hwclock_get_counter(), (int)id, (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH0], (int)kdrv_sie_out_lofs[KDRV_SIE_CH0], (int)scl_out.h);
		kdrv_sie_cmd_savefile(out_filename, kdrv_sie_out_addr[KDRV_SIE_CH0], (kdrv_sie_out_lofs[KDRV_SIE_CH0]*scl_out.h));
		kdrv_sie_dbg_dump("save raw image id: %d, lofs: %d, h: %d\r\n", (int)id, (int)kdrv_sie_out_lofs[KDRV_SIE_CH0], (int)scl_out.h);
	}

	return 0;
}

static INT32 kdrv_sie_test_common_isr_cb(UINT32 id, UINT32 sts, void *in_data, void *out_data)
{
	UINT32 idx = 0;
	UINT32 kdrv_sie_out_addr[KDRV_SIE_CH_MAX] = {0};
	static UINT32 tmp_cnt[KDRV_SIE_MAX_ENG] = {0};

	if (sts & KDRV_SIE_INT_VD) {
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_out_addr[KDRV_SIE_CH0]);
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH1_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_out_addr[KDRV_SIE_CH1]);
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH2_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_out_addr[KDRV_SIE_CH2]);
//		if (kdrv_sie_test_cmd_fc[id] <= 20) {
		if (tmp_cnt[id] % 300 == 0) {
			kdrv_sie_dbg_dump("id:%d, cur out ch0_addr: %#x, ch1_addr: %#x, ch2_addr: %#x\r\n", (int)id, (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH0], (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH1], (unsigned int)kdrv_sie_out_addr[KDRV_SIE_CH2]);
		}

		idx = (kdrv_sie_test_cmd_fc[id] % KDRV_SIE_TEST_RING_BUF_NUM);
		if (kdrv_sie_test_ppb[id][KDRV_SIE_CH0][idx] != 0) {
			kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH0][idx]);
		}
		if ((KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_CA) && (kdrv_sie_test_ppb[id][KDRV_SIE_CH1][idx] != 0)) {
			kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH1_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH1][idx]);
		}
		if ((KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_LA) && (kdrv_sie_test_ppb[id][KDRV_SIE_CH2][idx] != 0)) {
			kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH2_ADDR|KDRV_SIE_IGN_CHK, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH2][idx]);
		}
		kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_LOAD|KDRV_SIE_IGN_CHK, NULL);

//		if (kdrv_sie_test_cmd_fc[id] <= 20) {
		if (tmp_cnt[id] % 300 == 0) {
			kdrv_sie_dbg_dump("id:%d, next ch0_addr: %#x, ch1_addr: %#x, ch2_addr: %#x\r\n", (int)id, (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH0][idx], (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH1][idx], (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH2][idx]);
		}
		tmp_cnt[id]++;
		kdrv_sie_test_cmd_fc[id]++;
	}

	if (sts & KDRV_SIE_INT_CROPEND) {
	}
	return 0;
}

INT32 kdrv_sie_test_cmd_malloc(KDRV_SIE_VOS_MEM_INFO *vod_mem_info, UINT32 req_size)
{
	if (vos_mem_init_cma_info(&vod_mem_info->cma_info, VOS_MEM_CMA_TYPE_CACHE, req_size) != 0) {
		kdrv_sie_dbg_dump("init cma failed\r\n");
		return KDRV_SIE_E_NOMEM;
	}

	vod_mem_info->cma_hdl = vos_mem_alloc_from_cma(&vod_mem_info->cma_info);
	if (vod_mem_info->cma_hdl == NULL) {
		kdrv_sie_dbg_dump("alloc cma failed\r\n");
		return KDRV_SIE_E_NOMEM;
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_test_cmd_mfree(KDRV_SIE_VOS_MEM_INFO *vod_mem_info)
{
	if (vos_mem_release_from_cma(vod_mem_info->cma_hdl) != 0) {
		kdrv_sie_dbg_dump("release cma failed\r\n");
		return KDRV_SIE_E_NOMEM;
	}
	return KDRV_SIE_E_OK;
}

static void kdrv_sie_test_config_out_buffer(UINT32 id)
{
	UINT32 i = 0;
	UINT32 ch_output_size[KDRV_SIE_CH_MAX] = {0};
	KDRV_SIE_DATA_FMT data_fmt = KDRV_SIE_BAYER_8;
	UINT32 lofs_ch0 = 0, lofs_ch1 = 0;
	USIZE scl_out = {0};

	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_DATA_FMT, (void *)&data_fmt);
	kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_SCALEOUT, (void *)&scl_out);

	if (data_fmt == KDRV_SIE_YUV_422_SPT || data_fmt == KDRV_SIE_YUV_420_SPT) {
		//CCIR
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_LOF, (void *)(&lofs_ch0));
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH1_LOF, (void *)(&lofs_ch1));

		ch_output_size[0] = KDRV_SIE_CAL_CCIR_BUF_SIZE(lofs_ch0, scl_out.h);
		ch_output_size[1] = KDRV_SIE_CAL_CCIR_BUF_SIZE(lofs_ch1, scl_out.h);
	} else if (data_fmt == KDRV_SIE_YUV_422_NOSPT) {
		kdrv_sie_get(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_LOF, (void *)(&lofs_ch0));
		ch_output_size[0] = KDRV_SIE_CAL_CCIR_BUF_SIZE(lofs_ch0, scl_out.h);
	} else {		//RAW
		//ch0
		ch_output_size[KDRV_SIE_CH0] = KDRV_SIE_CAL_RAW_BUF_SIZE(SIE_OUT_PUB_BUF_W, SIE_OUT_PUB_BUF_H);

		for (i = 0; i < KDRV_SIE_TEST_RING_BUF_NUM; i++) {
			if (kdrv_sie_test_cmd_malloc(&kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH0][i], ch_output_size[KDRV_SIE_CH0]) == KDRV_SIE_E_OK) {
				kdrv_sie_test_ppb[id][KDRV_SIE_CH0][i] = kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH0][i].cma_info.vaddr;
				kdrv_sie_dbg_dump("id %d, ch0 pub_buf[%d] = 0x%x, size = 0x%x\r\n", (int)id, (int)i, (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH0][i], ch_output_size[KDRV_SIE_CH0]);
			} else {
				return;
			}
		}

		if (KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_CA) {
			ch_output_size[1] = KDRV_SIE_CAL_CA_BUF_SIZE();

			for (i = 0; i < KDRV_SIE_TEST_RING_BUF_NUM; i++) {
				if (kdrv_sie_test_cmd_malloc(&kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH1][i], ch_output_size[KDRV_SIE_CH1]) == KDRV_SIE_E_OK) {
					kdrv_sie_test_ppb[id][KDRV_SIE_CH1][i] = kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH1][i].cma_info.vaddr;
					kdrv_sie_dbg_dump("id %d, ch1 pub_buf[%d] = 0x%x, size = 0x%x\r\n", (int)id, (int)i, (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH1][i], ch_output_size[KDRV_SIE_CH1]);
				} else {
					return;
				}
			}
		}
	}

	if (KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_LA) {
		ch_output_size[2] = KDRV_SIE_CAL_LA_BUF_SIZE();

		for (i = 0; i < KDRV_SIE_TEST_RING_BUF_NUM; i++) {
			if (kdrv_sie_test_cmd_malloc(&kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH2][i], ch_output_size[KDRV_SIE_CH2]) == KDRV_SIE_E_OK) {
				kdrv_sie_test_ppb[id][KDRV_SIE_CH2][i] = kdrv_vos_mem_sie_out_buf_info[id][KDRV_SIE_CH2][i].cma_info.vaddr;
				kdrv_sie_dbg_dump("id %d, ch2 pub_buf[%d] = 0x%x, size = 0x%x\r\n", (int)id, (int)i, (unsigned int)kdrv_sie_test_ppb[id][KDRV_SIE_CH2][i], ch_output_size[KDRV_SIE_CH2]);
			} else {
				return;
			}
		}
	}
}

static void kdrv_sie_test_free_out_buffer(UINT32 id)
{
	UINT32 i, j;

	for (i = 0; i < KDRV_SIE_CH_MAX; i++) {
		for (j = 0; j < KDRV_SIE_TEST_RING_BUF_NUM; j++) {
			if (kdrv_vos_mem_sie_out_buf_info[id][i][j].cma_hdl != NULL) {
				kdrv_sie_test_cmd_mfree(&kdrv_vos_mem_sie_out_buf_info[id][i][j]);
			}
		}
	}
}

static BOOL kdrv_sie_test_cmd_config(UINT32 id, UINT32 pat_gen_mode, UINT32 out_bit_depth)
{
	KDRV_SIE_DATA_FMT data_fmt = KDRV_SIE_BAYER_8;
	KDRV_SIE_OUT_DEST out_dest = KDRV_SIE_OUT_DEST_DRAM;
	KDRV_SIE_OUTPUT_MODE_TYPE out_mode = KDRV_SIE_NORMAL_OUT;
	KDRV_SIE_RAW_ENCODE encode_info = {ENABLE, KDRV_SIE_ENC_58};
	KDRV_SIE_INT int_en = KDRV_SIE_INT_VD|KDRV_SIE_INT_DRAM_OUT0_END|KDRV_SIE_INT_CROPEND;
	KDRV_SIE_PATGEN_INFO *pat_gen_info;
	KDRV_SIE_PATGEN_INFO pat_gen_info_h_i = {KDRV_SIE_PAT_HINCREASE, 0x800, {1928, 1088}};
	KDRV_SIE_PATGEN_INFO pat_gen_info_h_v_i = {KDRV_SIE_PAT_HVINCREASE, 0x800, {1928, 1088}};
	KDRV_SIE_PATGEN_INFO pat_gen_info_c_b = {KDRV_SIE_PAT_COLORBAR, 0x64, {1928, 1088}};
	KDRV_SIE_PATGEN_INFO pat_gen_info_rand = {KDRV_SIE_PAT_RANDOM, 0x64, {1928, 1088}};
	KDRV_SIE_PATGEN_INFO pat_gen_info_fixed = {KDRV_SIE_PAT_FIXED, 0x64, {1928, 1088}};
	URECT sie_act_win = {2, 2, 1920, 1080};
	URECT sie_crp_win = {0, 0, 1920, 1080};
	UINT32 ch0_lofs = 0;

#if KDRV_SIE_TEST_CMD_PAT_GEN_EN
	switch (pat_gen_mode) {
		case KDRV_SIE_PAT_COLORBAR:
			pat_gen_info = &pat_gen_info_c_b;
			break;

		case KDRV_SIE_PAT_RANDOM:
			pat_gen_info = &pat_gen_info_rand;
			break;

		case KDRV_SIE_PAT_FIXED:
			pat_gen_info = &pat_gen_info_fixed;
			break;

		case KDRV_SIE_PAT_HINCREASE:
			pat_gen_info = &pat_gen_info_h_i;
			break;

		case KDRV_SIE_PAT_HVINCREASE:
			pat_gen_info = &pat_gen_info_h_v_i;
			break;
	}
	pat_gen_info = &pat_gen_info_h_v_i;
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_PATGEN_INFO, (void *)pat_gen_info);
#else
//sensor in
	//set signal/data phase/
#endif

//ACT and Crp window
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_ACT_WIN, (void *)&sie_act_win);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CROP_WIN, (void *)&sie_crp_win);

//Raw Encode
	if (KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_ENCODE) {
		data_fmt = KDRV_SIE_BAYER_12;
		kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_ENCODE, (void *)&encode_info);
	} else {
		if (out_bit_depth == KDRV_SIE_BAYER_8) {
			data_fmt = KDRV_SIE_BAYER_8;
		} else if (out_bit_depth == KDRV_SIE_BAYER_10) {
			data_fmt = KDRV_SIE_BAYER_10;
		} else if (out_bit_depth == KDRV_SIE_BAYER_12) {
			data_fmt = KDRV_SIE_BAYER_12;
		} else if (out_bit_depth == KDRV_SIE_BAYER_16) {
			data_fmt = KDRV_SIE_BAYER_16;
		} else {
			data_fmt = KDRV_SIE_BAYER_12;
		}
	}
//output
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_DATA_FMT, (void *)&data_fmt);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_OUT_DEST, (void *)&out_dest);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_OUTPUT_MODE, (void *)&out_mode);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_INTE, (void *)&int_en);

//Buffer Calculation
	kdrv_sie_test_config_out_buffer(id);

//addr/lineoffset
	if (data_fmt == KDRV_SIE_BAYER_8) {
		ch0_lofs = sie_crp_win.w;
	} else if (data_fmt == KDRV_SIE_BAYER_10) {
		ch0_lofs = sie_crp_win.w*10/8;
	} else if (data_fmt == KDRV_SIE_BAYER_12) {
		ch0_lofs = sie_crp_win.w*12/8;
	} else {
		ch0_lofs = sie_crp_win.w*16/8;
	}

	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_ADDR, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH0][0]);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH0_LOF, (void *)&ch0_lofs);
	if (KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_CA) {
		kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH1_ADDR, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH1][0]);
	}
	if (KDRV_SIE_TEST_CMD_FUNC_EN & EXAM_KDRV_SIE_FUNC_LA) {
		kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_CH2_ADDR, (void *)&kdrv_sie_test_ppb[id][KDRV_SIE_CH2][0]);
	}
	kdrv_sie_test_cmd_fc[id] = 1;
//isr callback
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_ISRCB, (void *)kdrv_sie_test_common_isr_cb);
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_LOAD, NULL);
	return TRUE;
}

BOOL kdrv_sie_test_cmd_open( unsigned char argc, char **pargv)
{
	UINT32 buf_addr = 0, buf_size = 0;
	KDRV_SIE_OPENCFG kdrv_open_cfg = {0};
	KDRV_SIE_MCLK_INFO mclk_info = {0};
	KDRV_DEV_ENGINE kdrv_eng_map[] = {
		KDRV_VDOCAP_ENGINE0,
		KDRV_VDOCAP_ENGINE1,
		KDRV_VDOCAP_ENGINE2,
		KDRV_VDOCAP_ENGINE3,
		KDRV_VDOCAP_ENGINE4
	};
	UINT32 id = KDRV_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&id);
	}

	kdrv_sie_dbg_dump("kdrv_sie_open: id: %d\r\n", id);

	//init
	if (kdrv_sie_test_init_flag == 0) {
		buf_size = kdrv_sie_buf_query(3);
		if (kdrv_sie_test_cmd_malloc(&kdrv_vos_mem_info, buf_size) == KDRV_SIE_E_OK) {
			kdrv_sie_dbg_ind("auto allocate kflow sie private memory\r\n");
			buf_addr = kdrv_vos_mem_info.cma_info.vaddr;
		} else {
			return -1;
		}
		kdrv_sie_buf_init(buf_addr, buf_size);
	}
	kdrv_sie_test_init_flag |= 1<<id;

	//open
	kdrv_siet_cmd_kdrv_id[id] = KDRV_DEV_ID(KDRV_CHIP0, kdrv_eng_map[id], 0);

	mclk_info.clk_en = ENABLE;
#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
		mclk_info.mclk_src_sel = KDRV_SIE_MCLKSRC_PLL5;
		mclk_info.clk_rate = 1000000;
#else
		mclk_info.mclk_src_sel = KDRV_SIE_MCLKSRC_480;
		mclk_info.clk_rate = 120000000;
#endif

	mclk_info.mclk_id_sel = KDRV_SIE_MCLK;
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_MCLK, (void *)&mclk_info);//set mclk

	kdrv_open_cfg.act_mode = KDRV_SIE_IN_PATGEN;
	kdrv_open_cfg.clk_src_sel = KDRV_SIE_CLKSRC_480;
	kdrv_open_cfg.pclk_src_sel = KDRV_SIE_PXCLKSRC_MCLK;
	kdrv_open_cfg.data_rate = 480000000;
	kdrv_sie_set(kdrv_siet_cmd_kdrv_id[id], KDRV_SIE_ITEM_OPENCFG, (void *)&kdrv_open_cfg);//open config
	kdrv_sie_open(KDRV_CHIP0, kdrv_eng_map[id]);// open sie

	return 0;
}

BOOL kdrv_sie_test_cmd_close( unsigned char argc, char **pargv)
{
	KDRV_DEV_ENGINE kdrv_eng_map[] = {
		KDRV_VDOCAP_ENGINE0,
		KDRV_VDOCAP_ENGINE1,
		KDRV_VDOCAP_ENGINE2,
		KDRV_VDOCAP_ENGINE3,
		KDRV_VDOCAP_ENGINE4
	};
	UINT32 id = KDRV_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&id);
	}

	kdrv_sie_dbg_dump("kdrv_sie_close: id: %d\r\n", id);
	//close sie
	kdrv_sie_close(KDRV_CHIP0, kdrv_eng_map[id]);

	//reset frame count
	kdrv_sie_test_cmd_fc[id] = 0;

	//uninit buffer
	kdrv_sie_test_init_flag &= ~(1 << id);
	if (kdrv_sie_test_init_flag == 0) {
		kdrv_sie_buf_uninit();
		kdrv_sie_test_cmd_mfree(&kdrv_vos_mem_info);	//free kdrv handle buffer
	}
	kdrv_sie_test_free_out_buffer(id);				//free output buffer

	return 0;
}

BOOL kdrv_sie_test_cmd_start( unsigned char argc, char **pargv)
{
	UINT32 pat_gen_mode = 0;
	UINT32 out_bit_depth = 0;
	KDRV_SIE_TRIG_INFO kdrv_trig_info = {0};
	UINT32 id = KDRV_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&pat_gen_mode);
		sscanf_s(pargv[2], "%d", (int *)&out_bit_depth);
	}

	kdrv_sie_dbg_dump("kdrv_sie_start: id: %d, pattern gen mode: %d, output bitdepth: %d\r\n", id, pat_gen_mode, out_bit_depth);
	//config sie parameters
	kdrv_sie_test_cmd_config(id, pat_gen_mode, out_bit_depth);

	//trig start
	kdrv_trig_info.trig_type = KDRV_SIE_TRIG_START;
	kdrv_trig_info.wait_end = FALSE;
	kdrv_sie_trigger(kdrv_siet_cmd_kdrv_id[id], NULL, NULL, (void *)&kdrv_trig_info);

	return 0;
}

BOOL kdrv_sie_test_cmd_stop( unsigned char argc, char **pargv)
{
	KDRV_SIE_TRIG_INFO kdrv_trig_info = {0};
	UINT32 id = KDRV_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&id);
	}

	kdrv_sie_dbg_dump("kdrv_sie_stop: id: %d\r\n", id);
	//stop
	kdrv_trig_info.trig_type = KDRV_SIE_TRIG_STOP;
	kdrv_trig_info.wait_end = FALSE;
	kdrv_sie_trigger(kdrv_siet_cmd_kdrv_id[id], NULL, NULL, (void *)&kdrv_trig_info);

	return 0;
}

BOOL kdrv_sie_test_cmd_on( unsigned char argc, char **pargv)
{
	UINT32 i = 0;
	UINT32 id = KDRV_SIE_ID_1;
	UINT32 pat_gen_mode = KDRV_SIE_PAT_HVINCREASE;
	UINT32 out_bit_depth = KDRV_SIE_BAYER_8;

	memset((void *)kdrv_sie_cmd_buf, 0, KDRV_SIE_CMD_LEN * 4);
	for (i = 0; i < 4; i++) {
		kdrv_sie_cmd[i] = &kdrv_sie_cmd_buf[KDRV_SIE_CMD_LEN * i];
	}

	switch (argc) {
	case 1:
		sscanf_s(pargv[0], "%d", (int *)&id);
		break;

	case 2:
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&pat_gen_mode);
		break;

	case 3:
		sscanf_s(pargv[0], "%d", (int *)&id);
		sscanf_s(pargv[1], "%d", (int *)&pat_gen_mode);
		sscanf_s(pargv[2], "%d", (int *)&out_bit_depth);
		break;
	}

	snprintf(kdrv_sie_cmd[0], KDRV_SIE_CMD_LEN, "%d", (int)id);
	snprintf(kdrv_sie_cmd[1], KDRV_SIE_CMD_LEN, "%d", (int)pat_gen_mode);
	snprintf(kdrv_sie_cmd[2], KDRV_SIE_CMD_LEN, "%d", (int)out_bit_depth);

	kdrv_sie_dbg_dump("kdrv_sie_on: id: %d, pattern gen mode: %d, output bitdepth: %d\r\n", id, pat_gen_mode, out_bit_depth);
	kdrv_sie_test_cmd_open(1, kdrv_sie_cmd);	//init and open
	kdrv_sie_test_cmd_start(3, kdrv_sie_cmd);	//config and start

	return 0;
}

BOOL kdrv_sie_test_cmd_off( unsigned char argc, char **pargv)
{
	UINT32 id = KDRV_SIE_ID_1;

	if (argc > 0) {
		sscanf_s(pargv[0], "%d", (int *)&id);
	}
	snprintf(kdrv_sie_cmd[0], KDRV_SIE_CMD_LEN, "%d", (int)id);

	kdrv_sie_dbg_dump("kdrv_sie_off: id: %d\r\n", id);
	kdrv_sie_test_cmd_stop(1, kdrv_sie_cmd);	//stop
	kdrv_sie_test_cmd_close(1, kdrv_sie_cmd);	//close and uninit
	return 0;
}
#endif

#if 0
#endif

int kdrv_sie_cmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_sie);
	UINT32 loop = 1;

	kdrv_sie_dbg_dump("---------------------------------------------------------------------\r\n");
	kdrv_sie_dbg_dump("  %s\n", "kdrv_sie");
	kdrv_sie_dbg_dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		kdrv_sie_dbg_dump("%15s : %s\r\n", kdrv_sie[loop].p_name, kdrv_sie[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int kdrv_sie_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(kdrv_sie, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_sie);
	UINT32 loop;
	int    ret;
	unsigned char ucargc = 0;

//	kdrv_sie_dbg_dump("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);
	if (strncmp(argv[1], "?", 2) == 0) {
		kdrv_sie_cmd_showhelp();
		return 0;
	}

	if (argc < 1) {
		kdrv_sie_dbg_dump("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], kdrv_sie[loop].p_name, strlen(argv[1])) == 0) {
			ret = kdrv_sie[loop].p_func(ucargc, &argv[2]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		kdrv_sie_dbg_dump("Invalid CMD !!\r\n");
		kdrv_sie_cmd_showhelp();
		return -1;
	}
	return 0;
}

