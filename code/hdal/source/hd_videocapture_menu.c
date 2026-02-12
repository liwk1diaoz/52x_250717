/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videoenc_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_common.h"
#include "hd_videocapture.h"
#define HD_MODULE_NAME HD_VIDEOCAP
#include "hd_int.h"
#include <string.h>

#define OUT_COUNT 	1

#define DBG_ERR(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)
#define DBGH(x) 						printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define CHKPNT      					printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)

extern HD_RESULT _hd_videocap_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_videocap_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_videocap_get_state(HD_OUT_ID _out_id, UINT32* p_state);

int _hd_videocap_dump_info(UINT32 did)
{
	int outpath;
	// Make path0 = IN_0 - OUT_0
	#define HD_VIDEOCAP_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	static CHAR  m_str[3][8] = {"OFF","ON","AUTO"};


	HD_IN_ID video_src_out_id = 0;
	CHAR  src_out[32];
	UINT32 state = 0;
	HD_IN_ID video_dest_in_id = 0;
	CHAR  dest_in[32];
	HD_PATH_ID video_cap_ctrl = HD_VIDEOCAP_PATH(HD_DAL_VIDEOCAP(did), HD_CTRL, HD_CTRL);
	UINT32 i;


	for (outpath = 0; outpath < OUT_COUNT; outpath++) {
		_hd_videocap_get_state(HD_VIDEOCAP_OUT(did, outpath), &state);
		if (state > 0) {
			HD_VIDEOCAP_CTRL param_ctrl;
			HD_VIDEOCAP_FUNC_CONFIG param_func_cfg;
			HD_VIDEOCAP_IN  param_in;
			HD_VIDEOCAP_DRV_CONFIG param_drv;
			CHAR  pxlfmt[16], dir[8];
			UINT32 func;
			HD_VIDEOCAP_OUT  param_out;
			HD_VIDEOCAP_CROP param_out_crop = {0};
			HD_PATH_ID video_cap_path = HD_VIDEOCAP_PATH(HD_DAL_VIDEOCAP(did), HD_IN(0), HD_OUT(outpath));
			UINT32 shdr_map;

			if (hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &param_drv)) {
					return HD_ERR_SYS;
			}
			if (strlen(param_drv.sen_cfg.sen_dev.driver_name) == 0) {
				//skip SHDR sub path
				continue;
			}
			_hd_dump_printf("------------------------- VIDEOCAP %-2d PATH & BIND -----------------------------\r\n", did);
			_hd_dump_printf("in      out     state   bind_src                bind_dest\r\n");
			_hd_videocap_get_bind_src(HD_VIDEOCAP_IN(did, 0), &video_src_out_id);
			_hd_src_out_id_str(HD_GET_DEV(video_src_out_id), HD_GET_OUT(video_src_out_id), src_out, 32);

			_hd_videocap_get_bind_dest(HD_VIDEOCAP_OUT(did, outpath), &video_dest_in_id);
			_hd_dest_in_id_str(HD_GET_DEV(video_dest_in_id), HD_GET_IN(video_dest_in_id), dest_in, 32);
			_hd_dump_printf("%-4d    %-4d    %-5s   %-20s    %-20s\r\n",
							0, did, s_str[state], src_out, dest_in);

			_hd_dump_printf("------------------------- VIDEOCAP %-2d DRV CONFIG ------------------------------\r\n", did);

			shdr_map = param_drv.sen_cfg.shdr_map;
			_hd_dump_printf("driver_name     if_type shdr_map\r\n");


			_hd_dump_printf("%-16s%d       %d\r\n", param_drv.sen_cfg.sen_dev.driver_name,
																				param_drv.sen_cfg.sen_dev.if_type,
																				shdr_map);

			_hd_dump_printf("sensor_pinmux   serial_if_pinmux     cmd_if_pinmux clk_lane_sel\r\n");
			_hd_dump_printf("0x%08X      0x%08X           0x%08X    0x%08X\r\n", param_drv.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux,
																			param_drv.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux,
																			param_drv.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux,
																			param_drv.sen_cfg.sen_dev.pin_cfg.clk_lane_sel);
			_hd_dump_printf("sen_2_serial_pin_map[0:7]\r\n");
			for (i = 0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
				_hd_dump_printf("%-8d", 			param_drv.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i]);
			}
			_hd_dump_printf("\r\n");


			_hd_dump_printf("ccir_msblsb_switch       ccir_vd_hd_pin\r\n");
			_hd_dump_printf("%d                        %d\r\n", param_drv.sen_cfg.sen_dev.pin_cfg.ccir_msblsb_switch,
																						param_drv.sen_cfg.sen_dev.pin_cfg.ccir_vd_hd_pin);

			_hd_dump_printf("vx1_tx241_cko_pin        vx1_tx241_cfg_2lane_mode\r\n");
			_hd_dump_printf("%d                        %d\r\n", param_drv.sen_cfg.sen_dev.pin_cfg.vx1_tx241_cko_pin,
																param_drv.sen_cfg.sen_dev.pin_cfg.vx1_tx241_cfg_2lane_mode);


			_hd_dump_printf("vx1_en  if_sel  ctl_sel  tx_type\r\n");
			_hd_dump_printf("%d       %d       %d        %d\r\n", param_drv.sen_cfg.sen_dev.if_cfg.vx1.en,
																			param_drv.sen_cfg.sen_dev.if_cfg.vx1.if_sel,
																			param_drv.sen_cfg.sen_dev.if_cfg.vx1.ctl_sel,
																			param_drv.sen_cfg.sen_dev.if_cfg.vx1.tx_type);
			_hd_dump_printf("tge_en  swap    vcap_vd_src  vcap_sync_set\r\n");
			_hd_dump_printf("%d       %d       %d         %d             \r\n", param_drv.sen_cfg.sen_dev.if_cfg.tge.tge_en,
																			param_drv.sen_cfg.sen_dev.if_cfg.tge.swap,
																			param_drv.sen_cfg.sen_dev.if_cfg.tge.vcap_vd_src,
																			param_drv.sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set);

			_hd_dump_printf("optin_en        sen_map_if      if_time_out\r\n");
			_hd_dump_printf("0x%X             %d               %d\r\n", param_drv.sen_cfg.sen_dev.option.en_mask,
																			param_drv.sen_cfg.sen_dev.option.sen_map_if,
																			param_drv.sen_cfg.sen_dev.option.if_time_out);


			_hd_dump_printf("------------------------- VIDEOCAP %-2d CTRL ------------------------------------\r\n", did);
			_hd_dump_printf("AE      AWB     AF      WDR     SHDR    ETH\r\n");

			if (hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &param_ctrl)) {
				return HD_ERR_SYS;
			}
			func = param_ctrl.func;
			_hd_dump_printf("%d       %d       %d       %d       %d       %d\r\n", (func&HD_VIDEOCAP_FUNC_AE) != 0,
																(func&HD_VIDEOCAP_FUNC_AWB) != 0,
																(func&HD_VIDEOCAP_FUNC_AF) != 0,
																(func&HD_VIDEOCAP_FUNC_WDR) != 0,
																(func&HD_VIDEOCAP_FUNC_SHDR) != 0,
																(func&HD_VIDEOCAP_FUNC_ETH) != 0);
			_hd_dump_printf("------------------------- VIDEOCAP %-2d IN FRAME --------------------------------\r\n", did);
			_hd_dump_printf("in      mode    w       h       pxlfmt  frc     out_frame_num   crop\r\n");
			if (hd_videocap_get(video_cap_path, HD_VIDEOCAP_PARAM_IN, &param_in)) {
				return HD_ERR_SYS;
			}
			if (hd_videocap_get(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &param_out_crop)) {
				return HD_ERR_SYS;
			}
			_hd_video_pxlfmt_str(param_in.pxlfmt, pxlfmt, 16);
			if (param_in.sen_mode == HD_VIDEOCAP_SEN_MODE_AUTO) {
				_hd_dump_printf("%-4d    AUTO    %-4d    %-4d    %-7s %d/%d    %d               ",
					0, param_in.dim.w, param_in.dim.h, pxlfmt, GET_HI_UINT16(param_in.frc),GET_LO_UINT16(param_in.frc), param_in.out_frame_num);
			} else {
				_hd_dump_printf("%-4d    %-4d     %-4d    %-4d   %-7s %d/%d    %d               ",
					0, param_in.sen_mode, param_in.dim.w, param_in.dim.h, pxlfmt, GET_HI_UINT16(param_in.frc),GET_LO_UINT16(param_in.frc), param_in.out_frame_num);
			}
			if (param_out_crop.mode == HD_CROP_ON) {
				_hd_dump_printf("%s:{%d,%d,%d,%d}\r\n",
					m_str[param_out_crop.mode], param_out_crop.win.rect.x, param_out_crop.win.rect.y, param_out_crop.win.rect.w, param_out_crop.win.rect.h);
			} else if (param_out_crop.mode == HD_CROP_AUTO) {
				_hd_dump_printf("%s:{%d/%d,%d,(%d,%d}\r\n",
					m_str[param_out_crop.mode], GET_HI_UINT16(param_out_crop.auto_info.ratio_w_h), GET_LO_UINT16(param_out_crop.auto_info.ratio_w_h),
					param_out_crop.auto_info.factor, param_out_crop.auto_info.sft.x, param_out_crop.auto_info.sft.y);
			} else {
				_hd_dump_printf("%s\r\n",
					m_str[param_out_crop.mode]);
			}


			_hd_dump_printf("------------------------- VIDEOCAP %-2d OUT FRAME -------------------------------\r\n", did);
			_hd_dump_printf("out     w       h       pxlfmt  dir     ddr_id  out_func        depth     frc\r\n");

			if (hd_videocap_get(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &param_out)) {
				return HD_ERR_SYS;
			}
			if (hd_videocap_get(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &param_func_cfg)) {
				return HD_ERR_SYS;
			}
			_hd_video_pxlfmt_str(param_out.pxlfmt, pxlfmt, 16);
			_hd_video_dir_str(param_out.dir, dir, 8);
			_hd_dump_printf("%-4d    %-4d    %-4d    %-7s %-4s    %-4d    0x%08X      %-4d      %d/%d\r\n",
					outpath, param_out.dim.w, param_out.dim.h, pxlfmt, dir,
					param_func_cfg.ddr_id, param_func_cfg.out_func, param_out.depth,
					GET_HI_UINT16(param_out.frc),GET_LO_UINT16(param_out.frc));
		}
	}

	return 0;
}

int _hd_videocap_show_status_p(void)
{
	char cmd[256];

	snprintf(cmd, 256, "cat /proc/hdal/vcap/info");
	system(cmd);
	return 0;
}

static HD_DBG_MENU videocap_debug_menu[] = {
	{0x01, "dump info",                         _hd_videocap_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videocap_menu(void)
{
	return hd_debug_menu_entry_p(videocap_debug_menu, "VIDEOCAP");
}



