/*
 * lv_user_font_conv_cmd.c
 *
 *  Created on: 2021¦~11¤ë17¤é
 *      Author: NVT02970
 */

#include <kwrap/stdio.h>
#include <string.h>
#include "kwrap/nvt_type.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include "hd_common.h"
#include "FileSysTsk.h"
#include "PrjInc.h"
#include "hd_common.h"
#ifdef __FREERTOS
#include "FreeRTOS.h"
#include <kflow_common/nvtmpp.h>
#endif

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          LvUserFontConvCmd
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#include "UIApp/lv_user_font_conv/lv_user_font_conv.h"


#define MAX_WORKER_NUM 						10
#define LV_USER_FONT_CONV_TEST_FONT_RES_ID 	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_32_1BPP
#define LV_USER_FONT_CONV_TEST_STRING_ID	LV_PLUGIN_STRING_ID_STRID_IMGSIZE

static HD_RESULT lv_user_font_conv_get_hd_common_buf(UINT32 size, lv_user_font_conv_mem_cfg* mem, INT32* blk)
{
	UINT32 pa;

	*blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, size, DDR_ID0); // Get block from mem pool
	if (*blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", *blk );
		return HD_ERR_SYS;
	}

	pa = hd_common_mem_blk2pa(*blk); // get physical addr
	if (pa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", *blk);
		return HD_ERR_SYS;
	}

	mem->output_buffer = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);
	if (mem->output_buffer == NULL) {
		DBG_ERR("Convert to VA failed for file buffer for decoded buffer!\r\n");
		return HD_ERR_SYS;
	}

	mem->output_buffer_size = size;

	return HD_OK;
}

static HD_RESULT lv_user_font_conv_release_hd_common_buf(const lv_user_font_conv_mem_cfg mem, const INT32 blk)
{
	hd_common_mem_munmap(mem.output_buffer, mem.output_buffer_size);
	hd_common_mem_release_block(blk);
	return HD_OK;
}

typedef struct {

	THREAD_HANDLE tid;
	int task_running;
	ID fid;
	char name[64];
	int max_cnt;
	HD_VIDEO_PXLFMT fmt;
} LV_USER_FONT_CONV_TEST_WORKER_PARAM;

#define FLG_LV_FONT_CONV_WORKER_IDLE         FLGPTN_BIT(0)

lv_font_t * g_font = NULL;
char* g_string = NULL;

THREAD_RETTYPE lv_user_font_conv_test_worker(void* arg)
{
	LV_USER_FONT_CONV_TEST_WORKER_PARAM* param =  (LV_USER_FONT_CONV_TEST_WORKER_PARAM*) arg;

	THREAD_ENTRY();

	lv_user_font_conv_draw_cfg draw_cfg = {0};
	lv_user_font_conv_mem_cfg mem_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};
	INT32 blk;
	int cnt = 0;

	DBG_DUMP("%p %s started\r\n", param, param->name);

	lv_user_font_conv_draw_cfg_init(&draw_cfg);

	draw_cfg.fmt = param->fmt; /* ARGB4444 */
	draw_cfg.string.font = g_font;
	draw_cfg.string.text = g_string;

	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);

	lv_user_font_conv_get_hd_common_buf(result.output_buffer_size, &mem_cfg, &blk);

	while(cnt++ < param->max_cnt)
	{
		lv_user_font_conv(&draw_cfg, &mem_cfg);
		usleep(1000*500);
	}

	lv_user_font_conv_release_hd_common_buf(mem_cfg, blk);

	DBG_DUMP("%s ended\r\n", param->name);

	set_flg(param->fid, FLG_LV_FONT_CONV_WORKER_IDLE);

	THREAD_RETURN(0);

}

LV_USER_FONT_CONV_TEST_WORKER_PARAM worker_param[MAX_WORKER_NUM] = {0};

ER lv_user_font_conv_test_multi_thread(int worker_num, int max_cnt, HD_VIDEO_PXLFMT fmt)
{

	FLGPTN			flag = 0;


	DBG_DUMP("worker_num = %d max_cnt = %d fmt=%lx\r\n", worker_num, max_cnt, fmt);

	if(worker_num > MAX_WORKER_NUM)
		worker_num = MAX_WORKER_NUM;

	g_font = (lv_font_t*) lv_plugin_get_font(LV_USER_FONT_CONV_TEST_FONT_RES_ID)->font;
	g_string = (char*) lv_plugin_get_string(LV_USER_FONT_CONV_TEST_STRING_ID)->ptr;

	for(int i=0 ; i<worker_num ; i++)
	{
		LV_USER_FONT_CONV_TEST_WORKER_PARAM* param = &worker_param[i];

		sprintf(param->name, "lv_fconv%d", i);
		param->max_cnt = max_cnt;
		param->fmt = fmt;

		if(param->fid == 0){

			if ((vos_flag_create(&param->fid, NULL, param->name)) != E_OK) {
				DBG_ERR("%s fail\r\n", param->name);
			}
		}

		if(param->tid == 0){

			param->tid = vos_task_create(lv_user_font_conv_test_worker, param, param->name, 16, 4096);

			if (param->tid == 0)
			{
				param->tid = 0;
				DBG_ERR("lv_user_font_conv_test_worker create failed.\r\n");
			}
			else{
				clr_flg(param->fid, FLG_LV_FONT_CONV_WORKER_IDLE);
				vos_task_resume(worker_param[i].tid);
			}

			usleep(1000*100);
		}
	}

	for(int i=0 ; i<worker_num ; i++)
	{
		LV_USER_FONT_CONV_TEST_WORKER_PARAM* param = &worker_param[i];
		wai_flg(&flag, param->fid, FLG_LV_FONT_CONV_WORKER_IDLE, TWF_ORW | TWF_CLR);
	}


	for(int i=0 ; i<worker_num ; i++)
	{
		LV_USER_FONT_CONV_TEST_WORKER_PARAM* param = &worker_param[i];

		vos_task_destroy(param->tid);
		param->tid = 0;

		usleep(500*100);
	}


	return 0;

}

ER lv_user_font_conv_test_style(lv_user_font_conv_draw_cfg draw_cfg)
{
	lv_user_font_conv_mem_cfg mem_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};
	INT32 blk;

	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);

	mem_cfg.working_buffer = NULL;
	lv_user_font_conv_get_hd_common_buf(result.output_buffer_size, &mem_cfg, &blk);
	lv_user_font_conv(&draw_cfg, &mem_cfg);

	lv_user_font_conv_release_hd_common_buf(mem_cfg, blk);

	return 0;

}

static BOOL Cmd_lv_user_font_conv_dbg_write_file(unsigned char argc, char **argv)
{

	bool enabled = false;

	if(argc > 0)
		enabled = atoi(argv[0]);

	lv_user_font_conv_dbg_set_write_file(enabled);

	DBG_DUMP("set %s\r\n", enabled ? "enabled" : "disabled");

	return TRUE;

}

static BOOL Cmd_lv_user_font_conv_test_multi_thread(unsigned char argc, char **argv)
{
	int worker_num = 0;
	int max_cnt = 100;
	uint8_t cnt = 0;
	HD_VIDEO_PXLFMT fmt = HD_VIDEO_PXLFMT_ARGB4444;

	if(argc > cnt){
		worker_num = atoi(argv[cnt]);
		cnt++;
	}

	if(argc > cnt){
		max_cnt = atoi(argv[cnt]);
		cnt++;
	}

	if(argc > cnt){
		uint8_t fmt_id = atoi(argv[cnt]);

		switch(fmt_id)
		{
		case 0:
			fmt = HD_VIDEO_PXLFMT_ARGB4444;
			break;

		case 1:
			fmt = HD_VIDEO_PXLFMT_YUV420;
			break;

		case 2:
			fmt = HD_VIDEO_PXLFMT_YUV422;
			break;

		default:
			fmt = HD_VIDEO_PXLFMT_ARGB4444;
			break;
		}
		cnt++;

		DBG_DUMP("fmt_id = %u\r\n", fmt_id);
	}

	if(worker_num){
		lv_user_font_conv_test_multi_thread(worker_num, max_cnt, fmt);
	}
	else{

	}

	return TRUE;

}

static BOOL Cmd_lv_user_font_conv_test_style(unsigned char argc, char **argv)
{
	lv_color32_t color32;
	lv_user_font_conv_draw_cfg draw_cfg;
	uint8_t cnt = 0;

	lv_user_font_conv_draw_cfg_init(&draw_cfg);

	if(argc > cnt){
		uint8_t fmt_id = atoi(argv[cnt]);

		switch(fmt_id)
		{
		case 0:
			draw_cfg.fmt = HD_VIDEO_PXLFMT_ARGB4444;
			break;

		case 1:
			draw_cfg.fmt = HD_VIDEO_PXLFMT_YUV420;
			break;

		case 2:
			draw_cfg.fmt = HD_VIDEO_PXLFMT_YUV422;
			break;

		default:
			draw_cfg.fmt = HD_VIDEO_PXLFMT_ARGB4444;
			break;
		}
		cnt++;

		DBG_DUMP("fmt_id = %u\r\n", fmt_id);
	}

	if(argc > cnt){
		lv_plugin_res_id font_id = atoi(argv[cnt]);
		draw_cfg.string.font = (lv_font_t*)lv_plugin_get_font(font_id)->font;
		cnt++;

		DBG_DUMP("font_id = %u\r\n", font_id);
	}

	if(argc > cnt){
		lv_plugin_res_id string_id = atoi(argv[cnt]);
		draw_cfg.string.text =  (char*)lv_plugin_get_string(string_id)->ptr;
		cnt++;

		DBG_DUMP("string_id = %u\r\n", string_id);
	}

	if(argc > cnt){
		lv_coord_t letter_space = atoi(argv[cnt]);
		draw_cfg.string.letter_space = letter_space;
		cnt++;

		DBG_DUMP("letter_space = %u\r\n", letter_space);
	}

	if(argc > cnt){
		lv_opa_t opa = atoi(argv[cnt]);

		draw_cfg.string.opa = opa;
		draw_cfg.bg.opa = opa;
		draw_cfg.border.opa = opa;
		cnt++;

		DBG_DUMP("opa = %u\r\n", opa);
	}

	if(argc > cnt){
		color32.full = strtoul(argv[cnt], NULL, 16);
		draw_cfg.string.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
		cnt++;

		DBG_DUMP("text color = %x\r\n", draw_cfg.string.color);
	}

	if(argc > cnt){
		color32.full = strtoul(argv[cnt], NULL, 16);
		draw_cfg.bg.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
		cnt++;

		DBG_DUMP("bg color = %x\r\n", draw_cfg.string.color);
	}

	if(argc > cnt){
		color32.full = strtoul(argv[cnt], NULL, 16);
		draw_cfg.border.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
		cnt++;

		DBG_DUMP("border color = %x\r\n", draw_cfg.string.color);
	}

	if(argc > cnt){
		lv_align_t align = atoi(argv[cnt]);
		draw_cfg.string.align = align;
		cnt++;

		DBG_DUMP("text align = %x\r\n", draw_cfg.string.color);
	}

	if(argc > cnt){
		lv_coord_t width = atoi(argv[cnt]);
		draw_cfg.border.width = width;
		cnt++;

		DBG_DUMP("border width = %x\r\n", draw_cfg.border.width);
	}

	lv_user_font_conv_test_style(draw_cfg);

	return TRUE;

}

SXCMD_BEGIN(lv_user_font_conv_cmd_tbl, "lv user font conv command")
SXCMD_ITEM("dbg_write_file", Cmd_lv_user_font_conv_dbg_write_file, "dbg write file")
SXCMD_ITEM("test_multi_thread", Cmd_lv_user_font_conv_test_multi_thread, "test multi thread")
SXCMD_ITEM("test_style", Cmd_lv_user_font_conv_test_style, "test style")
SXCMD_END()

static int lv_user_font_conv_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(lv_user_font_conv_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "lv_user_font_conv");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", lv_user_font_conv_cmd_tbl[loop].p_name, lv_user_font_conv_cmd_tbl[loop].p_desc);
	}
	return 0;
}


MAINFUNC_ENTRY(lv_user_font_conv, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(lv_user_font_conv_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		lv_user_font_conv_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], lv_user_font_conv_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = lv_user_font_conv_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	lv_user_font_conv_cmd_showhelp(vk_printk);
	return 0;
}
